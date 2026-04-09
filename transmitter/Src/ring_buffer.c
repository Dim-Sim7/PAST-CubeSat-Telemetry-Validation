#include "ring_buffer.h"
#include "telemetry.h"

inline int RingBuffer_Full(RingBuffer* b) 
{
    return ((uint16_t)(b->head - b->tail) == b->size);
}
inline int RingBuffer_Empty(RingBuffer* b) 
{
    return (b->head == b->tail);
}

int findPacketBySeq(RingBuffer* buffer, uint32_t seq, RingBufferEntry* out) 
{
    uint16_t count = (uint16_t)(buffer->head - buffer->tail);
    for (int i = 0; i < count; i++) 
    {
        // walk backwards from head through valid entries
        uint16_t idx = (buffer->head - 1 - i) & buffer->mask;
        if (buffer->entries[idx].seq == seq) 
        {
            memcpy(out, &buffer->entries[idx], sizeof(RingBufferEntry));
            return 0;
        }
    }
    return -1;
}

int dequeue(RingBuffer* buffer, RingBufferEntry* out) 
{
    if (RingBuffer_Empty(buffer)) return -1;

    uint16_t idx = buffer->tail & buffer->mask; // bit-wise wrap around. Much faster than modulus
    memcpy(out, &buffer->entries[idx], sizeof(RingBufferEntry));
    buffer->tail++; // increment only after data is copied
    return 0;
}

void enqueue(RingBuffer* buffer, TelemetryPacket_t* packet) 
{
    if (RingBuffer_Full(buffer)) buffer->tail++; // drop oldest packet in buffer

    uint16_t idx = buffer->head & buffer->mask;
    RingBufferEntry* entry = &buffer->entries[idx];
    entry->seq = packet->seq;
    entry->size = sizeof(TelemetryPacket_t) - MAX_PAYLOAD_SIZE + packet->len;
    memcpy(entry->data, packet, entry->size);
    buffer->head++;
}

void enqueueRaw(RingBuffer* buffer, RingBufferEntry* entry) 
{
    if (RingBuffer_Full(buffer)) buffer->tail++; // drop oldest packet in buffer
    uint16_t idx = buffer->head & buffer->mask;
    memcpy(&buffer->entries[idx], entry, sizeof(RingBufferEntry));
    buffer->head++;
}


