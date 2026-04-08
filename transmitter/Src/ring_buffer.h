#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "telemetry.h"

#define BUFFER_SIZE 64 // Can store 64 packets at once
#define BUFFER_MASK (BUFFER_SIZE - 1)

typedef struct __attribute__((__packed__))
{
    uint16_t size;                 // actual size of the stored packet
    uint32_t seq;
    uint8_t type;
    uint8_t data[MAX_PACKET_SIZE]; // size of TelemetryPacket_t
} RingBufferEntry;

typedef struct
{
    RingBufferEntry entries[BUFFER_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} RingBuffer;

RingBufferEntry* findPacketBySeq(RingBuffer* Buffer, uint32_t seq);
RingBufferEntry* dequeue(RingBuffer* Buffer);
void enqueue(RingBuffer* buffer, TelemetryPacket_t* packet);

#endif