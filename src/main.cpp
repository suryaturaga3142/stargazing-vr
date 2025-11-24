/*******************************************************************************
 * @file        main.cpp
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
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "watchdog.h"

#include "config.h"
#include "structs.h"

#include "display.h"
#include "gps.h"
#include "imu.h"
#include "sd_card.h"

#include "mechanics.h"
#include "monitor.h"
#include "rendering.h"
#include "user_ui.h"

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */

int main()
{
    // PHASE 1: Power On Setup
    stdio_init_all();

    user_ui_init();
    // Do something like an LED flash if rebooting happened from watchdog.
    // Setup watchdog with 5sec timeout during startup procedures. Pet it during long processes.
    watchdog_enable(5000, true);

    // PHASE 2: Connectivity Check
    sd_init();     // Initialize and check SD Card presence only. Do NOT mount or access data yet.
    // If not present, run a 10 second warning while polling sd_check()

    imu_init();     // Check IMU presence and initialize
    display_init(); // Initialize PIO related stuff, it'll be a fast function.

    // PHASE 3: Heavy Lifting (PET THE WATCHDOG MANY TIMES!)
    sd_load_data(); // Mount and bulk read the SD card data
    sd_buf_sort();  // Sorts data in 3 pass algorithm
    sd_deinit();    // Unmount the SD card
    gps_init();     // Initialize GPS module and enable GNRMC. Sync loc/RTC or use default
    
    // PHASE 4: Handover process
    // Set complete watchdog health
    // Enable all other interrupts and timers remaining like rendering
    // Set final RGB status
    // Reconfigure watchdog for main loop checking

    // Loop
    while (true) {
    }

    // Should never reach here.
    for(;;) tight_loop_contents();

    return 0;
}