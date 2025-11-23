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
#include "hardware/watchdog.h"

#include "config.h"
#include "globals.h"
#include "structs.h"
#include "startup.h"

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */

int main()
{
    // PHASE 1: Power On
    stdio_init_all();
    startup_peripherals();
    //Note: Must "pet" this watchdog inside any subsequent long wait loops (like the SD card retry loop).
    watchdog_enable(5000, true);
    startup_ui();

    // PHASE 2: Mounting devices
    startup_check_sd();
    startup_imu();
    startup_lcd();


    // PHASE 3: Time intensive tasks
    startup_sd();
    startup_gps();

    // PHASE 4: Handover to main


    // Loop
    while (true) {
        tight_loop_contents();
    }

    for(;;) {
        // Should never reach here.
    }

    return 0;
}