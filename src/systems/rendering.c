/*******************************************************************************
 * @file        rendering.c
 * @brief       Implements the functionality for the rendering module.
 * @details     Works in tandem with results of mechanics.h functions. The 
 *              rotations in mechanics allows for a single quaternion 
 *              calculation. This will be used here to remap the relevant
 *              stars through spatial culling, map them onto the screen,
 *              and formulating the command buffer for display.c to access.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * 
 * @note        This module is designed to give help with pixel mapping. Call
 *              only during pixel projection in interrupts.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "rendering.h"
#include "pico/stdlib.h"
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
static volatile bool g_is_rendering = false;
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Variables --------------------------- */

/* ----------------------------- Public Functions --------------------------- */

/**
 * @brief The center of this program. Goes through the actual rendering process.
 * 
 * @return true if rendering of stars was successful.
 */
bool run_main_render(void) {
    if (g_is_rendering) return false;

    g_is_rendering = true;
    // run all the calculation steps by calling mechanics functions
    // form the draw list
    // trigger DMA to send to display
    sleep_ms(50); // Simulate the heavy rendering load. This also tests the g_is_rendering flag. Output speed will auto adjust
    g_is_rendering = false;

    return true;
}