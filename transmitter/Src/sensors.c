#include "sensors.h"
#include "projdefs.h"
#include "rs.h"
#include "telemetry.h"


/* Reading pre-structured data define in telemetry.h that is under MAX_PACKET_SIZE length */
/* Since payload is small, I only want to use CRC for error detection and use NACKs to request retransmits of lost packets */
void readEmbeddedData(QueueHandle_t queue, volatile uint32_t* cur_seq, size_t* idx, 
                            PacketType_e type, const void* embeddedData, 
                            size_t dataCount, size_t dataSize, volatile uint32_t* dropped_packets) 
{
    /* Since data is copied into the queue, we can use a stack based packet for construction */
    TelemetryPacket_t packet = {0};
    const void* element = (const uint8_t*)embeddedData + (*idx * dataSize); /* Move forward one dataSize at a time */
    createPacket(&packet, element, type, cur_seq);
    if(xQueueSend(queue, &packet, pdMS_TO_TICKS(10)) != pdPASS) {
        // Dropped packet, queue full
        taskENTER_CRITICAL();
        (*dropped_packets)++;
        taskEXIT_CRITICAL();
    }
    *idx = (*idx + 1) % dataCount; /* move forward idx and wrap back over array if needed */
    vTaskDelay(pdMS_TO_TICKS(10));
}


/* Uses reed-solomon FEC for data recovery in the receiver for large payloads */
/* See RS algorithm taken from https://github.com/jannson/reedsolomon-c/tree/master
    1. reed_solomon* reed_solomon_new(int data_shards, int parity_shards);
        data_shards = number of data blocks in one shard
        parity_shards = number of parity data blocks in one shard    
        shards[0] = D0(32 bytes) D1 D2 D3 P0 P1 ->>>> d_s = 4, p_s = 2
    
    2. reed_solomon_encode2(reed_solomon* rs, unsigned char** shards, int nr_shards, int block_size)
        rs from 1.
        shards[nr_shards][block_size] =
        [
        D0[32], D1[32], D2[32], D3[32], P0[32], P1[32],   // shard_block 0
        D0[32], D1[32], D2[32], D3[32], P0[32], P1[32],   // shard_block 1
        D0[32], D1[32], D2[32], D3[32], P0[32], P1[32],   // shard_block 2
        ...
        ] <- image stored as a sequence of shards, each shard has 4 x 32 byte data and 2 x 32 byte parity data

        block_size = MAX_PAYLOAD_SIZE = 32 bytes
        data_shards = 4, parity_shards = 2
        1 shard_block = 6 shards

        to find nr_shards -> e.g our image is 20030 bytes
        data_per_block = data_shards x block_size = 4 x 32 = 128 bytes (this is usable blocks of data, doesn't include calculated parity)
        nr_shards = ceil(20030 / 128) = 157 * 6 = 942 individual shards = 942 individual packets
        therefore our 2D array is shards[942][32]
    
        nr_shards = 942
        block_size = 32
    
    3. Iterate down each row and fill the array
    4. call encode on array
    5. create and enqueue packets into the Packet_Queue for transmission
*/

static uint8_t data_shards[DATA_SHARDS][BLOCK_SIZE];
static uint8_t parity_shards[PARITY_SHARDS][BLOCK_SIZE];

static uint8_t* shards[SHARDS_PER_BLOCK];

static void initShardPtrTable(void)
{
    for (int i = 0; i < DATA_SHARDS; i++)
        shards[i] = data_shards[i];

    for (int i = 0; i < PARITY_SHARDS; i++)
        shards[DATA_SHARDS + i] = parity_shards[i];
}

/* Use this for fragmentation of large payloads */
void processFragmentData(QueueHandle_t queue, volatile uint32_t* cur_seq, 
                            PacketType_e type, const void* data, 
                            size_t dataSize, volatile uint32_t* dropped_packets, Reliability_e reliable)
{
    if (dataSize == 0) return;
    initShardPtrTable();
    const uint8_t* bytes = (const uint8_t*) data;

    reed_solomon* rs = reed_solomon_new(DATA_SHARDS, PARITY_SHARDS);

    uint16_t data_per_block = DATA_SHARDS * BLOCK_SIZE; // 128 bytes of usable data per block
    size_t nr_blocks = (dataSize + data_per_block - 1) / data_per_block;

    TelemetryPacket_t packet = {0};
    FragmentMeta_t meta = {0};

    for (size_t b = 0; b < nr_blocks; b++)
    {
        initShardPtrTable();

        size_t base_offset = b * data_per_block;

        /* Fill one block shard of data */
        for (int i = 0; i < DATA_SHARDS; i++)
        {
            size_t offset = base_offset + (i * BLOCK_SIZE);
            size_t remaining = (offset < dataSize) ? (dataSize - offset) : 0;

            size_t copy_size = (remaining > BLOCK_SIZE) ? BLOCK_SIZE : remaining;

            if (remaining > 0)
            {
                memcpy(data_shards[i], bytes + offset, copy_size);
            }

            /* Fill unused remainder with 0's */
            if (copy_size < BLOCK_SIZE)
            {
                memset(data_shards[i] + copy_size, 0, BLOCK_SIZE - copy_size);
            }
        }

        /* Encode block shard */
        reed_solomon_encode2(rs, (unsigned char**)shards, SHARDS_PER_BLOCK, BLOCK_SIZE);

        /* Enqueue data immediately after */
        for (int i = 0; i < SHARDS_PER_BLOCK; i++) /* i = 0 -> DATA_SHARDS + PARITY_SHARDS */
        {
            meta.block_id = b;
            meta.frag_idx = i;
            meta.frag_total = SHARDS_PER_BLOCK;
            meta.len = BLOCK_SIZE;

            createFragmentPacket(&packet, shards[i], type, cur_seq, meta, reliable);

            if (xQueueSend(queue, &packet, pdMS_TO_TICKS(10)) != pdPASS)
            {
                taskENTER_CRITICAL();
                (*dropped_packets)++;
                taskEXIT_CRITICAL();
            }
        }
    }

    reed_solomon_release(rs);
}

