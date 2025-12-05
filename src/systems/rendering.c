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
#define AMOUNT_OF_STARS 9000
// ...

/* ----------------------------- Private Variables -------------------------- */
static volatile bool g_is_rendering = false;
// --- Global Star Data Definitions ---
// DEFINITIONS for the externally linked buffers (used by main/test)
// static uint32_t g_star_coords_current[AMOUNT_OF_STARS]; 
// static uint32_t g_star_coords_previous[AMOUNT_OF_STARS]; 
// static uint16_t g_star_magnitudes[AMOUNT_OF_STARS]; 

// --- Local Control Variables ---
static StarPosition_t buffer_one[AMOUNT_OF_STARS];
static StarPosition_t buffer_two[AMOUNT_OF_STARS];

int selector = 1;

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
 * @brief Builds the SPI transfer sequence for a single star and executes the bursts.
 * @param buffer The coordinate buffer.
 * @param magnitudes The magnitude buffer.
 * @param index The star index to process.
 * @param mode COLOR_BLACK (erase) or a different color (draw).
 */
// static void execute_star_transfer(uint32_t buffer[AMOUNT_OF_STARS], uint16_t magnitudes[AMOUNT_OF_STARS], int index, uint16_t mode)
// {
//     // *** FIX 2: Wait for the previous DMA burst to finish ***
//     // This is crucial to ensure the SPI peripheral is free before sending the next command.
//     display_dma_wait_for_finish(); 

//     // The total data buffer for a single star (Address Parameters + Pixel Data)
//     uint8_t static_star_packet[BYTES_PER_STAR_PACKET];
    
//     // 1. Fill a static buffer with the address and pixel data for this star.
//     size_t data_length = buffer_star_data(buffer, magnitudes, mode, index);
    
//     if (data_length < 26) return; 

//     const uint8_t *data_ptr = static_star_packet;
    
//     // *** CASET (Column Address Set) Burst ***
    
//     // 2a. Send 0x2A Command Byte (D/C = 0)
//     display_set_dc(false); // Command Mode
//     display_spi_blocking((const uint8_t[]){0x2A}, 1); 
    
//     // 2b. Send 4 bytes of X-address parameters (D/C = 1)
//     display_set_dc(true); // Data Mode
//     display_spi_blocking(data_ptr, 4); // SC[15:0], EC[15:0]
//     data_ptr += 4;
    
//     // *** PASET (Page Address Set) Burst ***
    
//     // 3a. Send 0x2B Command Byte (D/C = 0)
//     display_set_dc(false); // Command Mode
//     display_spi_blocking((const uint8_t[]){0x2B}, 1);
    
//     // 3b. Send 4 bytes of Y-address parameters (D/C = 1)
//     display_set_dc(true); // Data Mode
//     display_spi_blocking(data_ptr, 4); // SP[15:0], EP[15:0]
//     data_ptr += 4;
    
//     // *** RAMWR (Memory Write) Burst ***
    
//     // 4a. Send 0x2C Command Byte (D/C = 0)
//     display_set_dc(false); // Command Mode
//     display_spi_blocking((const uint8_t[]){0x2C}, 1);
    
//     // 4b. Send 18 bytes of Pixel Data (D/C = 1) using DMA
//     display_set_dc(true); // Data Mode
//     display_dma_burst(data_ptr, 18); // 9 pixels * 2 bytes/pixel
    
//     // NOTE: The function now returns immediately after starting the DMA, 
//     // allowing the CPU to proceed to the next star's logic, while the 
//     // DMA controller handles the 18-byte pixel transfer.
// }



/**
 * @brief The center of this program. Goes through the actual rendering process.
 * 
 * @return true if rendering of stars was successful.
 */
