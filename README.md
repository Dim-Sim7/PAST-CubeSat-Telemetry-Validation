cmake -B build/Debug
cmake --build build/Debug --clean-first

openocd -f interface/stlink.cfg -f target/stm32l4.cfg

Need to implement ways of simulating faulty data:
    packet data corruption mid-transmission
    dropped packets
    packets arriving out of sync
    unexpected end of transmission from the transmitter
    ghost packets