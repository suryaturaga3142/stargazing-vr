/*******************************************************************************
 * @file        gps.c
 * @brief       Implements the complete functionality for the GPS module.
 * @details     Coding the driver for the NEO M10 GPS. Uses UART blocking for 
 *              sending UBX commands and receiving NMEA sentences. 
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * 
 * @note        This module is designed to be taken care of by interrupts or
 *              main. Use main for long blocking functions.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "gps.h"
#include "interrupts.h"
#include "monitor.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include <stdio.h>
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Variables --------------------------- */
GPSData_t g_latest_gps_data = {0};

volatile bool g_gps_correction_needed = false;

/* ----------------------------- Public Functions --------------------------- */

/**
 * @brief Initialize the GPS module and enable the location and date/time
 * 
 * @return true if initialization was successful
 */
bool gps_init(void) {
    // Setup the UART peripheral for GPS module.
    gpio_init(PIN_GPS_TX);
    gpio_init(PIN_GPS_RX);
    gpio_set_function(PIN_GPS_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_GPS_RX, GPIO_FUNC_UART);
    uart_init(UART_PORT, GPS_UART_BAUD);
    uart_set_hw_flow(UART_PORT, false, false);
    uart_set_format(UART_PORT, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_PORT, true);

    // Send UBX commands to configure the GPS module to output GNRMC sentences.
    // OPTIONAL

    // Setup a timer with interrupt to poll GPS data once a minute
    timer0_hw->inte |= TIMER_INTE_ALARM_1_BITS;
    irq_set_exclusive_handler(TIMER0_IRQ_1, irq_timer_gps_callback);
    irq_set_enabled(TIMER0_IRQ_1, true);
    timer0_hw->alarm[1] = timer_hw->timerawl + (GPS_CORRECTION_INTERVAL_MIN * 60 * 1000);

    return true;
}

/**
 * @brief Checks the global flag for needing new GPS data and reads if needed. Called by main only.
 * 
 * @return true if data was updated successfully.
 */
bool gps_check_and_read(void) {

    if (!g_gps_correction_needed) {
        return false;
    }

    g_gps_correction_needed = false;
    // Read NMEA sentences from UART and parse GNRMC for location and date/time.
    // Update g_latest_gps_data accordingly. return true only if successful.
    /*char buf[200];
    uart_read_blocking(UART_PORT, buf, 200);
    watchdog_update();
    printf("\r\nNEW\r\n%s\r\n", buf);
    */
    return true;
}