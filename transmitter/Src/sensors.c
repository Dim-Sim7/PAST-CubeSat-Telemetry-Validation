#include "sensors.h"
#include "telemetry.h"

/* */
void readEmbeddedData(QueueHandle_t queue, volatile uint32_t* cur_seq, size_t* idx, 
                            PacketType_e type, const void* embeddedData, 
                            size_t dataCount, size_t dataSize, volatile uint32_t* dropped_packets) 
{
    /* Since data is copied into the queue, we can use a stack based packet for construction */
    TelemetryPacket_t packet = {0};
    const void* element = (const uint8_t*)embeddedData + (*idx * dataSize); /* Move forward one dataSize at a time*/
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
