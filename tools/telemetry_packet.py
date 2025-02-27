from dataclasses import dataclass, asdict, fields
import struct

@dataclass
class TelemetryPacket:

    def to_json(self) -> str:
        return {
            'type': self.__class__.__name__.split('Telemetry')[-1],
            **asdict(self)
        }

    def to_bytes(self) -> None:
        attrs = [getattr(self, field.name) for field in fields(self)]
        return struct.pack(self._fmt, *attrs)
    
    _fmt = ''