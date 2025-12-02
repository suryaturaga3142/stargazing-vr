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
#include "user_ui.h"
#include "display.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <math.h>
// ...

/* ---------------------------- Private Constants --------------------------- */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
// Precalculated quaternions for location at PWL in J2000
static const Qfix_t Qfix_default = {
    .loc =   {0.65922f, 0.28787f, 0.30386f, -0.62776f},
    .time =  {1.0f    , 0.0f    , 0.0f    ,  0.0f    },
    .total = {0.65922f, 0.28787f, 0.30386f, -0.62776f}
};
// ...

/* ----------------------------- Private Variables -------------------------- */
static volatile bool g_is_rendering = false;
// Started at precalculated in case GPS is not available
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Variables --------------------------- */

// --- Star Catalog Data Structures ---
// These large arrays hold the pre-processed star data.
Star_t all_stars[STAR_CATALOG_SIZE_MAX];
SkyPatch_t sky_database[SKY_PATCH_RA_DIVISIONS][SKY_PATCH_DEC_DIVISIONS];

Qfix_t Qfix_last = Qfix_default;

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
    Quaternion_t q_fix_calc; // The one to use in calculation
    
    if (g_use_gps_location) q_fix_calc = Qfix_last.total;
    else                    q_fix_calc = Qfix_default.total;

    // If the displayed data is going the wrong way, change this to use q_imu instead of the conjugate
    Quaternion_t q_final = mech_normalize_q(mech_product_q(q_imu, q_fix_calc));

    //printf("Q final: w=%f x=%f y=%f z=%f\r\n", q_final.w, q_final.x, q_final.y, q_final.z);

    // Phase 2: Spatial culling
    // perspective_vector = q_final_conjugate * (0, 0, 1) * q_final
    // determine which sky patches are in view based on perspective_vector

    Vector3f_t perspective_vector = mech_rotate_v(mech_conjugate_q(q_final), (Vector3f_t) {0.0f, 0.0f, 1.0f});


    // f = Focal Length related to FOV (e.g., 1.0 / tan(fov/2))
    float f = 1.0f / tanf(50.0f * M_PI / 180.0f); // 100 deg FOV

    for (int i = 0; i < 6; i++) {
        Star_t star = all_stars[sky_database[0][0].start_index + i];
        Vector3f_t pt = mech_rotate_v(q_final, mech_star_to_vec(star));

        //printf("Star Vector Originl %d: x=%f y=%f z=%f\r\n", i, star.x, star.y, star.z);
        //printf("Star Vector Rotated %d: x=%f y=%f z=%f\r\n", i, pt.x, pt.y, pt.z);

        if (pt.z <= 0.0f) continue;

        float x_proj = (pt.x / pt.z) * f;
        float y_proj = (pt.y / pt.z) * f;

        if (x_proj >= -1.0f && x_proj <= 1.0f && y_proj >= -1.0f && y_proj <= 1.0f) {
            // Scale and cast
            int16_t x_int = (int16_t)(x_proj * 32000.0f);
            int16_t y_int = (int16_t)(y_proj * 32000.0f);
            uint8_t m_int = (uint8_t)(star.mag * 10.0f);

            // Print as Hex: $XXXXYYYMMM
            // %04X for 16-bit, %02X for 8-bit
            printf("$%04X%04X%02X\n", (uint16_t)x_int, (uint16_t)y_int, m_int);
        }

    }
    //printf("\r\n");

    // Phase 3: Star projection and draw list formation
    // for each star in visible sky patches, rotate by q_final to orient to (0, 0, 1)
    // project onto 2D screen space-
    // x_proj = x_rotated * z_to_screen (z_rotated = 1)
    // y_proj = y_rotated * z_to_screen (z_rotated = 1)
    // Similar calculation for brightness based on star magnitude
    // add to draw list if within screen bounds

    // Phase 4: Send to display
    // trigger DMA to send to display through display.c functions to use PIO

    sleep_ms(100); // Simulate the heavy rendering load. This also tests the g_is_rendering flag. Output speed will auto adjust
    g_is_rendering = false;

    return true;
}