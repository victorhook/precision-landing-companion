from threading import Thread
import socket
from enum import IntEnum
from dataclasses import dataclass, asdict
import struct
from queue import Queue, Empty

from telemetry_def import TelemetryPacket, TelemetryStatus


class TelemetryPacketType(IntEnum):
    STATUS = 0x01
    MARKERS_FOUND = 0x02



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
        print(f'Telemetry server Connecting to {ip}:{port}')
        self._socket.connect((ip, port))
        print(f'Telemetry server Connected to {ip}:{port}')

        while True:
            # Byte [0] we'll read the first byte which is the packet type
            packet_type_raw = self._socket.recv(1)

            try:
                packet_type = TelemetryPacketType(ord(packet_type_raw))
            except ValueError:
                print(f'Invalid packet type 0x{hex(ord(packet_type_raw))[2:].zfill(2)}')
                continue

            # Byte [1-2] is packet length
            packet_len_raw = self._socket.recv(2)
            packet_len = struct.unpack('<H', packet_len_raw)[0]

            if packet_len > 10000:
                print('Packet length above 3000, probably wrong...!')
                continue

            # Read payload data
            payload = self._socket.recv(packet_len)

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