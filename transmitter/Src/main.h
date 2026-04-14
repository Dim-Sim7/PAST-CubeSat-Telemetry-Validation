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


/* Hardware pins */
#define LED_Pin GPIO_PIN_3
#define LED_GPIO_Port GPIOB

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */