from threading import Thread
import socket
from enum import IntEnum
from dataclasses import dataclass
import struct


class TelemetryPacketType(IntEnum):
    STATUS = 0x01
    MARKERS_FOUND = 0x02


@dataclass
class TelemetryStatus:
    upTimeMs: int
    cameraFps: int

    _fmt = '<IB'


class TelemetryServer:

    def __init__(self) -> None:
        self._socket = socket.socket()

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
                print(f'{status}')
            elif packet_type == TelemetryPacketType.MARKERS_FOUND:
                pass
 

if __name__ == '__main__':
    ip = '192.168.0.202'
    port = 9096
    TelemetryServer().start(ip, port)