/*******************************************************************************
 * @file        mechanics.h
 * @brief       Library to declare all quaternion mechanics.
 * @details     Handles CMSIS derived functions for rotations and working with
 *              backend of rendering.
 * 
 * @see         mechanics.c for implementation details.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * @version     1.0
 ******************************************************************************/

#ifndef MECHANICS_H
#define MECHANICS_H

#include <stdbool.h>
#include "structs.h"
#include "config.h"

// --- Star Catalog Data Structures ---
// These large arrays hold the pre-processed star data.
extern Star_t all_stars[STAR_CATALOG_SIZE_MAX];
extern SkyPatch_t sky_database[SKY_PATCH_RA_DIVISIONS][SKY_PATCH_DEC_DIVISIONS];

extern volatile JulianDate_t g_current_time_jd;   // The high-precision master simulation clock.

extern volatile bool g_use_gps_location;          // Toggles between GPS and J2000 reference.

#ifdef __cplusplus
extern "C" {
#endif

bool mechanics_init(void);


#ifdef __cplusplus
}
#endif


#endif /* MECHANICS_H */