/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "portmacro.h"
#include "projdefs.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_uart.h"
#include "telemetry.h"
#include "testdata.h"
#include "uart_comms.h"
#include "sensors.h"
#include "ring_buffer.h"
#include "task.h"
#include "queue.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
xTaskHandle Read_GNSS;
xTaskHandle Read_BAROMETER;
xTaskHandle Read_IMU;
xTaskHandle Read_BATTERY;
xTaskHandle Transmit_Packets;
xTaskHandle Retransmit_Packets;

xQueueHandle Packet_Queue;
xQueueHandle Retransmit_Queue;

HistoryRingBuffer historyBuffer;

uint32_t cur_seq = 0;
uint32_t dropped_packets = 0;
static const size_t data_count = 50; /* 50 points of data in each data set */

/* --------- READING SENSOR DATA --------- */
/* Goal is to simulate a real telemetry stream where different types of data are sent at random intervals */
void Read_GNSS_Producer() 
{
  const TickType_t delay = pdMS_TO_TICKS(5 + rand() % 20); //random delay
  static size_t gnss_idx = 0;
  while(1) 
  {
    readEmbeddedData(Packet_Queue, &cur_seq, &gnss_idx, PACKET_TYPE_GNSS, (const void*)gnssData, 
    data_count, sizeof(gnssData[0]), &dropped_packets);
    vTaskDelay(delay);
  }
}

void Read_Barometer_Producer() 
{
  const TickType_t delay = pdMS_TO_TICKS(5 + rand() % 15);
  static size_t baro_idx = 0;
  while(1) 
  {
    readEmbeddedData(Packet_Queue, &cur_seq, &baro_idx, PACKET_TYPE_BARO, (const void*)baroData, 
    data_count, sizeof(baroData[0]), &dropped_packets);
    vTaskDelay(delay);
  }
}

void Read_IMU_Producer() 
{
  const TickType_t delay = pdMS_TO_TICKS(5 + rand() % 5);
  static size_t imu_idx = 0;
  while(1) 
  {
    readEmbeddedData(Packet_Queue, &cur_seq, &imu_idx, PACKET_TYPE_IMU, (const void*)imuData, 
    data_count, sizeof(imuData[0]), &dropped_packets);
    vTaskDelay(delay);
  }
}

void Read_Battery_Producer() 
{
  const TickType_t delay = pdMS_TO_TICKS(5 + rand() % 30);
  static size_t battery_idx = 0;
  while(1) 
  {
    readEmbeddedData(Packet_Queue, &cur_seq, &battery_idx, PACKET_TYPE_BATTERY, (const void*)batteryData, 
    data_count, sizeof(batteryData[0]), &dropped_packets);
    vTaskDelay(delay);
  }
}

/* Consumer task. Takes from the packet queue and transmits over UART 
    Need to be cautious of overflow as we have multiple producers filling up the queue
    We can implement another FIFO packet queue that continuously takes packets while 
    we are sending out with UART. HAL_UART_Transmit_IT is an interrupt driven transmit that allows us
    to continue as the bytes are being sent out, dont have to wait until all bytes are sent to continue

    Also can use a ring buffer specifically for the transmit, therefore can consume packets from the queue
    while waiting to transmit
    */
void Transmit_Packets_Consumer(void* argument) 
{
  TelemetryPacket_t packet = {0};
  uint16_t txSize;
  uint32_t tickDelay = pdMS_TO_TICKS(10);
  RingBuffer txBuffer;
  /* TODO: USE THE TXBUFFER TO STORE WHILE TRANSMITTING */
  while (1) 
  {
    if(xQueueReceive(Packet_Queue, &packet, tickDelay) == pdTRUE) 
    {
      /* Calculate actual size of packet */
      txSize = sizeof(TelemetryPacket_t) - MAX_PAYLOAD_SIZE + packet->len;
      HAL_UART_Transmit_IT(&huart2, (uint8_t*)&packet, txSize);
    }
  }
  
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  static uint8_t serial_string[51] = "";

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  Packet_Queue = xQueueCreate(128, sizeof(TelemetryPacket_t));
  if (Packet_Queue == NULL) {
    HAL_UART_Transmit(&huart1, (uint8_t*)"Queue was not created successfully", 34, HAL_MAX_DELAY);
  }

  /* Transmit the data using huart2, transmits 51 bytes with a 10ms timeout */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* Send telemetry data here, build packets, etc... meat goes here */
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    HAL_Delay(1000);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
