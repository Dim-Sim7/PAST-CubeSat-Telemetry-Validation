#ifndef UART_COMMS_H
#define UART_COMMS_H

#include "stm32l4xx_hal.h"
#include "ring_buffer.h"
#include "history.h"
#include "tmr.h"
#include "history.h"
#include "portmacro.h"
#include "projdefs.h"
#include "sensors.h"
#include "stm32l432xx.h"
#include "stm32l4xx_hal_uart.h"
#include "tasks.h"
#include "telemetry.h"

/* MUST be a power of 2 */
#define TX_BUFFER_SIZE 128
#define RX_BUFFER_SIZE 64

/* Triple Modualar Redundancy for uartTxBusy flag */
#define SET_BUSY_TRUE(a, b, c) do { \
    a = pdTRUE;                     \
    b = pdTRUE;                     \
    c = pdTRUE;                     \
} while(0)

#define SET_BUSY_FALSE(a, b, c) do { \
    a = pdFALSE;                     \
    b = pdFALSE;                     \
    c = pdFALSE;                     \
} while(0)
extern UART_HandleTypeDef huart2;

BaseType_t UartTx_Enqueue(TelemetryPacket_t *packet);
void StartUartRx(void);

#define NACK_START_BYTE 0xBB

typedef struct __packed
{
    uint8_t start_byte;
    uint8_t type;
    uint32_t seq;
    uint16_t crc;
} NACKPacket_t;


#endif