bool run_main_render(void) {
    if (g_is_rendering) return false;
    g_is_rendering = true;

    Clear_The_star();
    // Local selector for the inner logic block (RENAMED)
    int counter = 0;
    // Phase 1: Setup quaternions
    
    Quaternion_t q_imu = g_latest_imu_data.orientation;
    q_imu.y = -q_imu.y;
    Quaternion_t q_fix_calc; // The one to use in calculation
    
    if (g_use_gps_location) q_fix_calc = Qfix_last.total;
    else                    q_fix_calc = Qfix_default.total;

    // If the displayed data is going the wrong way, change this to use q_imu instead of the conjugate
    Quaternion_t q_final = mech_normalize_q(mech_product_q(q_imu, q_fix_calc));

    //printf("Q final: w=%f x=%f y=%f z=%f\r\n", q_final.w, q_final.x, q_final.y, q_final.z);

    // Phase 2: Spatial culling

    Vector3f_t perspective_vector = mech_rotate_v(mech_conjugate_q(q_final), (Vector3f_t) {0.0f, 1.0f, 0.0f});


    // f = Focal Length related to FOV (e.g., 1.0 / tan(fov/2))
    float f = -1.0f / tanf(40.0f * M_PI / 180.0f); // 100 deg FOV
    int ra_choice = mech_v_to_ra_bin(perspective_vector);
    int dec_choice = mech_v_to_dec_bin(perspective_vector);

    //printf("xx\r\n");

    for (int dec_i = dec_choice - 1; dec_i < dec_choice + 2; dec_i++) {

        if (dec_i < 0 || dec_i >= SKY_PATCH_DEC_DIVISIONS) continue;
        int ra_l = ra_choice - 1;
        int ra_h = ra_choice + 2;

        // Special cases for poles to include all RA patches
        if (dec_i == 0 || dec_i == SKY_PATCH_DEC_DIVISIONS - 1) {
            ra_l = 0;
            ra_h = SKY_PATCH_RA_DIVISIONS;
        }
        else if (dec_i == 1 || dec_i == SKY_PATCH_DEC_DIVISIONS - 2) {
            ra_l = ra_choice - 2;
            ra_h = ra_choice + 3;
        }

        for (int ra_i = ra_l; ra_i < ra_h; ra_i++) {

            int ra = (ra_i % SKY_PATCH_RA_DIVISIONS + SKY_PATCH_RA_DIVISIONS) % SKY_PATCH_RA_DIVISIONS;
            int dec = dec_i;

            for (int idx = 0; idx < sky_database[ra][dec].star_count; idx++) {
                Star_t star = all_stars[sky_database[ra][dec].start_index + idx];
                Vector3f_t pt = mech_rotate_v(q_final, mech_star_to_vec(star));

                //printf("Star Vector Originl %d: x=%f y=%f z=%f\r\n", i, star.x, star.y, star.z);
                //printf("Star Vector Rotated %d: x=%f y=%f z=%f\r\n", i, pt.x, pt.y, pt.z);

                if (pt.y <= 0.0f) continue;

                float x_proj = (pt.x / pt.y) * f;
                float z_proj = (pt.z / pt.y) * f;

                if (x_proj >= -1.0f && x_proj <= 1.0f && z_proj >= -1.0f && z_proj <= 1.0f) {
                    // Scale and cast
                    int16_t x_int = (int16_t)(x_proj * 420.0f); // Horizontal coordinate relative to center
                    int16_t z_int = (int16_t)(z_proj * 630.0f); // Vertical coordinate relative to center
                    uint8_t m_int = 255; //(uint8_t)(star.mag * 10.0f); // Magnitude of star (a lower number is brighter)

                    // Add these coordinates to a list
                    // Implement for Ryan: Use double  buffering to store. Erase the previous list and store in the new one.
                    //These are the coordinate list of where to draw and erase the stars
                    if(counter < AMOUNT_OF_STARS)
                    {
                        StarPosition_t *current_star;
                        StarPosition_t *previous_star;
                        if(selector)
                        {
                            current_star = &buffer_one[counter];
                            previous_star = &buffer_two[counter];
                            buffer_two[counter] = buffer_one[counter];
                            //printf("%5d  %5d\r\n", x_int, z_int);
                        }
                        else
                        {
                            current_star = &buffer_two[counter];
                            previous_star = &buffer_one[counter];
                            buffer_one[counter] = buffer_two[counter];
                            //magnitude[counter] = m_int;
                        }
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
    watchdog_update();

    if(selector)
    {
        erase_stars(buffer_two, counter);
        draw_stars(buffer_one, counter);
    }
    else
    {
        erase_stars(buffer_one, counter);
        draw_stars(buffer_two, counter);
    }

    selector = selector ^ 1;


    //sleep_ms(10); // Simulate the heavy rendering load. This also tests the g_is_rendering flag. Output speed will auto adjust
    g_is_rendering = false;

    return true;
}