from threading import Thread, Event
import socket
from enum import IntEnum
from dataclasses import dataclass, asdict
import struct
from queue import Queue, Empty, Full
import time
import typing as t
import signal
import sys

from telemetry_def import TelemetryPacket, TelemetryStatus


class TelemetryPacketType(IntEnum):
    STATUS = 0x01
    TAGS = 0x02
    LOG = 0x03
    PING = 0x10
    TELEMETRY_CMD_SET_DETECTION_PARAMS = 0x30
    TELEMETRY_CMD_UNKNOWN = 0xFF

@dataclass
class TelemetryLog(TelemetryPacket):
    level: int
    timestamp: int
    msg: str = ''

    _fmt = '<BI'

@dataclass
class TelemetryCommandPacket(TelemetryPacket):

    def to_bytes(self) -> bytes:
        payload = super().to_bytes()
        header = struct.pack('BB', self._cmd, len(payload))
        return header + payload
    
    _cmd = TelemetryPacketType.TELEMETRY_CMD_UNKNOWN

@dataclass
class TelemetryPing(TelemetryPacket):
    
    def to_bytes(self) -> bytes:
        return b'\x10\00'


@dataclass
class TelemetryCommandSetDetectionParams(TelemetryCommandPacket):
    quad_decimate: float
    quad_sigma: float
    refine_edges: bool
    decode_sharpening: float

    _fmt = '<ffBf'
    _cmd = TelemetryPacketType.TELEMETRY_CMD_SET_DETECTION_PARAMS

@dataclass
class Point2F:
    x: float
    y: float

    _fmt = '<ff'

@dataclass
class Tag:
    id: int
    lastSeen: int
    center: Point2F
    p1: Point2F
    p2: Point2F
    p3: Point2F
    p4: Point2F

    _header_fmt = '<HI'

    @classmethod
    def size(cls) -> int:
        return struct.calcsize(cls._header_fmt) + 5 * struct.calcsize(Point2F._fmt)
    
    @classmethod
    def from_bytes(cls, raw: bytes) -> 'Tag':
        fmt = '<' + (cls._header_fmt + 5 * Point2F._fmt).replace('<', '')
        values = struct.unpack(fmt, raw)
        header = values[:2]
        floats = values[2:]
        
        tag = Tag(
            id=header[0],
            lastSeen=header[1],
            center=Point2F(floats[0], floats[1]),
            p1=Point2F(floats[2], floats[3]),
            p2=Point2F(floats[4], floats[5]),
            p3=Point2F(floats[6], floats[7]),
            p4=Point2F(floats[8], floats[9])
        )

        return tag

@dataclass
class TelemetryTags(TelemetryPacket):
    has_lock: bool
    locked_tag: Tag
    tags: t.List[Tag]


@dataclass
class TelemetryPacketHeader:
    type: TelemetryPacketType
    len: int

from abc import abstractmethod


class TelemetryPacketSubscriber:

    @abstractmethod
    def handle_telemetry_packet(self, packet: TelemetryPacket) -> None:
        pass



