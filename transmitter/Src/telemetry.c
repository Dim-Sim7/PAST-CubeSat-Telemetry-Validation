#include "telemetry.h"

/* crc algorithm derived from here -> https://srecord.sourceforge.net/crc16-ccitt.html */
uint16_t calculateCrc(const uint8_t* data, uint8_t len) {
    uint16_t crc = 0xffff;
    uint8_t v = 0x80;
    
    for (uint8_t i = 0; i < len; ++i) {
        for (uint8_t bit = 0; bit < 8; bit++) {
            int xor_flag = ((crc & 0x8000) != 0);
            crc = crc << 1;
            if (data[i] & v) {
                crc += 1;
            }
            if (xor_flag) {
                crc = crc ^ POLY;
            }
            v = v >> 1;
        }
    }
    return crc;
}

/* Protocol byte order: little-endian (native ARM Cortex-M) */ 
void createPacket(TelemetryPacket_t* packet, const void* data, PacketType_e type, uint32_t* cur_seq) {
    const PacketInfo_t* info = &PACKET_INFO[type];

    packet->sof = TELEMETRY_SOF;
    packet->type = info->type;
    taskENTER_CRITICAL();
    packet->seq = (*cur_seq)++;
    taskEXIT_CRITICAL();
    packet->reliable = info->reliable;
    packet->len = info->len;
    packet->eof = TELEMETRY_EOF;

    memcpy(packet->payload, data, info->len);

    packet->crc = calculateCrc((const uint8_t*)packet, 
                    offsetof(TelemetryPacket_t, crc)); //stop right before crc field in struct

}



