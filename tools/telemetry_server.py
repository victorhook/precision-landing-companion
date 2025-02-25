from threading import Thread
import socket
from enum import IntEnum
from dataclasses import dataclass, asdict
import struct
from queue import Queue, Empty, Full
import time
import typing as t

from telemetry_def import TelemetryPacket, TelemetryStatus


class TelemetryPacketType(IntEnum):
    STATUS = 0x01
    TAGS = 0x02
    LOG = 0x03

@dataclass
class TelemetryLog(TelemetryPacket):
    level: int
    timestamp: int
    msg: str = ''

    _fmt = '<BI'



@dataclass
class Point2F:
    x: float
    y: float

    _fmt = '<ff'

@dataclass
class TagPosition:
    center: Point2F
    p1: Point2F
    p2: Point2F
    p3: Point2F
    p4: Point2F

    @classmethod
    def size(cls) -> int:
        return 5 * struct.calcsize(Point2F._fmt)
    
    @classmethod
    def from_bytes(cls, raw: bytes) -> 'TagPosition':
        fmt = '<' + (5 * Point2F._fmt).replace('<', '')
        floats = struct.unpack(fmt, raw)
        
        tag = TagPosition(
            center=Point2F(floats[0], floats[1]),
            p1=Point2F(floats[2], floats[3]),
            p2=Point2F(floats[4], floats[5]),
            p3=Point2F(floats[6], floats[7]),
            p4=Point2F(floats[8], floats[9])
        )

        return tag

@dataclass
class TelemetryTags(TelemetryPacket):
    tags: t.List[TagPosition]

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
        
    def _connect(self, ip: str, port: int) -> bool:
        try:    
            self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._socket.settimeout(3)
            self._socket.setblocking(True)
            self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            self._socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, 5)  # 5 sec idle before keep-alive probe
            self._socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, 1)  # 1 sec between probes
            self._socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT, 3)  # 3 failed probes = disconnect
            self._socket.connect((ip, port))
            return True
        except (ConnectionRefusedError, OSError) as e:
            print(f'Failed to open connection.. {e}')
            return False

    def start(self, ip: str, port: int) -> None:
        print(f'Telemetry server starting, reaching client at {ip}:{port}')

        while True:
            if not self._connect(ip, port):
                time.sleep(1)
                continue

            print(f'Telemetry server Connected to {ip}:{port}')

            while True:
                timed_out = False

                # First 3 bytes is header
                # Byte [0] we'll read the first byte which is the packet type
                # Byte [1-2] is packet length
                try:
                    header = self._socket.recv(3)
                    if not header:  # ✅ If recv() returns 0, connection is closed
                        timed_out = True
                except (socket.timeout, OSError):
                    timed_out = True
                    
                if timed_out:
                    self._timed_out()
                    break

                try:
                    packet_type_raw = header[0]
                    packet_type = TelemetryPacketType(packet_type_raw)
                except ValueError:
                    print(f'Invalid packet type 0x{hex(ord(packet_type_raw))[2:].zfill(2)}')
                    continue

                try:
                    packet_len_raw = header[1:]
                    packet_len = struct.unpack('<H', packet_len_raw)[0]
                except Exception as e:
                    print(f'Failed to unpack packet length... {e}')
                    break

                if packet_len > 10000:
                    print('Packet length above 3000, probably wrong...!')
                    continue

                # Read payload data, if needed
                if packet_len > 0:
                    try:
                        payload = self._socket.recv(packet_len)
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

                if packet_type == TelemetryPacketType.STATUS:
                    packet = TelemetryStatus(*struct.unpack(TelemetryStatus._fmt, payload))
                    #print(f'{packet.to_json()}')
                elif packet_type == TelemetryPacketType.TAGS:
                    nbr_of_tags = payload[0]
                    packet = TelemetryTags([])
                    #print(f'TAGS: {nbr_of_tags} ({packet_len} bytes): {payload}')
                    for tag in range(nbr_of_tags):
                        start_index = 1 + (tag*TagPosition.size())
                        end_index = start_index + TagPosition.size()
                        tag = TagPosition.from_bytes(payload[start_index:end_index])
                        packet.tags.append(tag)
                elif packet_type == TelemetryPacketType.LOG:
                    # We'll parse the header automatically, but msg by hand, as length varies
                    header_len = struct.calcsize(TelemetryLog._fmt)
                    log = TelemetryLog(*struct.unpack(TelemetryLog._fmt, payload[:header_len]))
                    log.msg = payload[header_len:].decode('ascii').rstrip('\x00')
                    packet = log

                if not packet:
                    print('Unable to parse packet...?')
                    continue

                # Call waiting subscribers
                #print(packet_type, TelemetryPacketType.LOG, packet_type == TelemetryPacketType.LOG)
                for subscriber in self._subscribers.get(packet_type, []):
                    subscriber.handle_telemetry_packet(packet)

                # Put packet to receiving queue
                try:
                    self._queue.put_nowait(packet)
                except Full:
                    # If the queue is full, we'll just discard the packets
                    self._discarded_packets += 1

    def _timed_out(self) -> None:
        print('Timed out reading data, assuming client disconnected, closing socket and re-trying to open')
        self._socket.close()


if __name__ == '__main__':
    ip = '192.168.0.202'
    port = 9096
    TelemetryServer().start(ip, port)