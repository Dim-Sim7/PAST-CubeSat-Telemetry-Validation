

# PARSE REED-SOLOMON DATA

from time import time

from receiver.telemetry_packet import TelemetryPacket
from receiver.telemetry_receiver import TelemetryReceiver
import config

class RSGroup:
    pass


# here I can look at the type of data the packet is, extract the data and then store it in the appropriate file
def handle_packet(pkt: TelemetryPacket):
    pass

def handle_block(block_id: int, data: bytes):
    pass
    
    
if __name__ == "__main__":
    with TelemetryReceiver(port=config.SERIAL_PORT, comm_params=config.SENSOR_PARAMS) as rx:
        rx.on_packet(handle_packet) # store completed packets / blocks on disk and display data
        rx.on_block_complete(handle_block)
        rx.run_forever()
    