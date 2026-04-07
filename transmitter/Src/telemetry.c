#include "telemetry.h"

/* crc algorithm from here -> https://srecord.sourceforge.net/crc16-ccitt.html */
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
void createPacket(TelemetryPacket_t* packet, const void* data, PacketType_e type, uint16_t* cur_seq) {
    const PacketInfo_t* info = &PACKET_INFO[type];

    packet->sof = TELEMETRY_SOF;
    packet->type = info->type;
    packet->seq = (*cur_seq)++;
    packet->reliable = UNRELIABLE;
    packet->len = info->len;
    packet->eof = TELEMETRY_EOF;

    memcpy(packet->payload, data, info->len);

    packet->crc = calculateCrc((const uint8_t*)packet, 
                    offsetof(TelemetryPacket_t, crc)); //stop right before crc field in struct

}

/* Helper functions for the history buffer */
HistoryRingBufferEntry* getFromHistorySpecific(HistoryRingBuffer* historyBuffer, int seq) {
    int i;
    for (i = 0; i < historyBuffer->count; ++i) {
        // walk backwards from head through valid entries
        int idx = (historyBuffer->head - 1 - i + HISTORY_SIZE) & HISTORY_MASK; // + HISTORY_SIZE prevents negative idx
        if (historyBuffer->entries[idx].seq == seq) {
            return &historyBuffer->entries[i];
        }
    }
    return NULL;
}

HistoryRingBufferEntry* getFromHistory(HistoryRingBuffer* historyBuffer) {
    if (historyBuffer->count == 0) {
        return NULL;
    }
    if (historyBuffer->tail == historyBuffer->head) {
        return NULL;
    }

    HistoryRingBufferEntry* entry = &historyBuffer->entries[historyBuffer->tail];
    historyBuffer->tail = (historyBuffer->head + 1) & HISTORY_MASK;

    return entry;
}

void addToHistory(HistoryRingBuffer* historyBuffer, TelemetryPacket_t* packet) {
    HistoryRingBufferEntry* entry = &historyBuffer->entries[historyBuffer->head];

    entry->seq = packet->seq;
    entry->size = packet->len;

    memcpy(entry->data, packet->payload, packet->len);

    historyBuffer->head = (historyBuffer->head + 1) & HISTORY_MASK;

    if (historyBuffer->count < HISTORY_SIZE) {
        historyBuffer->count++;
    }
}


