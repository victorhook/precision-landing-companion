from threading import Thread
import socket
from enum import IntEnum
from dataclasses import dataclass, asdict
import struct
from queue import Queue, Empty


class TelemetryPacketType(IntEnum):
    STATUS = 0x01
    MARKERS_FOUND = 0x02

@dataclass
class TelemetryPacket:
    pass

    def to_json(self) -> str:
        return {
            'type': self.__class__.__name__,
            **asdict(self)
        }

@dataclass
class TelemetryStatus(TelemetryPacket):
    upTimeMs: int
    cameraFps: int

    _fmt = '<IB'


class TelemetryServer:

    def __init__(self) -> None:
        self._socket = socket.socket()
        self._queue = Queue(500)

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

    def start(self, ip: str, port: int) -> None:
        print(f'Connecting to {ip}:{port}')
        self._socket.connect((ip, port))
        print(f'Connected to {ip}:{port}')

        while True:
            packet_size = 6
            pkt = self._socket.recv(packet_size)
            if len(pkt) != 6:
                print('Malformed package...')
                continue

            packet_type = TelemetryPacketType(int(pkt[0]))
            payload = pkt[1:]

            if packet_type == TelemetryPacketType.STATUS:
                status = TelemetryStatus(*struct.unpack(TelemetryStatus._fmt, payload))
                #print(f'{status.to_json()}')
                # If the queue is full, we'll just discard the packets
                self._queue.put(status, block=False)
            elif packet_type == TelemetryPacketType.MARKERS_FOUND:
                pass
 

if __name__ == '__main__':
    ip = '192.168.0.202'
    port = 9096
    TelemetryServer().start(ip, port)