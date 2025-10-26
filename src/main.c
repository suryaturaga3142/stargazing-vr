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
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/i2c.h"
#include <stdio.h>

#include "globals.h" // Includes all project headers
#include "gps.h"     // The driver we are testing
#include "imu.h"

#include "pico/stdlib.h"

/* ---------------------------- Private Constants --------------------------- */
// #define LCD_SPI_TEST
// #define GPS_DUMB_TEST
#define GPS_RAW_TEST
// #define IMU_TEST
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */


#ifdef LCD_SPI_TEST

#include "pico/stdlib.h"
#include <stdio.h>
#include "display.h"

// Define 16-bit (RGB565) colors
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_WHITE   0xFFFF
#define COLOR_BLACK   0x0000

int main() {
    // Initialize standard I/O (for printf over USB)
    stdio_init_all();
    sleep_ms(4000); // Wait for terminal to connect

    printf("=====================================\n");
    printf("     ILI9486 SPI Test\n");
    printf("=====================================\n");

    // Initialize the LCD
    lcd_init();
    printf("LCD Init complete.\n");

    // Run a color cycle test
    while (true) {
        printf("Filling screen RED\n");
        lcd_fill_screen(COLOR_RED);
        sleep_ms(1000);

        printf("Filling screen GREEN\n");
        lcd_fill_screen(COLOR_GREEN);
        sleep_ms(1000);

        printf("Filling screen BLUE\n");
        lcd_fill_screen(COLOR_BLUE);
        sleep_ms(1000);
        
        printf("Filling screen BLACK\n");
        lcd_fill_screen(COLOR_BLACK);
        sleep_ms(1000);
    }

    return 0;
}


#endif


#ifdef GPS_DUMB_TEST

int main() {
    // Initialize standard I/O (for printf over USB)
    stdio_init_all();
    sleep_ms(2000); // Wait for terminal to connect
    printf("=====================================\n");
    printf("Starting GPS Raw NMEA Test...\n");
    printf("=====================================\n");

    // Initialize the GPS driver
    gps_init();
    printf("GPS driver initialized. Forcing NMEA output at 9600 baud...\n");
    printf("Listening for NMEA sentences from NEO-M10:\n\n");

    for(;;) {

    }
    return 0;
}

#endif

#ifdef GPS_RAW_TEST

int main() {
    // Initialize standard I/O (for printf over USB)
    stdio_init_all();
    sleep_ms(2000); // Wait for terminal to connect
    printf("=====================================\n");
    printf("Starting GPS Raw NMEA Test...\n");
    printf("=====================================\n");

    // Initialize the GPS driver
    gps_init();
    printf("GPS driver initialized. Forcing NMEA output at 9600 baud...\n");
    printf("Listening for NMEA sentences from NEO-M10:\n\n");

    // Main application loop
    while (true) {
        // This function checks for a new line, prints it, and parses it.
        // It does all the work for this test.
        gps_update();

        // Note: We don't need a sleep_ms() here because the test
        // is interrupt-driven. gps_update() will return immediately
        // if no new line is ready, keeping the loop tight.
    }

    return 0;
}

#endif


#ifdef IMU_TEST

int main() {

    return 0;
}

#endif
