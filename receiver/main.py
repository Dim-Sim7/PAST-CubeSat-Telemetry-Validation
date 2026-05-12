

from time import time
from telemetry_receiver import log
from receiver.telemetry_packet import TelemetryPacket
from receiver.telemetry_receiver import TelemetryReceiver
import config
from payload_processing import *



# here I can look at the type of data the packet is, extract the data and then store it in the appropriate file
def handle_packet(pkt: TelemetryPacket):
    """
    Main callback registered with rx.on_packet()
    Routes each packet type to its handler which unpacks and writes to CSV
    """
    if pkt.type == config.PACKET_TYPE_GNSS:
        handle_gnss(pkt)
    elif pkt.type == config.PACKET_TYPE_BARO:
        handle_baro(pkt)
    elif pkt.type == config.PACKET_TYPE_IMU:
        handle_imu(pkt)
    elif pkt.type == config.PACKET_TYPE_BATTERY:
        handle_battery(pkt)
    elif pkt.type == config.PACKET_TYPE_RS_META:
        pass   # handled internally by TelemetryReceiver
    else:
        log.warning(f"Unknown packet type: {pkt.type:#04x} seq={pkt.seq}")

def handle_block(session_seq: int, packet_type: int, data: bytes):
    if packet_type == config.PACKET_TYPE_IMAGE:
        #save image to file 
        path = f"image_{session_seq}.jpg"
        with open(path, "wb") as f:
            f.write(data)
        log.info(f"Image saved: {path} ({len(data)} bytes)")
    
if __name__ == "__main__":
    with TelemetryReceiver(port=config.SERIAL_PORT, comm_params=config.SENSOR_PARAMS) as rx:
        rx.on_packet(handle_packet) # store completed packets / blocks on disk and display data
        rx.on_block_complete(handle_block)
        rx.run_forever()
    