import struct
import csv
import os
from datetime import datetime
from telemetry_packet import TelemetryPacket
from telemetry_receiver import log

# GNSSData_t:     float time, double lat, double lon, float alt
GNSS_FMT  = "<fddf"
# BarometerData_t: uint32 pressure, int16 temperature  
BARO_FMT  = "<Ih"
# IMUData_t:       9x int16
IMU_FMT   = "<9h"
# BatteryData_t:   uint8 hours, uint8 minutes, uint8 percent, char[8] status
BATT_FMT  = "<BBB8s"

LOG_DIR = "logs"
os.makedirs(LOG_DIR, exist_ok=True)

GNSS_FILE = os.path.join(LOG_DIR, "gnss.csv")
BARO_FILE = os.path.join(LOG_DIR, "baro.csv")
IMU_FILE = os.path.join(LOG_DIR, "imu.csv")
BATT_FILE = os.path.join(LOG_DIR, "battery.csv")

# make a writer for each data type 

_writers = {}
_csv_files = {}
def _get_writer(path: str, headers: list[str]) -> csv.DictWriter:
    if path not in _writers:
        file_exists = os.path.exists(path)
        
        f = open(path, "a", newline="")
        writer = csv.DictWriter(f, fieldnames=["timestamp", *headers])
        
        if not file_exists:
            writer.writeheader()
        _csv_files[path] = f
        _writers[path] = writer
    
    return _writers[path]

def _now() -> str:
    return datetime.now().isoformat(timespec="milliseconds")

def handle_gnss(pkt: TelemetryPacket):
    if len(pkt.payload) < struct.calcsize(GNSS_FMT):
        log.warning(f"GNSS payload too short: {len(pkt.payload)}")
        
    timestamp, lat, lon, alt = struct.unpack_from(GNSS_FMT, pkt.payload, 0)
    log.info(f"GNSS seq={pkt.seq} time={timestamp:.3f} lat={lat:.6f} lon={lon:.6f} alt={alt:.2f}m")
    writer = _get_writer(GNSS_FILE, ["seq", "time", "latitude", "longitude", "altitude"])
    writer.writerow({
        "timestamp": _now(),
        "seq":       pkt.seq,
        "time":      timestamp,
        "latitude":  lat,
        "longitude": lon,
        "altitude":  alt,
    })
    _csv_files[GNSS_FILE].flush()

def handle_baro(pkt: TelemetryPacket):
    if len(pkt.payload) < struct.calcsize(BARO_FMT):
        log.warning(f"Baro payload too short: {len(pkt.payload)}")
        return
    pressure, temperature = struct.unpack_from(BARO_FMT, pkt.payload, 0)
    log.info(f"BARO seq={pkt.seq} pressure={pressure}Pa temp={temperature / 100:.2f}°C")
    writer = _get_writer(BARO_FILE, ["seq", "pressure_pa", "temperature_c"])
    writer.writerow({
        "timestamp":     _now(),
        "seq":           pkt.seq,
        "pressure_pa":   pressure,
        "temperature_c": temperature / 100,
    })
    _csv_files[BARO_FILE].flush()

def handle_imu(pkt: TelemetryPacket):
    if len(pkt.payload) < struct.calcsize(IMU_FMT):
        log.warning(f"IMU payload too short: {len(pkt.payload)}")
        return
    ax, ay, az, gx, gy, gz, mx, my, mz = struct.unpack_from(IMU_FMT, pkt.payload, 0)
    log.info(
        f"IMU seq={pkt.seq} "
        f"accel=({ax},{ay},{az}) "
        f"gyro=({gx},{gy},{gz}) "
        f"mag=({mx},{my},{mz})"
    )
    writer = _get_writer(IMU_FILE, [
        "seq",
        "accel_x", "accel_y", "accel_z",
        "gyro_x",  "gyro_y",  "gyro_z",
        "mag_x",   "mag_y",   "mag_z",
    ])
    writer.writerow({
        "timestamp": _now(),
        "seq":       pkt.seq,
        "accel_x":   ax,  "accel_y": ay,  "accel_z": az,
        "gyro_x":    gx,  "gyro_y":  gy,  "gyro_z":  gz,
        "mag_x":     mx,  "mag_y":   my,  "mag_z":   mz,
    })
    _csv_files[IMU_FILE].flush()

def handle_battery(pkt: TelemetryPacket):
    if len(pkt.payload) < struct.calcsize(BATT_FMT):
        log.warning(f"Battery payload too short: {len(pkt.payload)}")
        return
    hours, minutes, percent, status_raw = struct.unpack_from(BATT_FMT, pkt.payload, 0)
    # strip null bytes and convert to python string
    status = status_raw.rstrip(b'\x00').decode('utf-8', errors='replace')
    log.info(
        f"BATTERY seq={pkt.seq} "
        f"time={hours:02d}:{minutes:02d} "
        f"charge={percent}% "
        f"status={status}"
    )
    writer = _get_writer(BATT_FILE, ["seq", "hours", "minutes", "percent", "status"])
    writer.writerow({
        "timestamp": _now(),
        "seq":       pkt.seq,
        "hours":     hours,
        "minutes":   minutes,
        "percent":   percent,
        "status":    status,
    })
    _csv_files[BATT_FILE].flush()