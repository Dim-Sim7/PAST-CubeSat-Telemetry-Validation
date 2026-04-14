#ifndef TASKS_H
#define TASKS_H

#include "testdata.h"
#include "telemetry.h"
#include "sensors.h"
#include "ring_buffer.h"
#include "queue.h"
#include "stm32l4xx_hal_uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "rand.h"
#include "uart_comms.h"

#define TASK_STACK_SIZE 512
#define MAX_QUEUE_SIZE 128 /* Producer packet queue size */

extern TaskHandle_t Read_GNSS;
extern TaskHandle_t Read_BAROMETER;
extern TaskHandle_t Read_IMU;
extern TaskHandle_t Read_BATTERY;
extern TaskHandle_t Transmit_Packets;
extern TaskHandle_t Retransmit_Packets;

/* Global queues */
extern QueueHandle_t Packet_Queue;
extern QueueHandle_t Retransmit_Queue;

/* Master running counter for packets */
extern volatile uint32_t cur_seq;
extern volatile uint32_t dropped_packets;

void Read_GNSS_Producer_Task(void* argument);
void Read_Barometer_Producer_Task(void* argument);
void Read_IMU_Producer_Task(void* argument);
void Read_Battery_Producer_Task(void* argument);

void Transmit_Packets_Consumer(void* argument);
void TelemetryMonitorTask(void* argument);

void CreateTasks(void);
#endif