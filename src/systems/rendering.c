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
#include "mechanics.h"
#include "imu.h"
#include "gps.h"
#include "pico/stdlib.h"
// ...

/* ---------------------------- Private Constants --------------------------- */
// Precalculated quaternion for location at PWL in J2000
static const Quaternion_t q_location_pwl_j2000 = {0.7071f, 0.0f, 0.7071f, 0.0f};
// ...

/* ----------------------------- Private Variables -------------------------- */
static volatile bool g_is_rendering = false;
// Start at precalculated in case GPS is not available
static Quaternion_t q_location = q_location_pwl_j2000;
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

    // Phase 1: Setup quaternions
    
    Quaternion_t q_imu = g_latest_imu_data.orientation;
    Quaternion_t q_time;
    
    if (g_use_gps_location) {
        if (g_latest_gps_data.is_valid) {
            // call mechanics functions to find q_location from gps data
        }
        // else it defaults to last known good location / PWL so do nothing
    }
    else q_location = q_location_pwl_j2000;

    // call a mechanics function to get q_time from RTC clock

    // call a mechanics function to combine q_imu, q_location, q_time into a final quaternion
    // q_final = q_imu * q_location * q_time

    // Phase 2: Spatial culling
    // perspective_vector = q_final_conjugate * (0, 0, 1) * q_final
    // determine which sky patches are in view based on perspective_vector

    // Phase 3: Star projection and draw list formation
    // for each star in visible sky patches, rotate by q_final to orient to (0, 0, 1)
    // project onto 2D screen space-
    // x_proj = x_rotated * z_to_screen (z_rotated = 1)
    // y_proj = y_rotated * z_to_screen (z_rotated = 1)
    // Similar calculation for brightness based on star magnitude
    // add to draw list if within screen bounds

    // Phase 4: Send to display
    // trigger DMA to send to display through display.c functions to use PIO

    sleep_ms(50); // Simulate the heavy rendering load. This also tests the g_is_rendering flag. Output speed will auto adjust
    g_is_rendering = false;

    return true;
}