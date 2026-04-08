#include "sensors.h"

void readEmbeddedData(QueueHandle_t queue, uint16_t* cur_seq, PacketType_e type) {
    /* Since data is copied into the queue, we can use one packet to create all packets*/
    TelemetryPacket_t packet = {0};

    if (type == PACKET_TYPE_GNSS) {
        size_t count = sizeof(gnssData)/sizeof(gnssData[0]);
        for (size_t i = 0; i < count; i++) {
            createPacket(&packet, &gnssData[i], type, cur_seq);
            xQueueSend(queue, &packet, portMAX_DELAY);
        }
    }
    else if (type == PACKET_TYPE_BARO) {
        size_t count = sizeof(baroData)/sizeof(baroData[0]);
        for (size_t i = 0; i < count; i++) {
            createPacket(&packet, &baroData[i], type, cur_seq);
            xQueueSend(queue, &packet, portMAX_DELAY);
        }
    }
    else if (type == PACKET_TYPE_IMU) {
        size_t count = sizeof(imuData)/sizeof(imuData[0]);
        for (size_t i = 0; i < count; i++) {
            createPacket(&packet, &imuData[i], type, cur_seq);
            xQueueSend(queue, &packet, portMAX_DELAY);
        }
    }
    else if (type == PACKET_TYPE_BATTERY) {
        size_t count = sizeof(batteryData)/sizeof(batteryData[0]);
        for (size_t i = 0; i < count; i++) {
            createPacket(&packet, &batteryData[i], type, cur_seq);
            xQueueSend(queue, &packet, portMAX_DELAY);
        }
    }
}