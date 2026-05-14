# https://docs.python.org/3/library/ctypes.html
# https://realpython.com/python-bindings-overview/#ctypes

import ctypes
so_file = "librs.so"

rs_functions = ctypes.CDLL(so_file)

rs_functions.fec_init.restype = None
rs_functions.fec_init.argtypes = []

#int reed_solomon_encode(reed_solomon* rs,
#        unsigned char** data_blocks,
 #       unsigned char** fec_blocks,
 #       int block_size);
rs_functions.reed_solomon_encode.restype = ctypes.c_int
rs_functions.reed_solomon_encode.argtypes = [
    ctypes.c_void_p,
    ctypes.POINTER(ctypes.c_char_p),
    ctypes.POINTER(ctypes.c_char_p),
    ctypes.c_int,
]


# reed_solomon* reed_solomon_new(int data_shards, int parity_shards);
rs_functions.reed_solomon_new.restype = ctypes.c_void_p #void pointer
rs_functions.reed_solomon_new.argtypes = [ctypes.c_int, ctypes.c_int]
 
# void reed_solomon_release(reed_solomon* rs)
rs_functions.reed_solomon_release.restype  = None
rs_functions.reed_solomon_release.argtypes = [ctypes.c_void_p]

# int reed_solomon_decode(reed_solomon* rs,
#                         unsigned char** data_blocks,
#                         int block_size,
#                         unsigned char** dec_fec_blocks,
#                         unsigned int* fec_block_nos,
#                         unsigned int* erased_blocks,
#                         int nr_fec_blocks)
rs_functions.reed_solomon_decode.restype  = ctypes.c_int
rs_functions.reed_solomon_decode.argtypes = [
    ctypes.c_void_p,                  # reed_solomon* rs
    ctypes.POINTER(ctypes.c_char_p),  # unsigned char** data_blocks
    ctypes.c_int,                     # int block_size
    ctypes.POINTER(ctypes.c_char_p),  # unsigned char** dec_fec_blocks
    ctypes.POINTER(ctypes.c_uint),    # unsigned int* fec_block_nos
    ctypes.POINTER(ctypes.c_uint),    # unsigned int* erased_blocks
    ctypes.c_int,                     # int nr_fec_blocks
]

# int reed_solomon_reconstruct(
#     reed_solomon* rs,
#     unsigned char** shards,
#     unsigned char* marks,
#     int nr_shards,
#     int block_size
# 
rs_functions.reed_solomon_reconstruct.restype = ctypes.c_int
rs_functions.reed_solomon_reconstruct.argtypes = [
    ctypes.c_void_p,                            # reed_solomon* rs
    ctypes.POINTER(ctypes.POINTER(ctypes.c_ubyte)),  # unsigned char** shards
    ctypes.POINTER(ctypes.c_ubyte),             # unsigned char* marks
    ctypes.c_int,                               # nr_shards
    ctypes.c_int,                               # block_size
]

rs_functions.fec_init()


class ReedSolomon:
    """
    Wrapper for reed_solomon_new and reed_solomon_decode
    """
    
    def __init__(self, data_shards: int, parity_shards: int) :
        self.data_shards = data_shards
        self.parity_shards = parity_shards
        self.total_shards = data_shards + parity_shards
        
        self._rs = rs_functions.reed_solomon_new(data_shards, parity_shards)
        if not self._rs:
            raise RuntimeError("reed_solomon_new return NULL")
        
    def __del__(self):
        rs_functions.reed_solomon_release(self._rs)
        self._rs = None
        
    # decodes one block
    def call_rs_decode(self, shards: list, marks:list) -> bool:
        """
        Translates Python shards/marks from rs_block decode_block() into C arrays and calls
        reed_solomon_decode
        
        Writes recovered data back into shards in-place
        
        shards: list of total_shards equal-length bytearrays
        marks: parallel list -- 1 = erased, 0 = present
        """
        ds = self.data_shards
        size = len(shards[0])
        
        # Which data shard indices are erased
        erased  = [i for i, bad in enumerate(marks[:ds]) if bad]

        # Which parity shard indices are available
        p_avail = [i for i, bad in enumerate(marks[ds:]) if not bad]
        
        nr = len(erased)
        
        if not erased:
            return True # nothing to recover
        
        if len(p_avail) < nr:
            return False # not enough parity to cover erasures
        
        # Mutable C buffers for all data shards
        # reed_solomon_decode writes recovered data back through these
        c_data_bufs = []

        for i in range(ds):
            buf = (ctypes.c_char * size)(*shards[i])
            c_data_bufs.append(buf)
            
        data_addresses = [
            ctypes.addressof(buf)
            for buf in c_data_bufs
        ]
        
        data_ptrs = (ctypes.c_void_p * ds)(*data_addresses)

        # Parity shard buffers — one per erasure
        fec_indices = p_avail[:nr]
        c_fec_bufs  = [(ctypes.c_char * size)(*shards[ds + i]) for i in fec_indices]
        fec_ptrs    = (ctypes.c_void_p * nr)(
            *[ctypes.addressof(b) for b in c_fec_bufs]
        )

        # fec_block_nos — position of each parity shard in original parity array
        fec_nos_arr = (ctypes.c_uint * nr)(*fec_indices)

        # erased_blocks — must be sorted (required by reed_solomon_decode in fec.c)
        erased_arr  = (ctypes.c_uint * nr)(*sorted(erased))
        
        ret = rs_functions.reed_solomon_decode(
            self._rs,
            data_ptrs,   # all 4 data shards
            size,        # 32 bytes per shard
            fec_ptrs,    # the parity shards for recovery
            fec_nos_arr, # which parity slots they came from
            erased_arr,  # which data slots are missing
            nr           # how many are missing
        )
        
        # Write recovered shards back into Python list
        for erased_shard in erased:
            shards[erased_shard][:] = bytes(c_data_bufs[erased_shard])
            
        return ret == 0
    
    def reconstruct(self, shards, marks):

        total = self.total_shards
        size  = len(shards[0])

        # Build mutable C buffers
        c_shards = []

        for shard in shards:

            buf = (ctypes.c_ubyte * size).from_buffer(shard)

            c_shards.append(buf)

        # unsigned char**
        ShardPtr = ctypes.POINTER(ctypes.c_ubyte)

        shard_ptrs = (ShardPtr * total)(
            *[
                ctypes.cast(buf, ShardPtr)
                for buf in c_shards
            ]
        )

        # marks array
        marks_arr = (ctypes.c_ubyte * total)(*marks)

        ret = rs_functions.reed_solomon_reconstruct(
            self._rs,
            shard_ptrs,
            marks_arr,
            total,
            size
        )

        return ret == 0