/* Definitions for packet and data structs and packet build functions*/
/* https://stackoverflow.com/questions/5473189/what-is-a-packed-structure-in-c */
#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"

/* Maximum bytes that can be stored in payload
    If we have a data struct that goes over 32 bytes we need to adjust this*/
#define MAX_PAYLOAD_SIZE 32

#define TELEMETRY_SOF 0xAA
#define TELEMETRY_EOF 0x55

#define POLY 0x1021 /* for crc-ccitt mask */

typedef uint8_t Reliability_e;
typedef uint8_t PacketType_e;

#define RELIABLE    0x00
#define UNRELIABLE  0x01

#define PACKET_TYPE_GNSS       0x01
#define PACKET_TYPE_BARO       0x02
#define PACKET_TYPE_IMU        0x03
#define PACKET_TYPE_BATTERY    0x04
#define PACKET_TYPE_IMAGE      0x05
#define PACKET_TYPE_RETRANSMIT 0x06

/* I used a fixed buffer for the payload as it is simple and predictable
When I send the packet I only send the used portion*/
typedef struct __packed
{
    uint8_t sof;
    uint8_t type;

    uint32_t seq; /* Packet sequence ID */

    uint16_t block_id; /* identifier within a block -> see processLargePayloadData()*/
    uint8_t frag_index; /* 0, 1, 2... */
    uint16_t frag_total; /* total number of fragments */
    
    uint8_t reliable;
    uint8_t len;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint16_t crc;
    uint8_t eof;
} TelemetryPacket_t;

typedef struct
{
    uint16_t block_id;
    uint16_t frag_idx;
    uint16_t frag_total;
    uint16_t len;
} FragmentMeta_t;

#define MAX_PACKET_SIZE sizeof(TelemetryPacket_t)

typedef struct __packed
{
    float time;
    double latitude;
    double longitude;
    float altitude;
} GNSSData_t;

typedef struct __packed
{
    uint32_t pressure;
    int16_t temperature;
} BarometerData_t;

typedef struct __packed
{
    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;
    int16_t angularVelX;
    int16_t angularVelY;
    int16_t angularVelZ;
    int16_t magX;
    int16_t magY;
    int16_t magZ;
} IMUData_t;

typedef struct __packed
{
    uint8_t hours;
    uint8_t minutes;
    uint8_t percent;
    char status[8];
} BatteryData_t;

typedef struct __packed
{
    uint8_t type;
    uint32_t from_seq;
    uint32_t to_seq;
} RetransmitRequest;

typedef struct {
    uint8_t      len;
    Reliability_e reliable;
} PacketInfo_t;

extern const PacketInfo_t PACKET_INFO[];
extern const PacketType_e PACKET_TYPES[];

uint16_t calculateCrc(const uint8_t* data, uint8_t len);
uint8_t validateCrc(uint16_t* packet_crc, const uint8_t* data, int8_t len);
void createPacket(TelemetryPacket_t* packet, const void* data, PacketType_e type, volatile uint32_t* cur_seq);
void createFragmentPacket(TelemetryPacket_t* packet, const void* data, PacketType_e type, 
                volatile uint32_t* cur_seq, FragmentMeta_t fragMeta, Reliability_e reliable);
#endif