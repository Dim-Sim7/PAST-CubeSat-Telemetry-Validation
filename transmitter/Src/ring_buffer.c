#include "ring_buffer.h"

RingBufferEntry* findPacketBySeq(RingBuffer* buffer, uint32_t seq) {
    for (int i = 0; i < buffer->count; ++i) {
        // walk backwards from head through valid entries
        int idx = (buffer->head - 1 - i + BUFFER_SIZE) & BUFFER_MASK;
        if (buffer->entries[idx].seq == seq) {
            return &buffer->entries[idx];
        }
    }
    return NULL;
}

RingBufferEntry* dequeue(RingBuffer* buffer) {
    if (buffer->count == 0) {
        return NULL;
    }
    if (buffer->tail == buffer->head) {
        return NULL;
    }

    RingBufferEntry* entry = &buffer->entries[buffer->tail];
    buffer->tail = (buffer->tail + 1) & BUFFER_MASK;
    buffer->count--;

    return entry;
}

void enqueue(RingBuffer* buffer, TelemetryPacket_t* packet) {
    RingBufferEntry* entry = &buffer->entries[buffer->head];

    entry->seq = packet->seq;
    /* Calculating size of the actual packet */
    entry->size = sizeof(TelemetryPacket_t) - MAX_PAYLOAD_SIZE + packet->len;

    memcpy(entry->data, packet, entry->size);

    buffer->head = (buffer->head + 1) & BUFFER_MASK;

    if (buffer->count < BUFFER_SIZE) {
        buffer->count++;
    } else {
        //overwrite oldest
        buffer->tail = (buffer->tail + 1) & BUFFER_MASK;
    }
}
