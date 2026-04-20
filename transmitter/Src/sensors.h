/* Responsible for reading in the CSV data files. 
   In real application it would communicate with real sensors to retrieve data*/
#ifndef SENSORS_H
#define SENSORS_H

#include "telemetry.h"
#include "testdata.h"
#include "queue.h"
#include "rs.h"

/* Values for reed solomon encoding of large data */
#define DATA_SHARDS 4
#define PARITY_SHARDS 2

#define BLOCK_SIZE (MAX_PAYLOAD_SIZE) /* size of each data and parity shard D0[32], D1[32], D2[32], D3[32], P0[32], P1[32] */
#define SHARDS_PER_BLOCK (DATA_SHARDS + PARITY_SHARDS)

#define MAX_BLOCKS 200
#define MAX_SHARDS (MAX_BLOCKS * SHARDS_PER_BLOCK)

void readEmbeddedData(QueueHandle_t queue, volatile uint32_t* cur_seq, size_t* idx, 
                            PacketType_e type, const void* embeddedData, 
                            size_t dataCount, size_t dataSize, volatile uint32_t* dropped_packets);

void processFragmentData(QueueHandle_t queue, volatile uint32_t* cur_seq, 
                            PacketType_e type, const void* data, 
                            size_t dataSize, volatile uint32_t* dropped_packets, Reliability_e reliable);


#endif