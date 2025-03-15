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
    group: int
    timestamp: int
    msg: str = ''

    _fmt = '<BBI'

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
class LandingTarget:
    angle_x: float  # rad	-  X-axis angular offset of the target from the center of the image
    angle_y: float  # rad	-  Y-axis angular offset of the target from the center of the image
    distance: float # m	-  Distance to the target from the vehicle
    size_x: float   # rad	-  Size of target along x-axis
    size_y: float   # rad	-  Size of target along y-axis
    id: int # uint16_t

    _fmt = '<fffffH'

    @classmethod
    def size(cls) -> int:
        return struct.calcsize(cls._fmt)
    
    def __str__(self) -> str:
        decimals = 3
        return f'[{self.id}] ({round(self.angle_x, decimals)}, {round(self.angle_y, decimals)}) {round(self.distance, decimals)}  ({round(self.size_x, decimals)}, {round(self.size_y, decimals)})'

@dataclass
class TelemetryTags(TelemetryPacket):
    has_lock: bool
    locked_tag: Tag
    landing_target: LandingTarget
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
        self._rx = Queue()
        self._stop_flag = Event()

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

    def stop(self) -> None:
        self._stop_flag.set()

    def _read(self, nbr_of_bytes: int, buf: bytearray) -> int:
        bytes_read = 0
        while bytes_read < nbr_of_bytes:
            bytes_left = nbr_of_bytes - bytes_read
            bytes_read += self._socket.recv_into(buf, bytes_left, socket.MSG_WAITALL)
        return True

    def start(self, ip: str, port: int) -> None:
        self._socket: socket.socket
        self._stop_flag.clear()

        while not self._stop_flag.is_set():
            connect_attempts = 0
            timeouts_in_row = 0
            try:
                print(f'Connecting to {ip}:{port}')
                connect_attempts += 1
                self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self._socket.setblocking(True)
                self._socket.settimeout(1)
                self._socket.connect((ip, port))
                print('Connected')
            except ConnectionRefusedError:
                print('Connection refused, retrying...')
                time.sleep(2)
                continue
            except TimeoutError:
                print(f'Connect {connect_attempts} timeout, retrying...')
                time.sleep(1)
                continue

            header_buf = bytearray(5)
            payload_buf = bytearray(1024)

            while True:
                try:
                    if not self._read(5, header_buf):
                        print('CONTINUE HEADER')
                        continue

                    header = self._parse_header(header_buf)
                    if not header:
                        print('CONTINUE HEADER 2')
                        continue

                    if not self._read(header.len, payload_buf):
                        print('CONTINUE HEADER')
                        continue

                    packet = self._parse_packet(header, payload_buf[:header.len])

                    if not packet:
                        print('Unable to parse packet...?')
                        continue

                    timeouts_in_row = 0

                    if header.type == TelemetryPacketType.TAGS:
                        packet: TelemetryTags

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

                except TimeoutError:
                    timeouts_in_row += 1
                    if timeouts_in_row > 3:
                        break

                except ConnectionResetError as e:
                    print('Connection reset by peer, closing socket')
                    break

                except Exception as e:
                    # No data available
                    print(f'EXCEPTION: {e}')
                
    def _parse_packet(self, header: TelemetryPacketHeader, payload: bytes) -> TelemetryPacket:
        try:
            if header.type == TelemetryPacketType.STATUS:
                packet = TelemetryStatus(*struct.unpack(TelemetryStatus._fmt, payload))
                #print(f'{packet.to_json()}')
            elif header.type == TelemetryPacketType.TAGS:
                # 1. Header
                nbr_of_tags = payload[0]
                has_lock = payload[1]
                
                # 2. Locked Tag
                index_start = 2
                index_end = 2 + Tag.size()
                locked_tag = Tag.from_bytes(payload[index_start:index_end])

                # 3. Landing target
                index_start = index_end
                index_end += LandingTarget.size()
                landing_target = LandingTarget(*struct.unpack(LandingTarget._fmt, payload[index_start:index_end]))

                # 4. All tags (if available)
                packet = TelemetryTags(has_lock, locked_tag, landing_target, [])
                #print(f'TAGS: {nbr_of_tags} ({packet_len} bytes): {payload}')
                for tag in range(nbr_of_tags):
                    #start_index = 2 + Tag.size() + (tag*Tag.size())
                    #end_index = start_index + Tag.size()
                    index_start = index_end
                    index_end = index_start + Tag.size()
                    tag = Tag.from_bytes(payload[index_start:index_end])
                    packet.tags.append(tag)

            elif header.type == TelemetryPacketType.LOG:
                # We'll parse the header automatically, but msg by hand, as length varies
                header_len = struct.calcsize(TelemetryLog._fmt)
                log = TelemetryLog(*struct.unpack(TelemetryLog._fmt, payload[:header_len]))
                log.msg = payload[header_len:].decode('ascii').rstrip('\x00')
                packet = log
                print(f'[{log.level}, {log.group}] ({len(self._queue.queue)}) -> {log.msg.strip()}')

            return packet

        except struct.error as e:
            print(f'Failed to parse packet for {header.type} ({payload})')

    def _parse_header(self, header_raw: bytearray) -> TelemetryPacketHeader:
        # First 5 bytes are header
        # Byte 0-1 is magic (0x1234)
        # Byte 2 is packet type
        # Byte 3-4 is packet length

        if len(header_raw) != 5:
            return None

        try:
            magic, packet_type_raw, packet_len = struct.unpack('<HBH', header_raw)
            if magic != 0x1234:
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


if __name__ == '__main__':
    ip = '192.168.0.202'
    port = 9096
    server = TelemetryServer()
    server.start(ip, port)
