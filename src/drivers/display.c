/*******************************************************************************
 * @file        display.c
 * @brief       Implements the initialization for the LCD Display module.
 * @details     Very high level for initializing and passing draw commands. 
 *              All low level complexity handled by PIO.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * 
 * @note        This module is designed to be driven by interrupts and is not
 *              intended to be called from a blocking main loop. Only the 
 *              initialization is.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include <stdbool.h>
#include "display.h"
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */

/**
 * @brief Initialization of the LCD display with PIO and DMA
 * 
 * @return true if initialization was successful
 */
bool display_init(void) {
    // Initialize PIO, DMA, and any other low level stuff for driving the display.
    // This function should be fast and non-blocking. No verification is needed
    // Set the screen to dark blue.

    return true;
}