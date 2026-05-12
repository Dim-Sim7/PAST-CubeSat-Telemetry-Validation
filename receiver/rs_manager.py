
from typing import Dict, Optional

from receiver import config
from receiver.rs_group import RSGroup, TransmissionMeta
from receiver.telemetry_packet import TelemetryPacket
from receiver.telemetry_receiver import log
from receiver.rs_c_wrapper import ReedSolomon


class RSManager:
    def __init__(self, data_shards: int, parity_shards: int):
        self._rs = ReedSolomon(data_shards, parity_shards)
        
        self._meta : Dict[int, TransmissionMeta] = {}
        self._groups: Dict[int, RSGroup] = {}
        
        self._data_shards = data_shards
        self._parity_shards = parity_shards
        
    def add_meta(self, pkt: TelemetryPacket):
        try:
            meta = TransmissionMeta.from_packet(pkt)
            self._meta[pkt.seq] = meta
            log.info(
                f"Stored RS meta: seq={pkt.seq} "
                f"size={meta.total_size}"
            )
        except ValueError as e:
            log.error(f"Bad RS meta packet: {e}")
            
    def add_shard(self, pkt: TelemetryPacket) -> Optional[tuple[int, int, bytes]]:
        ptype = pkt.type
        
        # Create RSGroup when first shard arrives
        if ptype not in self._groups:

            meta = self._meta.pop(pkt.seq, None)

            if meta is None:
                log.warning(
                    f"No RS meta for type={ptype} "
                    f"seq={pkt.seq}"
                )
                return None

            self._groups[ptype] = RSGroup(
                meta=meta,
                session_seq=pkt.seq,
                payload_type=ptype,
                data_shards=self._data_shards,
                parity_shards=self._parity_shards,
                shard_size=config.RS_SHARD_SIZE,
            )

        group = self._groups[ptype]

        result = group.feed_shard(pkt, self._rs)

        if result is None:
            return None

        del self._groups[ptype]

        return (
            group.session_seq,
            ptype,
            result,
        )
        
            