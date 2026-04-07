/* Responsible for reading in the CSV data files. 
   In real application it would communicate with real sensors to retrieve data*/
#ifndef SENSORS_H
#define SENSORS_H

#include "telemetry.h"
#include "stdio.h"
#include "FreeRTOS.h" 
#include "queue.h"


typedef int (*ParseLineFn)(const char* line, void* data);

#endif