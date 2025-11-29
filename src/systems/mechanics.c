/*******************************************************************************
 * @file        mechanics.c
 * @brief       Implements the functionality for the quaternion rotations.
 * @details     Complete set of functions to use CMSIS-DSP for rotations.
 *              Basically more extensive wrapper functions for CMSIS-DSP.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * 
 * @note        This module is designed to be driven by interrupts and is not
 *              intended to be called from a blocking main loop. Use as needed.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "mechanics.h"
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
// A default GPS fix to use when no valid data is available. Upon intialization, find the default quaternion.
static GPSData_t default_fix = {
    .time = {2000, 1, 1, 0, 0, 0},
    .latitude = 0.0f,
    .longitude = 0.0f,
    .is_valid = false
};
static Quaternion_t q_default_fix;

static Quaternion_t q_actual_fix;

// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Variables -------------------------- */

// --- Star Catalog Data Structure Definitions ---
// The actual memory for our star catalog and spatial culling grid is allocated here.
Star_t all_stars[STAR_CATALOG_SIZE_MAX];
SkyPatch_t sky_database[SKY_PATCH_RA_DIVISIONS][SKY_PATCH_DEC_DIVISIONS];

volatile JulianDate_t g_current_time_jd = {0.0};

volatile bool g_use_gps_location = true; // Default to using GPS location on startup

/* ----------------------------- Public Functions --------------------------- */


/**
 * @brief A fast initialization of the default quaternions used in mechanics
 * 
 * @return true if initialization was successful.
 */
bool mechanics_init(void) {
    // Calculate the default fix quaternion from default_fix
    return true;
}