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

extern volatile GPSData_t g_latest_gps_data;      // Updated by the main loop from GPS data.

extern volatile bool g_gps_correction_needed;     // Set by RTC alarm, handled by main.

#endif /* GPS_H */