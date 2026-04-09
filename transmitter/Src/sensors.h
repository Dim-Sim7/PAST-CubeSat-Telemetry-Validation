/* Responsible for reading in the CSV data files. 
   In real application it would communicate with real sensors to retrieve data*/
#ifndef SENSORS_H
#define SENSORS_H

#include "telemetry.h"
#include "testdata.h"
#include "queue.h"

void readEmbeddedData(QueueHandle_t queue, volatile uint32_t* cur_seq, size_t* idx, 
                            PacketType_e type, const void* embeddedData, 
                            size_t dataCount, size_t dataSize, volatile uint32_t* dropped_packets);


#endif