#include "history.h"
#include "ring_buffer.h"
#include "tmr.h"

/* Buffer initialisation */
static RingBufferEntry historyEntries[HISTORY_BUFFER_SIZE];
TMR_SECTION_B static RingBuffer historyBuffer = {
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
        return HISTORY_NOT_FOUND;
    }
    return HISTORY_FOUND;
}