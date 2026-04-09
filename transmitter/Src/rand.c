#include "rand.h"

/* Random number generator found from -> https://github.com/CTSRD-CHERI/FreeRTOS-Demos-CHERI-RISC-V/blob/master/bsp/rand.c */
/* rand() was not working so I found the freeRTOS function xApplicationGetRandomNumber */

/* Temporary fix - defines "uxrandom" function */


UBaseType_t ulNextRand;

UBaseType_t uxRand( void )
{
    const uint32_t ulMultiplier = 0x015a4e35UL, ulIncrement = 1UL;

    /* Utility function to generate a pseudo random number. */
    ulNextRand = ( ulMultiplier * ulNextRand ) + ulIncrement;
    return( ( int ) ( ulNextRand >> 16UL ) & 0x7fffUL );
}

/*
 * Set *pulNumber to a random number, and return pdTRUE. When the random number
 * generator is broken, it shall return pdFALSE.
 * The macros ipconfigRAND32() and configRAND32() are not in use
 * anymore in FreeRTOS+TCP.
 */
BaseType_t xApplicationGetRandomNumber( uint32_t * pulNumber )
{
    *pulNumber = uxRand();
    return pdTRUE;
}