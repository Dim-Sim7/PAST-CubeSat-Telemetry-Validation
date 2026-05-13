cmake -B build/Debug
cmake --build build/Debug --clean-first

openocd -f interface/stlink.cfg -f target/stm32l4x.cfg
arm-none-eabi-gdb build/Debug/telemetry_transmitter.elf

STM32_Programmer_CLI -c port=ST-LINK -w build/Debug/telemetry_transmitter.elf -v -rst

Need to implement ways of simulating faulty data:
    packet data corruption mid-transmission
    dropped packets
    packets arriving out of sync
    unexpected end of transmission from the transmitter
    ghost packets