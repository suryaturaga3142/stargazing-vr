#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Minimal conversions used by the library

float qToFloat(int16_t q, uint8_t qFormat)
{
    // q is in Q format; scale according to qFormat
    // This is a conservative implementation
    return (float)q / (float)(1 << qFormat);
}

bool wait_ms(uint32_t ms)
{
    struct timespec req = { ms/1000, (ms%1000)*1000000 };
    nanosleep(&req, NULL);
    return true;
}
