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

/* I used a fixed buffer for the payload as it is simple and predictable
When I send the packet I only send the used portion*/
typedef struct __attribute__((__packed__))
{
    uint8_t sof;
    uint8_t type;
    uint32_t seq;
    uint8_t reliable;
    uint8_t len;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint16_t crc;
    uint8_t eof;
} TelemetryPacket_t;

#define MAX_PACKET_SIZE sizeof(TelemetryPacket_t)

typedef struct __attribute__((__packed__))
{
    float time;
    double latitude;
    double longitude;
    float altitude;
} GNSSData_t;

typedef struct __attribute__((__packed__))
{
    uint32_t pressure;
    int16_t temperature;
} BarometerData_t;

typedef struct __attribute__((__packed__))
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

typedef struct __attribute__((__packed__))
{
    uint8_t hours, minutes;
    uint8_t percent;
    char status[8];
} BatteryData_t;

typedef struct __attribute__((__packed__))
{
    uint8_t type;
    uint32_t from_seq;
    uint32_t to_seq;
} RetransmitRequest;

/* Helper enums and struct to construct packets */
typedef enum {
    RELIABLE,
    UNRELIABLE
} Reliability_e;

typedef enum {
    PACKET_TYPE_GNSS,
    PACKET_TYPE_BARO,
    PACKET_TYPE_IMU,
    PACKET_TYPE_BATTERY
} PacketType_e;


typedef struct {
    PacketType_e type;
    uint8_t      len;
    Reliability_e reliable;
} PacketInfo_t;

extern const PacketInfo_t PACKET_INFO[];
extern const PacketType_e PACKET_TYPES[];

uint16_t calculateCrc(const uint8_t* data, uint8_t len);
void createPacket(TelemetryPacket_t* packet, const void* data, PacketType_e type, volatile uint32_t* cur_seq);

#endif