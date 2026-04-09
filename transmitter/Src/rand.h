#ifndef RAND_H
#define RAND_H


#include "FreeRTOS.h"

/* Use by the pseudo random number generator. */
extern UBaseType_t ulNextRand;

UBaseType_t uxRand( void );
BaseType_t xApplicationGetRandomNumber(uint32_t * pulNumber);

#endif