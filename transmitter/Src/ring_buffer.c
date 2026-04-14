#include "ring_buffer.h"
#include "telemetry.h"

inline int RingBuffer_Full(RingBuffer* b) 
{
    return (RB_HEAD(b) - RB_TAIL(b)) == b->size;
}
inline int RingBuffer_Empty(RingBuffer* b) 
{
    return (RB_HEAD(b) - RB_TAIL(b)) == 0;
}

int findPacketBySeq(RingBuffer* buffer, uint32_t seq, RingBufferEntry* out) 
{
    uint16_t count = (uint16_t)(RB_HEAD(buffer) - RB_TAIL(buffer));
    for (int i = 0; i < count; i++) 
    {
        // walk backwards from head through valid entries
        uint16_t idx = (RB_HEAD(buffer) - 1 - i) & buffer->mask;
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

    uint16_t idx = RB_TAIL(buffer) & buffer->mask; // bit-wise wrap around. Much faster than modulus
    memcpy(out, &buffer->entries[idx], sizeof(RingBufferEntry));
    RB_SET_TAIL(buffer, RB_TAIL(buffer) + 1); // increment only after data is copied
    return 0;
}

void enqueue(RingBuffer* buffer, TelemetryPacket_t* packet) 
{
    if (RingBuffer_Full(buffer)) { RB_SET_TAIL(buffer, RB_TAIL(buffer) + 1); } // drop oldest packet in buffer
    uint16_t idx = RB_HEAD(buffer) & buffer->mask;

    RingBufferEntry* entry = &buffer->entries[idx];
    entry->seq = packet->seq;
    /* Fixed header size + actual payload bytes */
    entry->size = sizeof(TelemetryPacket_t) - MAX_PAYLOAD_SIZE + packet->len;
    memcpy(entry->data, packet, entry->size);

    RB_SET_HEAD(buffer, RB_HEAD(buffer) + 1);
}

void enqueueRaw(RingBuffer* buffer, RingBufferEntry* entry) 
{
    if (RingBuffer_Full(buffer)) { RB_SET_TAIL(buffer, RB_TAIL(buffer) + 1); } // drop oldest packet in buffer
    uint16_t idx = RB_HEAD(buffer) & buffer->mask;

    RingBufferEntry* slot = &buffer->entries[idx];
    /* Defensive bounds check - corrupt size field could overflow slot->data[] */
    if (entry->size > MAX_PACKET_SIZE) return;
    slot->seq = entry->seq;
    slot->size = entry->size;
    memcpy(slot->data, entry->data, entry->size);

    RB_SET_HEAD(buffer, RB_HEAD(buffer) + 1);
}


