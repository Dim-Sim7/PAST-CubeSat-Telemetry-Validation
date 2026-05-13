
import logging
from typing import Dict, Optional

import config
from rs_group import RSGroup, TransmissionMeta
from telemetry_packet import TelemetryPacket

from rs_c_wrapper import ReedSolomon

log = logging.getLogger(__name__)

class RSManager:
    def __init__(self, data_shards: int, parity_shards: int):
        self._rs = ReedSolomon(data_shards, parity_shards)
        
        # both keyed by group_id
        self._meta : Dict[int, TransmissionMeta] = {}
        self._groups: Dict[int, RSGroup] = {}
        
        self._data_shards = data_shards
        self._parity_shards = parity_shards
        
    def add_meta(self, pkt: TelemetryPacket):
        try:
            meta = TransmissionMeta.from_packet(pkt)
            self._meta[pkt.group_id] = meta
            log.info(
                f"Stored RS meta: group={pkt.group_id} "
                f"size={meta.total_size}"
            )
        except ValueError as e:
            log.error(f"Bad RS meta packet: {e}")
            
    def add_shard(self, pkt: TelemetryPacket) -> Optional[tuple[int, int, bytes]]:
        ptype = pkt.type
        
        # Create RSGroup when first shard arrives
        if pkt.group_id not in self._groups:

            meta = self._meta.get(pkt.group_id)

            if meta is None:
                log.warning(
                    f"No RS meta for group={pkt.group_id} "
                    f"seq={pkt.seq}"
                )
                return None

            self._groups[pkt.group_id] = RSGroup(
                meta=meta,
                session_seq=pkt.seq,
                group_id=pkt.group_id,
                payload_type=ptype,
                data_shards=self._data_shards,
                parity_shards=self._parity_shards,
                shard_size=config.RS_SHARD_SIZE,
            )

        group = self._groups.get(pkt.group_id)
        if not group:
            return None
        
        result = group.feed_shard(pkt, self._rs)

        if result is None:
            return None

        del self._groups[pkt.group_id]

        return (
            group.group_id,
            ptype,
            result,
        )
        
            