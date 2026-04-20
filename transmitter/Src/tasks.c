#include "tasks.h"


/* definitions */
TaskHandle_t Read_GNSS;
TaskHandle_t Read_BAROMETER;
TaskHandle_t Read_IMU;
TaskHandle_t Read_BATTERY;
TaskHandle_t Transmit_Packets;
TaskHandle_t Retransmit_Packets;
TaskHandle_t Read_IMAGE_DATA;

QueueHandle_t Packet_Queue;
QueueHandle_t Retransmit_Queue;

volatile uint32_t cur_seq         = 0;
volatile uint32_t dropped_packets = 0;

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

    watermark = uxTaskGetStackHighWaterMark(Read_GNSS);
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

    watermark = uxTaskGetStackHighWaterMark(Read_BAROMETER);
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

    watermark = uxTaskGetStackHighWaterMark(Read_IMU);
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

    watermark = uxTaskGetStackHighWaterMark(Read_BATTERY);
    printf("Battery Task HWM: %lu words\r\n", (unsigned long)watermark);

    xApplicationGetRandomNumber(&r);
    vTaskDelay(pdMS_TO_TICKS(5 + (r % 30)));
  }
}

void Read_Image_Data_Task(void* argument)
{
  UBaseType_t watermark;
  
  while(1)
  {
    size_t img_size = 0;
    const uint8_t* data = getCameraFrame(&img_size);
    processFragmentData(Packet_Queue, &cur_seq, PACKET_TYPE_IMAGE, (const void*)(data), img_size, &dropped_packets, RELIABLE);
    watermark = uxTaskGetStackHighWaterMark(Read_IMAGE_DATA);
    printf("Read Image Data Task HWM: %lu words\r\n", (unsigned long)watermark);

    vTaskDelay(pdMS_TO_TICKS(10000));
  }  
  
}

/* Consumer task. Takes from the packet queue and transmits over UART 
    Need to be cautious of overflow as we have multiple producers filling up the queue
    We can implement another FIFO packet queue that continuously takes packets while 
    we are sending out with UART. 
 */

void Transmit_Packets_Consumer(void* argument) 
{
  TelemetryPacket_t packet;
  uint32_t tickDelay = pdMS_TO_TICKS(10);
  while (1) 
  {
    /* Block until a packet is available from producer queue or tickDelay elapses*/
    if(xQueueReceive(Packet_Queue, &packet, tickDelay) == pdTRUE) 
    {
      UartTx_Enqueue(&packet);
    }
  }
}

void Telemetry_Monitor_Task(void* argument)
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

void Retransmit_Task(void* argument)
{
  uint32_t seq;
  RingBufferEntry historyEntry;
  TelemetryPacket_t pkt;
  while(1)
  {
    if (xQueueReceive(Retransmit_Queue, &seq, portMAX_DELAY) == pdTRUE)
    {
      /* Search history buffer for this seq number */
      /* Re enqueue into txBuffer for retransmit */
      if(getFromHistory(seq, &historyEntry) == HISTORY_FOUND)
      {
        /* Packet found -> Retransmit */
        memcpy(&pkt, historyEntry.data, sizeof(pkt));
        UartTx_Enqueue(&pkt);
      }
    }
  }
}

void CreateTasks(void)
{
    xTaskCreate(Telemetry_Monitor_Task, "TelemetryMonitor", SENSOR_TASK_STACK_SIZE, NULL, 1, NULL);
    /* Producer tasks */
    xTaskCreate(Read_GNSS_Producer_Task, "Read_GNSS", SENSOR_TASK_STACK_SIZE, NULL, 2, &Read_GNSS);
    xTaskCreate(Read_Barometer_Producer_Task, "Read_Barometer", SENSOR_TASK_STACK_SIZE, NULL, 2, &Read_BAROMETER);
    xTaskCreate(Read_IMU_Producer_Task, "Read_IMU", SENSOR_TASK_STACK_SIZE, NULL, 2, &Read_IMU);
    xTaskCreate(Read_Battery_Producer_Task, "Read_Battery", SENSOR_TASK_STACK_SIZE, NULL, 2, &Read_BATTERY);
    xTaskCreate(Read_Image_Data_Task, "Read_Image_Data", TASK_STACK_SIZE, NULL, 2, &Read_IMAGE_DATA);
    /* Consumer tasks */
    xTaskCreate(Transmit_Packets_Consumer, "Transmit packets", TASK_STACK_SIZE, NULL, 3, &Transmit_Packets);

    /* Retransmit task */
    xTaskCreate(Retransmit_Task, "Retransmit_Task", SENSOR_TASK_STACK_SIZE, NULL, 3, &Retransmit_Packets);
}