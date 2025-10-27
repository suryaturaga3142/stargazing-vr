/*******************************************************************************
 * @file        globals.h
 * @brief       List of all global declarations used in the project.
 * @details     Contains important and relevant declarations for all files.
 * 
 * @see         globals.c for implementation details.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * @version     1.0
 ******************************************************************************/

#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "structs.h"

// --- Star Catalog Data Structures ---
// These large arrays are defined in globals.c and hold the pre-processed star data.
extern Star_t all_stars[STAR_CATALOG_SIZE_MAX];
extern SkyPatch_t sky_database[SKY_PATCH_RA_DIVISIONS][SKY_PATCH_DEC_DIVISIONS];
extern uint32_t g_star_count;                           // Total number of stars in the array

// --- Real-Time Shared Data ---
// Volatile variables to safely share data between high-frequency ISRs and the main application.
extern volatile IMUData_t g_latest_imu_data;      // Updated by the 100Hz IMU ISR.
extern volatile JulianDate_t g_current_time_jd;   // The high-precision master simulation clock.

// --- Low-Priority Shared Data ---
// Volatile variables for background tasks.
extern volatile GPSData_t g_latest_gps_data;            // Updated by the main loop from GPS data.

// --- System State Flags ---
// Volatile flags used by ISRs to communicate events to the main loop or other ISRs.
extern volatile bool g_is_rendering;              // Prevents render ISR overruns.
extern volatile bool g_toggle_mode_request;       // Set by button ISR, handled by main.
extern volatile bool g_gps_correction_needed;     // Set by RTC alarm, handled by main.
extern volatile bool g_use_gps_location;          // Toggles between GPS and J2000 reference.
extern volatile uint8_t g_watchdog_checkin_flags; // Used by ISRs and the watchdog supervisor.

#endif /* GLOBALS_H */