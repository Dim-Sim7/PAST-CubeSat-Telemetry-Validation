#ifndef IMAGE_H
#define IMAGE_H
#include <stdint.h>
#include <stddef.h>
extern const unsigned char Cat_jpg[];
extern unsigned int Cat_jpg_len;
const uint8_t* getCameraFrame(size_t* size);

#endif