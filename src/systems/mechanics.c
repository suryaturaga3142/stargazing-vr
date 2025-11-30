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

// ...

/* ----------------------------- Private Functions -------------------------- */

// ...

/* ----------------------------- Public Variables -------------------------- */

// --- Star Catalog Data Structure Definitions ---
// The actual memory for our star catalog and spatial culling grid is allocated here.
Star_t all_stars[STAR_CATALOG_SIZE_MAX];
SkyPatch_t sky_database[SKY_PATCH_RA_DIVISIONS][SKY_PATCH_DEC_DIVISIONS];

volatile bool g_use_gps_location = true; // Default to using GPS location on startup

/* ----------------------------- Public Functions --------------------------- */

/**
 * @brief Converts regular time into sidereal time
 * 
 * @param current_time Complete packaged date time and year.
 * @return sidereal_time Time relative to stars (only valid till hours)
 */
UTCTime_t mech_utc_to_sidereal(UTCTime_t current_time) {
    UTCTime_t sidereal_time;
    return sidereal_time;
}

/**
 * @brief Converts latitude and longitude into a quaternion
 * 
 * @param latitude User GPS latitude
 * @param longitude User GPS longitude
 * @return q_loc rotation quaternion for this location
 */
Quaternion_t mech_location_to_q(float latitude, float longitude) {
    Quaternion_t q_loc;
    return q_loc;
}

/**
 * @brief Converts time into a quaternion
 * 
 * @param time UTCTime_t of just the time of day
 * @return q_time rotation quaternion for this time
 */
Quaternion_t mech_time_to_q(UTCTime_t time) {
    Quaternion_t q_time;
    return q_time;
}

/**
 * @brief Finds conjugate of the quaternion
 * 
 * @param q Generic quaternion
 * @return q_conj Conjugate of q
 */
Quaternion_t mech_conjugate_q(Quaternion_t q) {
    Quaternion_t q_conj;
    return q_conj;
}

/**
 * @brief Finds product q_prod = q1 * q2
 * 
 * @param q1 First quaternion
 * @param q2 Second quaternion
 * @return q_prod Product result (watch of for order)
 */
Quaternion_t mech_product_q(Quaternion_t q1, Quaternion_t q2) {
    Quaternion_t q_prod;
    return q_prod;
}