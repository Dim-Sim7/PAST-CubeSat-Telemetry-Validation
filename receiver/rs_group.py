from typing import Dict, List, Optional
import struct
import logging
import time


import config
from rs_block import RSBlock
from telemetry_packet import TelemetryPacket

# Meta packet payload — just uint32_t total_size
# Matches: uint32_t total_size passed to createPacket()
META_FMT  = "<I"
META_SIZE = struct.calcsize(META_FMT)   # 4 bytes


log = logging.getLogger(__name__)

class TransmissionMeta:
    """
    Metadata for an upcoming FEC transmission.
    Parsed from a PACKET_TYPE_RS_META single CRC packet.
    Only carries total_size — nr_blocks is derived from it.
    """
 
    def __init__(self, total_size: int):
        self.total_size = total_size # total size of entire RS payload

    @classmethod
    def from_packet(cls, pkt : TelemetryPacket) -> "TransmissionMeta":
        """Parse from a PACKET_TYPE_RS_META TelemetryPacket."""
        if len(pkt.payload) < META_SIZE:
            raise ValueError(
                f"RS meta packet too short: "
                f"{len(pkt.payload)} < {META_SIZE}"
            )
        total_size, = struct.unpack_from(META_FMT, pkt.payload, 0)
        return cls(total_size)
 
    def __str__(self):
        return f"TransmissionMeta(total_size={self.total_size})"


class RSGroup:
    
    """
    Owns all RSBlocks for one large payload transmission
    
    Created when PACKET_TYPE_RS_META arrives
    
    nr_blocks is derived from this packet payload (total_size of large payload)
    
    Only one type of large payload can be accumulated (i,e IMAGE, DATA SNAPSHOT)
    """
    
    def __init__(self, meta: TransmissionMeta, session_seq: int, payload_type: int, group_id: int,
                 data_shards:   int = config.RS_DATA_SHARDS,
                 parity_shards: int = config.RS_PARITY_SHARDS,
                 shard_size:    int = config.RS_SHARD_SIZE):
        
        self.session_seq = session_seq
        self.payload_type = payload_type
        self.group_id = group_id
        self.total_size = meta.total_size
        self.data_shards = data_shards
        self.parity_shards = parity_shards
        self.shard_size = shard_size
        
        self.created = time.monotonic()
        self.last_update = self.created
        # Derive nr_blocks - same as processFragmentData() in C transmitter
        # data_per_block = DATA_SHARDS * BLOCK_SIZE
        data_per_block = data_shards * shard_size
        self.nr_blocks = (meta.total_size + data_per_block - 1) // data_per_block
        
        # Active RSBlocks that are accumulating shards, keyed by block_id
        self._blocks: Dict[int, RSBlock] = {}
        
        # Fully decoded blocks
        self._decoded_blocks: Dict[int, bytes] = {}
        
        log.info(
            f"RSGroup created: type={payload_type:#04x} "
            f"seq={session_seq} "
            f"total_size={self.total_size} "
            f"nr_blocks={self.nr_blocks}"
        )
        
    def feed_shard(self, pkt : TelemetryPacket , rs) -> Optional[bytes]:
        """
        Feed a shard packet into the correct RSBlock
        
        pkt : TelemetryPacket
        rs : ReedSolomon instance
        """
        self.last_update = time.monotonic()
        block_id = pkt.block_id
        
        if block_id >= self.nr_blocks:
            return None
        
        # Create RSBlock on first shard for this block
        if block_id not in self._blocks:
            self._blocks[block_id] = RSBlock(
                block_id= block_id,
                data_shards= self.data_shards,
                parity_shards= self.parity_shards,
                shard_size= self.shard_size
            )
        
        block = self._blocks[block_id]
        
        #Extract raw bytes from packet -- always SHARD_SIZE bytes
        start = config.HEADER_SIZE
        end = start + self.shard_size
        raw_shard = pkt.raw[start:end]
        block.feed(pkt.frag_index, raw_shard)
        
        if not block.can_decode:
            return None
        
        # Block has enough shards -- attemp to decode
        try:
            data = block.decode_block(rs)
            self._decoded_blocks[block_id] = data
            log.info(
                f"RSGroup seq={self.session_seq} "
                f"block={block_id} decoded: {len(data)} bytes "
                f"({len(self._decoded_blocks)}/{self.nr_blocks} blocks done)"
            )
        except ValueError as e:
            log.error(str(e))
            return None
        finally:
            del self._blocks[block_id]
            
        if self.complete:
            return self.reassemble()
        return None

    @property
    def complete(self) -> bool:
        return len(self._decoded_blocks) == self.nr_blocks
    
    @property
    def age(self) -> float:
        return time.monotonic() - self.created

    @property
    def idle(self) -> float:
        return time.monotonic() - self.last_update
    
    def reassemble(self) -> bytes:
        """
        Assemble all decoded blocks in order
        total_size tells us where real data ends in last block
        """
        raw = b"".join(self._decoded_blocks[i] for i in range(self.nr_blocks))
        return raw[:self.total_size]