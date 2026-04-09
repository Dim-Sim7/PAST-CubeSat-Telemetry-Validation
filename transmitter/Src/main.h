#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif
//#include "projdefs.h"
#include "ring_buffer.h"
#include "sensors.h"
#include "stm32l4xx_hal_uart.h"
#include "stm32l4xx_hal.h"
#include "FreeRTOS.h"
#include "tasks.h"
#include "queue.h"
#include <stdint.h>


void Error_Handler(void);
// /* Global task handles */
// extern TaskHandle_t Read_GNSS;
// extern TaskHandle_t Read_BAROMETER;
// extern TaskHandle_t Read_IMU;
// extern TaskHandle_t Read_BATTERY;
// extern TaskHandle_t Transmit_Packets;
// extern TaskHandle_t Retransmit_Packets;

// /* Global queues */
// extern QueueHandle_t Packet_Queue;
// extern QueueHandle_t Retransmit_Queue;

// /* Packet counters */
// extern volatile uint32_t cur_seq;
// extern volatile uint32_t dropped_packets;

/* Hardware pins */
#define LED_Pin GPIO_PIN_3
#define LED_GPIO_Port GPIOB

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */