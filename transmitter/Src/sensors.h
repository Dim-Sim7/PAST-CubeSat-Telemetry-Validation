/* Responsible for reading in the CSV data files. 
   In real application it would communicate with real sensors to retrieve data*/
#ifndef SENSORS_H
#define SENSORS_H

#include "telemetry.h"
#include "testdata.h"
#include "queue.h"

void readEmbeddedData(QueueHandle_t queue, uint16_t* cur_seq, PacketType_e type);


#endif