class TelemetryServer:

    def __init__(self) -> None:
        self._socket: socket.socket = None
        self._queue = Queue(maxsize = 500)
        self._discarded_packets = 0
        self._subscribers: t.Dict[TelemetryPacketType, TelemetryPacketSubscriber] = dict()
        self._tx = Queue()
        self._stop_flag = Event()
        self._socket_closed_flag = Event()

    def subscribe(self, packet_type: TelemetryPacketType, subscriber: TelemetryPacketSubscriber) -> None:
        if packet_type not in self._subscribers:
            self._subscribers[packet_type] = []
        self._subscribers[packet_type].append(subscriber)

    def get_all_packets_no_block(self) -> TelemetryPacket:
        packets = []
        packet = True
        while packet is not None:
            packet = self.get_packet(0)
            if packet:
                packets.append(packet)

        return packets

    def get_packet(self, timeout_s: int) -> TelemetryPacket:
        try:
            return self._queue.get(timeout_s)
        except Empty:
            return None
        
    def send_packet(self, packet: TelemetryPacket) -> None:
        self._tx.put(packet)

    def _connect(self, ip: str, port: int) -> bool:
        try:    

            if not self._socket is None:
                self._socket.close()

            self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._socket.setblocking(True)
            self._socket.settimeout(3)
            #self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            #self._socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, 2)  # 5 sec idle before keep-alive probe
            #self._socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, 1)  # 1 sec between probes
            #self._socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT, 3)  # 3 failed probes = disconnect
            self._socket.connect((ip, port))
            return True
        except (ConnectionRefusedError, OSError) as e:
            print(f'Failed to open connection.. {e}')
            return False

    def stop(self) -> None:
        self._stop_flag.set()

    def start(self, ip: str, port: int) -> None:
        self._stop_flag.clear()
        print(f'Telemetry server starting, reaching client at {ip}:{port}')

        while not self._stop_flag.is_set():
            if not self._connect(ip, port):
                time.sleep(1)
                print('retry?')
                continue

            print(f'Telemetry server Connected to {ip}:{port}')
            self._socket_closed_flag.clear()
            last_ping = 0

            while not self._socket_closed_flag.is_set() and not self._stop_flag.is_set():
                timed_out = False

                if (time.time() - last_ping) > 1:
                    try:
                        last_ping = time.time()
                        self._socket.send(TelemetryPing().to_bytes())
                    except Exception as e:
                        print(f'Error sending ping: {e}')

                # Read header
                header = self._read_header()
                if not header:
                    continue

                # Read payload data, if needed
                if header.len > 0:
                    try:
                        payload = self._socket.recv(header.len)
                        if not payload:
                            timed_out = True
                    except (socket.timeout, OSError):
                        timed_out = True

                    if timed_out:
                        self._timed_out()
                        break
                else:
                    payload = b''

                packet = None

                try:
                    if header.type == TelemetryPacketType.STATUS:
                        packet = TelemetryStatus(*struct.unpack(TelemetryStatus._fmt, payload))
                        #print(f'{packet.to_json()}')
                    elif header.type == TelemetryPacketType.TAGS:
                        nbr_of_tags = payload[0]
                        has_lock = payload[1]
                        locked_tag = Tag.from_bytes(payload[2:2+Tag.size()])
                        packet = TelemetryTags(has_lock, locked_tag, [])
                        #print(f'TAGS: {nbr_of_tags} ({packet_len} bytes): {payload}')
                        for tag in range(nbr_of_tags):
                            start_index = 2 + Tag.size() + (tag*Tag.size())
                            end_index = start_index + Tag.size()
                            tag = Tag.from_bytes(payload[start_index:end_index])
                            packet.tags.append(tag)
                    elif header.type == TelemetryPacketType.LOG:
                        # We'll parse the header automatically, but msg by hand, as length varies
                        header_len = struct.calcsize(TelemetryLog._fmt)
                        log = TelemetryLog(*struct.unpack(TelemetryLog._fmt, payload[:header_len]))
                        log.msg = payload[header_len:].decode('ascii').rstrip('\x00')
                        packet = log

                except struct.error as e:
                    print(f'Failed to parse packet for {header.type} ({payload})')
                    continue

                if not packet:
                    print('Unable to parse packet...?')
                    continue

                #print(header.type.name)

                # Call waiting subscribers
                for subscriber in self._subscribers.get(header.type, []):
                    subscriber.handle_telemetry_packet(packet)

                # Put packet to receiving queue
                try:
                    self._queue.put_nowait(packet)
                except Full:
                    # If the queue is full, we'll just discard the packets
                    self._discarded_packets += 1

                # At last, we'll send pending TX packets
                while not self._tx.empty():
                    try:
                        pkt = self._tx.get()
                        print(f'TX: {pkt} ({" ".join(hex(a)[2:].zfill(2) for a in pkt.to_bytes())})')
                        self._socket.send(pkt.to_bytes())
                    except Exception as e:
                        print(f'Failed to send packet: {e}')

            time.sleep(1)

        print('Stopping')
        try:
            self._socket_closed_flag.set()
            self._socket.close()
        except Exception as e:
            print(f'Error when closing TCP connection: {e}')

    def _timed_out(self) -> None:
        print('Timed out reading data, assuming client disconnected, closing socket and re-trying to open')
        self._socket_closed_flag.set()
        self._socket.close()

    def _read_header(self) -> TelemetryPacketHeader:
        timed_out = False

        # First 5 bytes are header
        # Byte 0-1 is magic (0x1234)
        # Byte 2 is packet type
        # Byte 3-4 is packet length

        try:
            header_size = 5
            header = self._socket.recv(header_size)
            if not header or len(header) != header_size:
                timed_out = True
        except (socket.timeout, OSError) as e:
            print(f'timed out reading: {e}')
            timed_out = True
            
        if timed_out:
            print('timed out reading!')
            self._timed_out()
            return None

        try:
            magic, packet_type_raw, packet_len = struct.unpack('<HBH', header)
            if magic != 0x1234:
                print('Incorrect magic')
                return None

            packet_type = TelemetryPacketType(packet_type_raw)

        except (ValueError, TypeError):
            try:
                hex_str = '0x' + hex(ord(packet_type_raw))[2:].zfill(2)
            except TypeError:
                hex_str = str(packet_type_raw)
                print(f'Invalid packet type {hex_str}')
                return None

        if packet_len > 10000:
            print('Packet length above 3000, probably wrong...!')
            return None

        return TelemetryPacketHeader(packet_type, packet_len)


server = TelemetryServer()
ip = '192.168.0.202'
port = 9096
t = Thread(target=server.start, args=(ip, port))

#def signal_handler(sig, frame):
#    global server
#    print("\nCtrl+C detected! Cleaning up before exiting...")
#    server.stop()
#    t.join()
#    sys.exit(0)

# Attach signal handler
#signal.signal(signal.SIGINT, signal_handler)


if __name__ == '__main__':
    t.start()
    t.join()
    print('Done!')