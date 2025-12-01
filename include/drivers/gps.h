/*******************************************************************************
 * @file        gps.h
 * @brief       Header file for GPS driver library
 * @details     Function and locals declaration for GPS module.
 * 
 * @see         gps.c for implementation details.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * @version     1.0
 ******************************************************************************/

#ifndef GPS_H
#define GPS_H

#include <stdbool.h>
#include "structs.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif
    
extern GPSData_t g_latest_gps_data;      // Updated by the main loop from GPS data.

extern volatile bool g_gps_correction_needed;     // Set by interrupt, handled by gps_check_and_read.

bool gps_init(void);
bool gps_check_and_read(void);

#ifdef __cplusplus
}
#endif

#endif /* GPS_H */