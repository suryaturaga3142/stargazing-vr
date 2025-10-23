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
 * @brief Holds the latest parsed GPS data.
 * All values are 0 or false until a valid fix is acquired.
 */
typedef struct {
    float latitude;         // Decimal degrees
    float longitude;        // Decimal degrees
    float altitude;         // Meters
    float speed;            // Knots
    int satellites_tracked; // Number of satellites
    bool fix_valid;         // True if fix is valid
} gps_data_t;

/**
 * @brief Initializes the UART hardware (pins, baud, interrupts)
 * using the settings from config.h.
 */
void gps_init(void);

/**
 * @brief This function should be called repeatedly in the main loop.
 * It checks for a complete NMEA sentence from the UART interrupt,
 * parses it, and updates the internal GPS data state.
 *
 * @return true if a new RMC or GGA sentence was successfully parsed, false otherwise.
 */
bool gps_update(void);

/**
 * @brief Gets a copy of the most recently parsed GPS data.
 * This function is safe to call at any time.
 *
 * @return A gps_data_t struct with the latest data.
 */
gps_data_t gps_get_data(void);


#endif /* GPS_H */