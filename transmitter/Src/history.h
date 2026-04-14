#ifndef HISTORY_H
#define HISTORY_H

#include "ring_buffer.h"

/* Amount of packets that can be held (in RingBufferEntry form)
   MUST be a power of 2 */
#define HISTORY_BUFFER_SIZE 128

#define HISTORY_FOUND 1
#define HISTORY_NOT_FOUND 0
void addHistory(TelemetryPacket_t* packet);
void addHistoryRaw(RingBufferEntry* entry);
int getFromHistory(uint32_t seq, RingBufferEntry* out);

#endif