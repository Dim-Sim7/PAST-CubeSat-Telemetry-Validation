#include "tasks.h"



RingBufferEntry txEntries[TX_BUFFER_SIZE];
RingBufferEntry historyEntries[HISTORY_BUFFER_SIZE];
RingBuffer txBuffer = { .entries = txEntries, .head = 0, .tail = 0, .size = TX_BUFFER_SIZE, .mask = TX_BUFFER_SIZE - 1};
RingBuffer historyBuffer = { .entries = historyEntries, .head = 0, .tail = 0, .size = HISTORY_BUFFER_SIZE, .mask = HISTORY_BUFFER_SIZE - 1 };
/* --------- READING SENSOR DATA --------- */
/* Goal is to simulate a real telemetry stream where different types of data are sent at random intervals */
void Read_GNSS_Producer_Task(void* argument) 
{
  static size_t gnss_idx = 0;
  UBaseType_t watermark;
  uint32_t r;  // Random number storage
  while(1) 
  {
    readEmbeddedData(Packet_Queue, &cur_seq, &gnss_idx, PACKET_TYPE_GNSS, (const void*)gnssData, 
    DATA_COUNT, sizeof(gnssData[0]), &dropped_packets);

    watermark = uxTaskGetStackHighWaterMark(NULL);
    printf("GNSS Task HWM: %lu words\r\n", (unsigned long)watermark);

    xApplicationGetRandomNumber(&r);
    vTaskDelay(pdMS_TO_TICKS(5 + (r % 20)));
  }
}

void Read_Barometer_Producer_Task(void* argument) 
{
  static size_t baro_idx = 0;
  UBaseType_t watermark;
  uint32_t r;
  while(1) 
  {
    readEmbeddedData(Packet_Queue, &cur_seq, &baro_idx, PACKET_TYPE_BARO, (const void*)baroData, 
    DATA_COUNT, sizeof(baroData[0]), &dropped_packets);

    watermark = uxTaskGetStackHighWaterMark(NULL);
    printf("Barometer Task HWM: %lu words\r\n", (unsigned long)watermark);
    
    xApplicationGetRandomNumber(&r);
    vTaskDelay(pdMS_TO_TICKS(5 + (r % 15)));
  }
}

void Read_IMU_Producer_Task(void* argument) 
{
  static size_t imu_idx = 0;
  UBaseType_t watermark;
  uint32_t r;
  while(1) 
  {
    readEmbeddedData(Packet_Queue, &cur_seq, &imu_idx, PACKET_TYPE_IMU, (const void*)imuData, 
    DATA_COUNT, sizeof(imuData[0]), &dropped_packets);

    watermark = uxTaskGetStackHighWaterMark(NULL);
    printf("IMU Task HWM: %lu words\r\n", (unsigned long)watermark);

    xApplicationGetRandomNumber(&r);
    vTaskDelay(pdMS_TO_TICKS(5 + (r % 5)));
  }
}

void Read_Battery_Producer_Task(void* argument) 
{
  static size_t battery_idx = 0;
  UBaseType_t watermark;
  uint32_t r;
  while(1) 
  {
    readEmbeddedData(Packet_Queue, &cur_seq, &battery_idx, PACKET_TYPE_BATTERY, (const void*)batteryData, 
    DATA_COUNT, sizeof(batteryData[0]), &dropped_packets);

    watermark = uxTaskGetStackHighWaterMark(NULL);
    printf("Battery Task HWM: %lu words\r\n", (unsigned long)watermark);

    xApplicationGetRandomNumber(&r);
    vTaskDelay(pdMS_TO_TICKS(5 + (r % 30)));
  }
}

/* Consumer task. Takes from the packet queue and transmits over UART 
    Need to be cautious of overflow as we have multiple producers filling up the queue
    We can implement another FIFO packet queue that continuously takes packets while 
    we are sending out with UART. 
 */
void Transmit_Packets_Consumer(void* argument) 
{
  TelemetryPacket_t packet = {0};
  uint32_t tickDelay = pdMS_TO_TICKS(10);

  while (1) 
  {
    if(xQueueReceive(Packet_Queue, &packet, tickDelay) == pdTRUE) 
    {
      /* Received packet from the producer queue, now enqueue it to txBuffer*/
      enqueue(&txBuffer, &packet);
      StartUartTx();
    }
  }
}

void TelemetryMonitorTask(void* argument)
{
  uint32_t last_seq = 0;
  uint32_t last_dropped = 0;

  while (1)
  {

    uint32_t cur = cur_seq;
    uint32_t dropped = dropped_packets;

    printf("Total Packets: %lu, Dropped: %lu\r\n",
       (unsigned long)cur,
       (unsigned long)dropped);
    printf("Packets/sec: %lu, Dropped/sec: %lu\r\n", (unsigned long)(cur - last_seq), (unsigned long)(dropped - last_dropped));
    last_seq = cur;
    last_dropped = dropped;

    vTaskDelay(pdMS_TO_TICKS(1000)); /* Prints every 1 second */
  }
}

void CreateTasks(void)
{
    xTaskCreate(TelemetryMonitorTask, "TelemetryMonitor", TASK_STACK_SIZE, NULL, 1, NULL);
    /* Producer tasks */
    xTaskCreate(Read_GNSS_Producer_Task, "Read_GNSS", TASK_STACK_SIZE, NULL, 2, &Read_GNSS);
    xTaskCreate(Read_Barometer_Producer_Task, "Read_Barometer", TASK_STACK_SIZE, NULL, 2, &Read_BAROMETER);
    xTaskCreate(Read_IMU_Producer_Task, "Read_IMU", TASK_STACK_SIZE, NULL, 2, &Read_IMU);
    xTaskCreate(Read_Battery_Producer_Task, "Read_Battery", TASK_STACK_SIZE, NULL, 2, &Read_BATTERY);

    /* Consumer tasks */
    xTaskCreate(Transmit_Packets_Consumer, "Transmit packets", TASK_STACK_SIZE, NULL, 3, &Transmit_Packets);
}