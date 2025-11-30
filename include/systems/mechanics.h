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

extern volatile bool g_use_gps_location;          // Toggles between GPS and J2000 reference.

#ifdef __cplusplus
extern "C" {
#endif

Quaternion_t mech_conjugate_q(Quaternion_t q);
Quaternion_t mech_product_q(Quaternion_t q1, Quaternion_t q2);
Quaternion_t mech_location_to_q(void);
Quaternion_t mech_time_to_q(void);

#ifdef __cplusplus
}
#endif


#endif /* MECHANICS_H */