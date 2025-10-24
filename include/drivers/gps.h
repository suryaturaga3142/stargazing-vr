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

/**
 * @brief Initializes the UART peripheral and configures the NEO-M10.
 * @details Sets up GPIOs for uart1, sends a UBX command to the module
 * to force NMEA output at 9600 baud, and enables the RX interrupt.
 */
void gps_init(void);

/**
 * @brief Processes one buffered NMEA sentence from the interrupt.
 * @details This function should be called repeatedly in the main application
 * loop. It checks if a new line is ready, and if so, processes it,
 * prints it, and updates the g_latest_gps_data global.
 */
void gps_update(void);

#endif /* GPS_H */