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
#include "lcd.h"
#include "pico/stdlib.h"
#include "hardware/watchdog.h"
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

#define AMOUNT_OF_STARS 5000
#define MAG_LIMIT      6.5f   // Dimmest visible star (Value -> 0)
#define MAG_BRIGHTEST -1.5f   // Brightest reference star (Value -> MAX)
#define SCALE_FACTOR   (1.0f / (MAG_LIMIT - MAG_BRIGHTEST))
// ...

/* ----------------------------- Private Variables -------------------------- */

static volatile bool g_is_rendering = false;
//volatile bool pause_button = false;
//volatile bool unpause_button = false;

// --- Global Star Data Definitions ---
// DEFINITIONS for the externally linked buffers (used by main/test)
// static uint32_t g_star_coords_current[AMOUNT_OF_STARS]; 
// static uint32_t g_star_coords_previous[AMOUNT_OF_STARS]; 
// static uint16_t g_star_magnitudes[AMOUNT_OF_STARS]; 

// --- Local Control Variables ---
static StarPosition_t buffer_one[AMOUNT_OF_STARS];
static StarPosition_t buffer_two[AMOUNT_OF_STARS];

static int selector = 1;
static int last_frame_star_count = 0;

/* ----------------------------- Private Functions -------------------------- */

static uint8_t mag_to_brightness_u8(float mag) {
    // 1. Check bounds
    if (mag >= MAG_LIMIT) return 0;       // Too dim
    if (mag <= MAG_BRIGHTEST) return 255; // Too bright (clamp to max)

    // 2. Linear Scaling
    // Formula: (Limit - Mag) * Scale * 255
    float val = (MAG_LIMIT - mag) * SCALE_FACTOR * 255.0f;

    return (uint8_t)val;
}

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

    if (g_pause_toggle_request) {
        g_play_screen = !g_play_screen;

        if (g_play_screen) {
            LCD_Clear(0x0000);
            last_frame_star_count = 0;
        }
        /*else {
            beautiful_background();
            if(selector) draw_paused_stars(buffer_two, last_frame_star_count);
            else         draw_paused_stars(buffer_one, last_frame_star_count);
        }*/

        g_pause_toggle_request = false;
    }

    if (!g_play_screen) {
        g_is_rendering = false;
        return false;
    }

    // Local selector for the inner logic block (RENAMED)
    int counter = 0;

    // Phase 1: Setup quaternions
    
    Quaternion_t q_imu = g_latest_imu_data.orientation;
    q_imu.y = q_imu.y;
    q_imu.z = -q_imu.z;
    Quaternion_t q_fix_calc; // The one to use in calculation
    
    if (g_use_gps_location) q_fix_calc = Qfix_last.total;
    else                    q_fix_calc = Qfix_default.total;

    // If the displayed data is going the wrong way, change this to use q_imu instead of the conjugate
    Quaternion_t q_final = mech_normalize_q(mech_product_q(q_imu, q_fix_calc));

    //printf("Q final: w=%f x=%f y=%f z=%f\r\n", q_final.w, q_final.x, q_final.y, q_final.z);

    // Phase 2: Spatial culling

    Vector3f_t perspective_vector = mech_rotate_v(mech_conjugate_q(q_final), (Vector3f_t) {0.0f, 1.0f, 0.0f});


    // f = Focal Length related to FOV (e.g., 1.0 / tan(fov/2))
    float f = -1.0f / tanf(50.0f * M_PI / 180.0f); // 100 deg FOV
    int ra_choice = mech_v_to_ra_bin(perspective_vector);
    int dec_choice = mech_v_to_dec_bin(perspective_vector);

    //printf("xx\r\n");

    for (int dec_i = dec_choice - 1; dec_i < dec_choice + 2; dec_i++) {

        if (dec_i < 0 || dec_i >= SKY_PATCH_DEC_DIVISIONS) continue;
        int ra_l = ra_choice - 3;
        int ra_h = ra_choice + 4;

        // Special cases for poles to include all RA patches
        if (dec_i == 0 || dec_i == SKY_PATCH_DEC_DIVISIONS - 1) {
            ra_l = 0;
            ra_h = SKY_PATCH_RA_DIVISIONS;
        }
        else if (dec_i == 1 || dec_i == SKY_PATCH_DEC_DIVISIONS - 2) {
            ra_l = ra_choice - 5;
            ra_h = ra_choice + 6;
        }
        else if (dec_i == 2 || dec_i == SKY_PATCH_DEC_DIVISIONS - 3) {
            ra_l = ra_choice - 3;
            ra_h = ra_choice + 4;
        }

        for (int ra_i = ra_l; ra_i < ra_h; ra_i++) {

            int ra = (ra_i % SKY_PATCH_RA_DIVISIONS + SKY_PATCH_RA_DIVISIONS) % SKY_PATCH_RA_DIVISIONS;
            int dec = dec_i;

            for (int idx = 0; idx < sky_database[ra][dec].star_count; idx++) {
                Star_t star = all_stars[sky_database[ra][dec].start_index + idx];
                Vector3f_t pt = mech_rotate_v(q_final, mech_star_to_vec(star));

                //printf("Star Vector Originl %d: x=%f y=%f z=%f\r\n", i, star.x, star.y, star.z);
                //printf("Star Vector Rotated %d: x=%f y=%f z=%f\r\n", i, pt.x, pt.y, pt.z);

                if (pt.y <= 0.1f) continue;

                float x_proj = (pt.x / pt.y) * f;
                float z_proj = (pt.z / pt.y) * f;
                // Scale and cast
                if (x_proj >= -1.0f && x_proj <= 1.0f && z_proj >= -1.0f && z_proj <= 1.0f) {

                    int16_t x_int = (int16_t)(x_proj * 420.0f); // Horizontal coordinate relative to center
                    int16_t z_int = (int16_t)(z_proj * 630.0f); // Vertical coordinate relative to center

                    if (x_int >= -X_HALF && x_int <= X_HALF && z_int >= -Y_HALF && z_int <= Y_HALF) {
                        uint8_t m_int =  mag_to_brightness_u8(star.mag); 

                        // Add these coordinates to a list and use double buffering
                        if(counter < AMOUNT_OF_STARS) {
                            StarPosition_t *current_star;
                            
                            if(selector) current_star = &buffer_one[counter];
                            else         current_star = &buffer_two[counter];

                            current_star->x_proj = x_int;
                            current_star->z_proj = z_int;
                            current_star->magnitude = m_int;

                            counter++;
                        }
                        // Print as Hex: $XXXXYYYMMM
                        // %04X for 16-bit, %02X for 8-bit
                        //printf("$%04X%04X%02X\n", (uint16_t)x_int, (uint16_t)z_int, m_int);
                    }
                }
            }
        }
    }
    //watchdog_update();

    if(selector) {
        erase_stars(buffer_two, last_frame_star_count);
        draw_stars(buffer_one, counter);
    }
    else {
        erase_stars(buffer_one, last_frame_star_count);
        draw_stars(buffer_two, counter);
    }

    last_frame_star_count = counter;
    selector = selector ^ 1;

    //sleep_ms(50);

    g_is_rendering = false;

    return true;
}