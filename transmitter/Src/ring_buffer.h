/* Implementation informed by http://www.simplyembedded.org/tutorials/interrupt-free-ring-buffer /*/

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "telemetry.h"
#include "tmr.h"

typedef struct 
{
    uint16_t size;                 // actual size of the stored packet
    uint32_t seq;                  // seq of the stored packet
    uint8_t data[MAX_PACKET_SIZE]; // pre-packed data of the stored packet MAX_PACKET_SIZE = sizeof(TelemetryPacket_t)
} RingBufferEntry;

/* Volatile is used to ensure the program checks the value
    of head and tail each time it is used. Without these,
    the compiler may cache these in registers and never re
    read them from actual memory. Dangerous in interrupt-driven code */
typedef struct
{
    RingBufferEntry* entries;

    volatile uint16_t head_a;
    volatile uint16_t head_b;
    volatile uint16_t head_c;

    volatile uint16_t tail_a;
    volatile uint16_t tail_b;
    volatile uint16_t tail_c;

    uint16_t size; /* The number of entries in the array MUST be a power of 2*/
    uint16_t mask; /* size - 1  ---- Used for bit-wise index wrap arounds*/
} RingBuffer;

/* Triple Modular Redundancy for head and tail */
#define RB_HEAD(b) TMR_Vote_size((b)->head_a, (b)->head_b, (b)->head_c)
#define RB_TAIL(b) TMR_Vote_size((b)->tail_a, (b)->tail_b, (b)->tail_c)

#define RB_SET_HEAD(b, v) do { \
    (b)->head_a = (v);         \
    (b)->head_b = (v);         \
    (b)->head_c = (v);         \
} while(0)

#define RB_SET_TAIL(b, v) do { \
    (b)->tail_a = (v);         \
    (b)->tail_b = (v);         \
    (b)->tail_c = (v);         \
} while(0)


int findPacketBySeq(RingBuffer* buffer, uint32_t seq, RingBufferEntry* out);
int dequeue(RingBuffer* buffer, RingBufferEntry* out);
void enqueue(RingBuffer* buffer, const TelemetryPacket_t* packet);
void enqueueRaw(RingBuffer* buffer, RingBufferEntry* entry);

extern int RingBuffer_Full(RingBuffer* b);
extern int RingBuffer_Empty(RingBuffer* b);

#endif