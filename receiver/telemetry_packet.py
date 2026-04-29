# TELEMETRY PACKET DATA CLASS
from dataclasses import dataclass, field

import config

    
@dataclass
class TelemetryPacket:
    sof:        int
    type:       int
    seq:        int
    block_id:   int
    frag_index: int
    frag_total: int
    reliable:   int
    len:        int
    payload:    bytes    # trimmed to actual len bytes
    crc_rx:     int      # CRC value received in the packet
    crc_calc:   int      # CRC value we computed from the data
    eof:        int
    raw:        bytes = field(repr=False, default=b"")

    @property
    def crc_ok(self) -> bool:
        return self.crc_rx == self.crc_calc
    
    @property
    def framing_ok(self) -> bool:
        return self.sof == config.SOF_BYTE and self.eof == config.EOF_BYTE
    
    @property
    def is_fragmented(self) -> bool:
        return self.frag_total > 0
    
    @property
    def packet_type(self) -> config.PacketType:
        try:
            return config.PacketType(self.type)
        except ValueError:
            return config.PacketType.UNKNOWN

 
    def is_valid(self) -> bool:
        return self.framing_ok and self.crc_ok and (0 <= self.len <= config.MAX_PAYLOAD_SIZE)
    
    def __str__(self):
        path    = "FEC" if self.is_fragmented else "CRC"
        crc_str = "OK" if self.crc_ok else \
                  f"FAIL(rx={self.crc_rx:#06x} calc={self.crc_calc:#06x})"
        return (
            f"Pkt[{path} type={self.packet_type.name} seq={self.seq} "
            f"block={self.block_id} frag={self.frag_index}/{self.frag_total} "
            f"reliable={self.reliable} len={self.len} CRC={crc_str} "
            f"framing={'OK' if self.framing_ok else 'FAIL'}]"
        )