/*******************************************************************************
 * @file        main.c
 * @brief       Implements the functionality for the main module.
 * @details     This is the main file for the project.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * 
 * @note        This module is designed to be the main module to drive everything.
 *              Don't put interrupts here. Initialize everything, go through the
 *              startup process, and execute the lowest level non critical blocking
 *              tasks that potentially cause priority blocking when in handlers.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include <stdio.h>
#include "gps.h"
#include "imu.h"

#include "pico/stdlib.h"

/* ---------------------------- Private Constants --------------------------- */
#define GPS_TEST
// #define IMU_TEST
// #define LCD_SPI_TEST
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */

#ifdef GPS_TEST

// How often to print new data (in milliseconds)
#define PRINT_INTERVAL_MS 1000

int main() {
    // Initialize stdio for printf over USB
    stdio_init_all();
    sleep_ms(2000); // Wait for terminal to connect
    printf("Pico SDK Peripheral Sandbox Started...\n");

    // Initialize the GPS module
    gps_init();
    printf("GPS module initialized. Waiting for data...\n");

    // Timer for periodic printing
    absolute_time_t next_print_time = make_timeout_time_ms(PRINT_INTERVAL_MS);

    while (true) {
        // Continuously poll the GPS driver to process new NMEA sentences
        gps_update();

        // Periodically print the latest known data
        if (time_reached(next_print_time)) {
            next_print_time = make_timeout_time_ms(PRINT_INTERVAL_MS); // Reset timer
            
            gps_data_t data = gps_get_data();

            if (data.fix_valid) {
                printf("[GPS_TEST] Fix: VALID, Sats: %d\n", data.satellites_tracked);
                printf("  Lat: %f, Lon: %f\n", data.latitude, data.longitude);
                printf("  Alt: %f m, Speed: %f knots\n", data.altitude, data.speed);
            } else {
                printf("[GPS_TEST] Fix: INVALID (Searching...)\n");
            }
        }
        
        // You could add other tasks here.
        // We don't sleep, so gps_update() is polled as fast as possible.
        // tight_loop_contents(); // Use this if you have no other tasks
    }

    return 0;
}

#endif

#ifdef IMU_H

int main() {

    return 0;
}

#endif


#ifdef LCD_SPI_TEST

#endif