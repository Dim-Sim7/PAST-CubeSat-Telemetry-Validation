cmake -B build/Debug
cmake --build build/Debug --clean-first


TO DO

finish history.c
find out how to receive NACKs and send out packets from history
figure out how to split up large payloads into multiple packets
    can maybe send a particular SOF like 0xAB which indicates data spread amongst multiple packets
    need a counter of how many packets, start sequence of packet and end sequence packet. can send a telemetry packet with payload of sequence numbers
    find resources online, i think i had some notes on this in my notebook

Implement forward error correction
TMR -> uartTxBusy, cur_seq, dropped_packets -> ring buffer head and tail pointers
Implement TMR_Vote_Size -> setters and getters

After these, the transmitter is finished. Can maybe add some debugging tools