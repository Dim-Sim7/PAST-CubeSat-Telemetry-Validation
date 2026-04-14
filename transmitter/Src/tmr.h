#ifndef TMR_H
#define TMR_H

#include <stddef.h>
#include <stdint.h>
#include "FreeRTOS.h"


/* Voting algorithms for different types of data
    Uses bit-wise operations to return either 2 of 3 same values or 3 of 3 same values 
    AND  (1 AND 0) = 0, (1 AND 1) = 1
    OR   (1 OR 0) = 1, (0 OR 0) = 0*/
static inline size_t TMR_Vote_size(size_t a, size_t b, size_t c) 
{
    return (a & b) | (b & c) | (a & c);
}

static inline size_t TMR_Vote_u32(uint32_t a, uint32_t b, uint32_t c) 
{
    return (a & b) | (b & c) | (a & c);
}

static inline BaseType_t TMR_Vote_BaseType(BaseType_t a, BaseType_t b, BaseType_t c)
{
    return (a & b) | (b & c) | (a & c);
}

#endif