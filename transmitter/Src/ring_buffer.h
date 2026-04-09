/* Implementation informed by http://www.simplyembedded.org/tutorials/interrupt-free-ring-buffer /*/

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "telemetry.h"



typedef struct __attribute__((__packed__))
{
    uint16_t size;                 // actual size of the stored packet
    uint32_t seq;                  // seq of the stored packet
    uint8_t data[MAX_PACKET_SIZE]; // data of the stored packet MAX_PACKET_SIZE = sizeof(TelemetryPacket_t)
} RingBufferEntry;

/* Volatile is used to ensure the program checks the value
    of head and tail each time it is used. Without these,
    the compiler may cache these in registers and never re
    read them from actual memory. Dangerous in interrupt-driven code */
typedef struct
{
    RingBufferEntry* entries;
    volatile uint16_t head;
    volatile uint16_t tail;
    uint16_t size;
    uint16_t mask; /* size - 1  ---- Used for bit-wise index wrap arounds*/
} RingBuffer;

int findPacketBySeq(RingBuffer* buffer, uint32_t seq, RingBufferEntry* out);
int dequeue(RingBuffer* buffer, RingBufferEntry* out);
void enqueue(RingBuffer* buffer, TelemetryPacket_t* packet);
void enqueueRaw(RingBuffer* buffer, RingBufferEntry* entry);

extern int RingBuffer_Full(RingBuffer* b);
extern int RingBuffer_Empty(RingBuffer* b);

#endif