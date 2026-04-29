# https://realpython.com/python3-object-oriented-programming/ brushed up on my python briefly before starting
""" 
TELEMETRY_SOF 0xAA
TELEMETRY_EOF 0x55

POLY 0x1021 /* for crc-ccitt mask */

RELIABLE    0x00
UNRELIABLE  0x01

PACKET_TYPE_GNSS       0x01
PACKET_TYPE_BARO       0x02
PACKET_TYPE_IMU        0x03
PACKET_TYPE_BATTERY    0x04
PACKET_TYPE_IMAGE      0x05
PACKET_TYPE_RETRANSMIT 0x06

STM32 Telemetry UART Receiver
==============================
Self-contained — no external dependencies except pyserial.
 
Two receive paths:
  frag_total == 1  →  CRC-only packet, dispatch immediately via on_packet()
  frag_total  > 1  →  RS FEC block, accumulate shards, decode via on_block_complete()
 
Packet wire layout (TelemetryPacket_t __packed, little-endian):
  Offset  Field        Type      Size
  ------  -----------  --------  ----
  0       sof          uint8_t   1
  1       type         uint8_t   1
  2       seq          uint32_t  4
  6       block_id     uint16_t  2
  8       frag_index   uint8_t   1
  9       frag_total   uint16_t  2
  11      reliable     uint8_t   1
  12      len          uint8_t   1
  13      payload      uint8_t[] MAX_PAYLOAD_SIZE  (always full size on wire)
  13+N    crc          uint16_t  2
  15+N    eof          uint8_t   1

"""

""" 
    CONFIGURATION
"""
from enum import IntEnum
import struct

import serial

SERIAL_PORT     = 'dev/ttyUSB0'
BAUD_RATE       = 115200
BYTE_SIZE       = serial.EIGHTBITS
STOP_BITS       = serial.STOPBITS_ONE
PARITY          = serial.PARITY_NONE

SOF_BYTE        = 0xAA
EOF_BYTE        = 0x55
MAX_PAYLOAD_SIZE = 32
CRC_POLY        = 0x1021
CRC_INIT        = 0xFFFF 

class PacketType(IntEnum):
    GNSS       = 0x01
    BARO       = 0x02
    IMU        = 0x03
    BATTERY    = 0x04
    IMAGE      = 0x05
    RETRANSMIT = 0x06
    UNKNOWN    = 0xFF
    
RS_DATA_SHARDS   = 4
RS_PARITY_SHARDS = 2

# Struct layout 
# https://www.digitalocean.com/community/tutorials/python-struct-pack-unpack
# https://docs.python.org/3/library/struct.html
# < for little endian (which is what the TX uses)
HEADER_FMT       = "<BBIHBHBB"
HEADER_SIZE      = struct.calcsize(HEADER_FMT)
TRAILER_FMT      = "<HB" 
TRAILER_SIZE     = struct.calcsize(TRAILER_FMT)
FULL_PACKET_SIZE = HEADER_SIZE + MAX_PAYLOAD_SIZE + TRAILER_SIZE
    
SENSOR_PARAMS = {
    'baudrate': BAUD_RATE,
    'stopbits': STOP_BITS,
    'parity': PARITY,
    'bytesize': BYTE_SIZE,
}