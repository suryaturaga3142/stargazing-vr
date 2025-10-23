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
#include "gps.h"
// #include "minmea.h" // Assumes minmea.h is in the src/ dir or include path

// We get all hardware defs from your config.h
// #include "config.h" 

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
// #include <string.h> // For strcpy

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */




int main() 
{
    stdio_init_all();
    gps_init();

    for(;;) {
        gps_update();
    }

    return 0;
}