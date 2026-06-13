# PAST CubeSat Telemetry Validation

This project validates the telemetry communication link for a CubeSat system. It consists of two parts: firmware running on an STM32L4 microcontroller that encodes and transmits telemetry packets over UART, and a Python receiver on the host that parses the incoming stream, checks packet integrity, and logs the results.

The goal is to confirm the receiver handles both normal traffic and fault conditions correctly before the firmware is finalised for flight.

## How it works

The transmitter encodes telemetry into framed packets and sends them serially. Each packet carries a type, sequence number, payload, and an integrity check — either a CRC for simple packets or Reed-Solomon FEC for larger, fragmented payloads. The receiver syncs to the stream, parses each packet, verifies the checksum or reconstructs the RS shards, and logs everything to `telemetry_receiver.log`.

## Building the firmware

You'll need the `arm-none-eabi-gcc` toolchain, CMake, and OpenOCD.

```bash
cmake -B build/Debug
cmake --build build/Debug --clean-first
```

To flash via ST-LINK:

```bash
openocd -f interface/stlink.cfg -f target/stm32l4x.cfg
arm-none-eabi-gdb build/Debug/telemetry_transmitter.elf
```

Or using STM32CubeProgrammer directly:

```bash
STM32_Programmer_CLI -c port=ST-LINK -w build/Debug/telemetry_transmitter.elf -v -rst
```

## Running the receiver

```bash
cd receiver
python receiver.py
```

Events are written to `telemetry_receiver.log` in the project root.
