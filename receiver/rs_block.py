

from typing import List, Optional
from telemetry_receiver import log
import config
"""_summary_

This class represents one RS block that is stored within an RS Group

The STM32 sends TOTAL_SHARDS packets per block, all with the same block_id:
    frag_index 0 .. DATA_SHARDS-1      → data shards
    frag_index DATA_SHARDS .. TOTAL-1  → parity shards

Can reconstruct as long as any DATA_SHARDS shards arrive.
Shards that fail CRC upstream are never fed in — they count as erasures.

"""
class RSBlock:
    def __init__(self, block_id: int, data_shards: int = config.RS_DATA_SHARDS,
                 parity_shards: int = config.RS_PARITY_SHARDS, shard_size: int = config.RS_SHARD_SIZE):
        
        self.block_id = block_id
        self.data_shards = data_shards
        self.parity_shards = parity_shards
        self.total_shards = data_shards + parity_shards
        self.shard_size = shard_size
        
        self.shards: List[Optional[bytearray]] = [None] * self.total_shards #dict of 6 shards
        self.received = 0 # counter to keep track of how many shards is received
        
    def feed(self, frag_index: int, raw_payload: bytes) -> bool:
        """_summary_
        Store a shard from its raw SHARD_SIZE bytes.
        Return true if new and false if out of range or duplicate
        """
        
        # out of range
        if frag_index >= self.total_shards:
            log.warning(f"RSBlock block ={self.block_id}: "
                        f"frag_index {frag_index} out of range "
                        f"(total=(self.total_shards))")
            return False
        
        # duplicate
        if self.shards[frag_index] is not None:
            log.warning(f"RSBlock block ={self.block_id}: "
                        f"frag_index {frag_index} duplicate "
                        f"(total=(self.total_shards))")
            return False
        
        shard = bytearray(self.shard_size)
        n = min(self.shard_size, len(raw_payload)) # capture the minimum of shard size or raw payload
        shard[:n] = raw_payload[:n]
        self.shards[frag_index] = shard
        self.received += 1
        return True
    
    @property
    def can_decode(self) -> bool:
        return self.received >= self.data_shards
    
    @property
    def missing_indices(self) -> List[int]:
        # returns the indices of missing shards
        missing = []
        for i in enumerate(self.shards):
            if i is None:
                missing.append(i)
        return missing
    
    def decode(self, rs) -> bytes:
        shards = [] # raw byte array of data
        marks = [] # 1 = missing, 0 = data
        
        for s in self.shards:
            if s is None:
                # missing data, zero it out
                shards.append(bytearray(self.shard_size))
                marks.append(1)
            else:
                shards.append(bytearray(s))
                marks.append(0)
                

        ok = rs.reconstruct(shards, marks)
        if not ok:
            raise ValueError(
                f"RSBlock block={self.block_id}: RS decode failed — "
                f"received {self.received}/{self.total_shards}, "
                f"need at least {self.data_shards}"
            )
 
        return b"".join(bytes(shards[i]) for i in range(self.data_shards))
