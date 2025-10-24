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
// #define GPS_I2C_SCAN
// #define GPS_I2C_TEST
// #define GPS_DUMB_TEST
// #define GPS_RAW_TEST
// #define IMU_TEST
// #define LCD_SPI_TEST
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */



#ifdef GPS_I2C_SCAN

int main() {
    // Initialize standard I/O (for printf over USB)
    stdio_init_all();
    sleep_ms(4000); // Wait for terminal to connect

    printf("=====================================\n");
    printf("     I2C Scanner on i2c1\n");
    printf("=====================================\n");
    printf("Initializing i2c1...\n");
    printf("SDA PIN: %d (GPIO %d)\n", PIN_GPS_TX, PIN_GPS_TX);
    printf("SCL PIN: %d (GPIO %d)\n", PIN_GPS_RX, PIN_GPS_RX);

    // Initialize i2c1
    i2c_init(i2c1, 100 * 1000); // 100 kHz
    
    // Set pins to I2C function
    gpio_set_function(PIN_GPS_TX, GPIO_FUNC_I2C); // GPIO 38
    gpio_set_function(PIN_GPS_RX, GPIO_FUNC_I2C); // GPIO 39
    
    // The SDK's i2c_init enables internal pull-ups.
    // We can explicitly enable them to be safe.
    gpio_pull_up(PIN_GPS_TX);
    gpio_pull_up(PIN_GPS_RX);

    printf("Scanning addresses 0x00 to 0x7F...\n\n");

    for (int addr = 0; addr < (1 << 7); ++addr) {
        
        // Skip invalid 7-bit addresses
        if (addr < 0x08 || addr > 0x77) {
            continue;
        }

        uint8_t rxdata;
        int ret = i2c_read_blocking(i2c1, addr, &rxdata, 1, false);

        if (ret >= 0) {
            // Found a device!
            printf("Device found at I2C address: 0x%02X\n", addr);
        } else {
            // No device at this address
            // Optional: print a dot for every failed attempt
            // printf(".");
        }
        sleep_ms(10); // Short delay between probes
    }

    printf("\nScan complete.\n");

    // Loop forever
    while (true) {
        sleep_ms(1000);
    }

    return 0;
}
#endif


#ifdef GPS_I2C_TEST

int main() {
    // Initialize standard I/O (for printf over USB)
    stdio_init_all();
    
    // This delay is CRITICAL. It gives your computer time
    // to connect to the new USB serial port.
    sleep_ms(4000); // 4-second delay

    printf("=====================================\n");
    printf("Starting GPS I2C NMEA Test...\n");
    printf("=====================================\n");

    // Initialize the GPS driver (which also inits i2c1)
    gps_init();
    printf("GPS driver initialized for I2C on i2c1 (GPIO 38/39).\n");
    printf("Polling for NMEA sentences from NEO-M10 at 0x%02X:\n\n", 0x42);

    // Main application loop
    while (true) {
        // This function polls for data, prints it, and parses it.
        gps_update();

        // We need a small delay so we don't spam the I2C bus
        // and flood the terminal.
        sleep_ms(10); 
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


#ifdef LCD_SPI_TEST

int main() {

    return 0;
}

#endif