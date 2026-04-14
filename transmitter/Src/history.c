#include "history.h"
#include "ring_buffer.h"

/* Buffer initialisation */
static RingBufferEntry historyEntries[HISTORY_BUFFER_SIZE];
static RingBuffer historyBuffer = {
    .entries = historyEntries,
    .head_a  = 0, .head_b = 0, .head_c = 0,
    .tail_a  = 0, .tail_b = 0, .tail_c = 0,
    .size    = HISTORY_BUFFER_SIZE,
    .mask    = HISTORY_BUFFER_SIZE - 1
};

void addHistory(TelemetryPacket_t* packet) 
{
    enqueue(&historyBuffer, packet);
}

void addHistoryRaw(RingBufferEntry *entry)
{
    enqueueRaw(&historyBuffer, entry);
}

int getFromHistory(uint32_t seq, RingBufferEntry* out)
{
    if (findPacketBySeq(&historyBuffer, seq, out) != 0) {
        /* Cannot find packet */
        return -1;
    }
    return 0;
}