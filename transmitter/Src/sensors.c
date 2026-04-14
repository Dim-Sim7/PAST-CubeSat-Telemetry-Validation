#include "sensors.h"
#include "telemetry.h"

/* Reading pre-structured data define in telemetry.h that is under MAX_PACKET_SIZE length */
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

/* Use this for fragmentation of large payloads */
void readFragmentData(QueueHandle_t queue, volatile uint32_t* cur_seq, 
                            PacketType_e type, const void* data, 
                            size_t dataSize, volatile uint32_t* dropped_packets, Reliability_e reliable) 
{
    if (dataSize == 0) return;
    const uint8_t* bytes = (const uint8_t*)data;
    TelemetryPacket_t packet = {0};
    FragmentMeta_t fragMeta = {0};
    /* Ceiling integer division */
    uint16_t frag_total = (dataSize + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE;

    for (uint16_t frag_idx = 0; frag_idx < frag_total; frag_idx++)
    {
        size_t offset = frag_idx * MAX_PAYLOAD_SIZE;

        size_t payloadChunkSize = dataSize - offset;
        if (payloadChunkSize > MAX_PAYLOAD_SIZE)
        {
            payloadChunkSize = MAX_PAYLOAD_SIZE;
        }
        fragMeta.frag_idx = frag_idx;
        fragMeta.frag_total = frag_total;
        fragMeta.len = payloadChunkSize;
        createFragmentPacket(&packet, bytes + offset, type, cur_seq, fragMeta, reliable);
        if(xQueueSend(queue, &packet, pdMS_TO_TICKS(10)) != pdPASS) 
        {
            // Dropped packet, queue full
            taskENTER_CRITICAL();
            (*dropped_packets)++;
            taskEXIT_CRITICAL();
        }
    }
}