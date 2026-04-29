# TELEMETRY_RECEIVER CLASS
""" 
UART receiver with automatic routing between single packet parsing and RS block paths
"""
"""
 Short article on pySerial and using it for embedded comms. Goes over basic set up and usage
 https://alknemeyer.github.io/embedded-comms-with-python/
 https://alknemeyer.github.io/embedded-comms-with-python-part-2/
"""

import logging
import struct
import time
from typing import Callable, Optional

from pyparsing import Dict
import serial

from receiver import config
from receiver.telemetry_packet import TelemetryPacket

# Error Logging 
# https://docs.python.org/3/library/logging.html
# https://dev.to/koladev/error-handling-and-logging-in-python-mi1
logging.basicConfig(
    filename='telemetry_receiver.log',
    level=logging.DEBUG,
    format="%(asctime)s [%(levelname)s] %(message)s"
)
log = logging.getLogger(__name__)

class TelemetryReceiver:
    def __init__(self, port: str, comm_params: dict,
                 rs_data_shards:   int = config.RS_DATA_SHARDS,
                 rs_parity_shards: int = config.RS_PARITY_SHARDS):
        
        self._port        = port
        self._comm_params = comm_params
        self._ser         = None
        self._buf         = bytearray()

        #need to import this code
        self._rs = ReedSolomonNew(rs_data_shards, rs_parity_shards)
        self._rs_data_shards   = rs_data_shards
        self._rs_parity_shards = rs_parity_shards
        
        self._rs_groups: Dict[int, ParseReedSolomon] = {}
        
        self._packet_cb: Optional[Callable[[TelemetryPacket], None]] = None
        self._block_complete_cb: Optional[Callable[[int, bytes], None]] = None

    # register handler for crc only packets
    def on_packet(self, cb: Callable[[TelemetryPacket], None]):
        self._packet_cb = cb
 
    # register handler for RS decoded blocks
    def on_block_complete(self, cb: Callable[[int, bytes], None]):
        self._block_complete_cb = cb

    def connect(self):
        self._ser = serial.Serial(
            port=self._port, 
            **self._comm_params, 
            timeout=1.0
        )
        log.info(
            f"Connected: {self._port} "
            f"@ {self._comm_params.get('baudrate', '?')} baud  "
            f"packet={config.FULL_PACKET_SIZE}B  "
            f"RS={self._rs_data_shards}+{self._rs_parity_shards}"
        )
        
    def disconnect(self):
        if self._ser and self._ser.is_open:
            self._ser.close()
            log.info("Serial port closed")
    
    #https://stackoverflow.com/questions/1984325/explaining-pythons-enter-and-exit/38685520#38685520
    
    def __enter__(self):
        self.connect()
        return self
    
    def __exit__(self, *args):
        self.disconnect()
    
    def __del__(self):
        self.disconnect()


    def _sync(self):
        """ 
        Scan forward until SOF_BYTE is at position 0
        """
        idx = self._buf.find(config.SOF_BYTE)
        if idx < 0:
            self._buf.clear()
        elif idx > 0:
            log.debug(f"Sync: discarding {idx} bytes before SOF")
            del self._buf[:idx]

    def _process_buffer(self):
        while True:
            self._sync()
            if len(self._buf) < config.FULL_PACKET_SIZE:
                break # not enough bytes yet
            
            # consume FULL_PACKET_SIZE bytes from buffer
            raw = bytes(self._buf[:config.FULL_PACKET_SIZE])
            del self._buf[:config.FULL_PACKET_SIZE] # remove consumed packets from buffer
            
            pkt = parse_packet(raw)
            
            if pkt is None:
                continue
            
            # Framing check
            if not pkt.framing_ok:
                log.debug("Framing error: discarding packet")
                continue
            
            # Crc check
            if not pkt.crc_ok:
                log.debug(f"CRC not matching: discarding packet {pkt.seq}")
                continue
            
            log.debug(str(pkt))
            
            # Route
            if not pkt.is_fragmented:
                self._handle_single(pkt)
            else:
                self._handle_rs_shard(pkt)
                
    def _handle_single(self, pkt: TelemetryPacket):
        log.info("f:Single: {pkt}")
        if self._packet_cb:
            try:
                self._packet_cb(pkt)
            except Exception as e:
                log.error(f"on_packet callback raised:{e}")
    
    def _handle_rs_shard(self, pkt: TelemetryPacket):
        block_id = pkt.block_id
        
        if block_id not in self._rs_groups:
            self._rs_groups[block_id] = RSGroup

    # I read this to understand how to actually get data from the serial stream
    # This is the main loop to read from the connected serial stream
    # https://www.pyserial.org/docs/reading-data
    def run_forever(self):
        self.connect()
        log.info("Listening… (Ctrl-C to stop)")
        try:
            while True:
                if self._ser is None:
                    raise RuntimeError("Serial port not connected")
                n = self._ser.in_waiting
                if n:
                    self._buf.extend(self._ser.read(n))
                    self._process_buffer()
                else:
                    time.sleep(0.001)
        except KeyboardInterrupt:
            log.info("Stopped by user")
            

