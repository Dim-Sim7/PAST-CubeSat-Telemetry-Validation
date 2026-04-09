#ifndef UART_COMMS_H
#define UART_COMMS_H

#include "stm32l4xx_hal.h"
#include "ring_buffer.h"

extern UART_HandleTypeDef huart2;
extern RingBuffer txBuffer;
extern RingBuffer historyBuffer;

void StartUartTx(void);
uint8_t UartTxBusy(void);

#endif