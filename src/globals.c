/*******************************************************************************
 * @file        globals.c
 * @brief       Implements definitions for all globals.
 * @details     Defines everything in the project that is required globally by
 *              all systems.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * 
 * @note        This module is designed to be a set of definitions and statics.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "globals.h"

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Variables -------------------------- */

// --- Star Catalog Data Structure Definitions ---
// The actual memory for our star catalog and spatial culling grid is allocated here.
Star_t all_stars[STAR_CATALOG_SIZE_MAX];
SkyPatch_t sky_database[SKY_PATCH_RA_DIVISIONS][SKY_PATCH_DEC_DIVISIONS];


// --- Real-Time Shared Data Definitions ---
volatile IMUData_t g_latest_imu_data = {0};
volatile JulianDate_t g_current_time_jd = {0.0};


// --- Low-Priority Shared Data Definitions ---
volatile GPSData_t g_latest_gps_data = {0};


// --- System State Flag Definitions ---
volatile bool g_is_rendering = false;
volatile bool g_drift_correct_request = false;
volatile bool g_toggle_mode_request = false;
volatile bool g_gps_correction_needed = false;
volatile bool g_use_gps_location = true; // Default to using GPS location on startup
volatile uint8_t g_watchdog_checkin_flags = 0;

/* ----------------------------- Public Functions --------------------------- */
// ...