""" 
Parse exactly FULL_PACKET_SIZE bytes into a TelemetryPacket
Returns None on errors (wrong size, unpack failure)
Must still check if returned packet is not corrupted

https://medium.com/vicara-hardware-university/a-guide-to-transmitting-structures-using-stm32-uart-and-python-56b67f806566
"""
def parse_packet(raw: bytes) -> Optional[TelemetryPacket]:
    if len(raw) != config.FULL_PACKET_SIZE:
        log.error(f"parse_packet: bad size {len(raw)} (expected {config.FULL_PACKET_SIZE})")
        return None
    
    # try make the Telemetry Packet header
    try:
        sof, ptype, seq, block_id, frag_index, frag_total, reliable, plen = \
                struct.unpack_from(config.HEADER_FMT, raw, 0)
    except struct.error as e:
        log.error(f"parse_packet: header unpack failed: {e}")
        return None
    
    # extract the raw full 32bit payload from start = HEADER_SIZE to end =  HEADER_SIZE + MAX_PAYLOAD_SIZE
    payload_raw = raw[config.HEADER_SIZE: config.HEADER_SIZE + config.MAX_PAYLOAD_SIZE]
    
    # take out actual used payload from 0 to plen
    payload = payload_raw[:plen]
    
    # try make the Telemetry Packet trailer
    try:
        crc_rx, eof = struct.unpack_from(config.TRAILER_FMT, raw, config.HEADER_SIZE +  config.MAX_PAYLOAD_SIZE)
    except struct.error as  e:
        log.error(f"parse_packet: trailer unpack: {e}")
        return None
    
    crc_calc = calculate_crc(raw[:config.HEADER_SIZE + config.MAX_PAYLOAD_SIZE])
    
    return TelemetryPacket(
        sof=sof, type=ptype, seq=seq, block_id=block_id,
        frag_index=frag_index, frag_total=frag_total,
        reliable=reliable, len=plen, payload=payload,
        crc_rx=crc_rx, crc_calc=crc_calc, eof=eof, raw=raw
    )

# CRC calc translated from the C transmitter code
# https://www.reddit.com/r/learnpython/comments/17icb1j/declaring_an_unsigned_integer_16_in_python_for/ <-- how to mimic unsigned ints for python
def calculate_crc(data: bytes, poly: int = config.CRC_POLY, init: int = config.CRC_INIT) -> int:
    crc = init
    for byte in data:
        v = 0x80
        for bit in range(8):
            xor_flag = (crc & 0x8000) != 0
            crc = (crc << 1) & 0xFFFF      # keep to 16 bits

            if byte & v:
                crc += 1

            if xor_flag:
                crc ^= poly

            v >>= 1
    return crc