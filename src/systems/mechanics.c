/*******************************************************************************
 * @file        mechanics.c
 * @brief       Implements the functionality for the quaternion rotations.
 * @details     Complete set of functions to use CMSIS-DSP for rotations.
 *              Basically more extensive wrapper functions for CMSIS-DSP.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * 
 * @note        This module is designed to be used as assistance.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "mechanics.h"
#include <math.h>
// ...

/* ---------------------------- Private Constants --------------------------- */
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#define HALF_PI 1.5707963267948966f // M_PI / 2.0
#define SECONDS_PER_DAY 86400.0
#define SIDEREAL_RATE_PER_DAY 1.00273790935 // Ratio of sidereal to mean solar day

// Constants for GMST calculation (simplified formula, uses J2000 epoch)
#define JD_2000_0 2451545.0
#define SIDEREAL_TIME_2000_0_RAD 1.752179611f // GMST at JD 2000.0 (in radians)
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */

/**
 * @brief Converts calendar date/time to Julian Day (JD).
 *
 * @param time The structured UTC date and time.
 * @return JulianDate_t The corresponding Julian Date.
 */
static JulianDate_t mech_utc_to_julian(UTCTime_t time) {
    int32_t Y = time.year;
    int32_t M = time.month;
    int32_t D = time.day;
    float   H = (float)time.hour + (float)time.minute / 60.0f + (float)time.second / 3600.0f;

    // Standard to handle years 1900-2100 correctly
    if (M <= 2) {
        Y--;
        M += 12;
    }

    int32_t A = Y / 100;
    int32_t B = 2 - A + (A / 4); // Gregorian Calendar Correction

    double jd_integer = 365.25 * (Y + 4716) + 30.6001 * (M + 1) + D + B - 1524.5;
    double jd_fraction = H / 24.0;
    
    JulianDate_t jd = {
        .jd = jd_integer + jd_fraction
    };
    return jd;
}

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
 * @return sidereal_time Time relative to stars
 */
SideReal_t mech_utc_to_sidereal(UTCTime_t current_time) {
    // 1. Calculate Julian Date (JD) for the current UTC
    JulianDate_t jd_current = mech_utc_to_julian(current_time);
    
    // 2. Calculate the number of days since the J2000 epoch (J2000.0)
    // T_D = JD - 2451545.0
    double T_D = jd_current.jd - JD_2000_0;
    
    // 3. Calculate GMST in hours (simplified model)
    // GMST_hours = 6.697374558 + 0.06570982441908 * T_D + 1.00273790935 * UT
    // Where UT is the fractional part of the day (UTC in hours / 24)
    double UT = (double)current_time.hour + (double)current_time.minute / 60.0 + (double)current_time.second / 3600.0;
    UT /= 24.0; // Fractional part of the day

    // A simplified but accurate formula for GMST
    double GMST_0_hours = 6.697374558 + (0.06570982441908 * T_D);
    
    // Add time elapsed since 0h UT (corrected for sidereal rate)
    double GMST_hours = GMST_0_hours + (SIDEREAL_RATE_PER_DAY * (UT * 24.0));
    
    // Normalize to [0, 24) hours
    GMST_hours = fmod(GMST_hours, 24.0);
    if (GMST_hours < 0) {
        GMST_hours += 24.0;
    }
    
    // 4. Convert GMST from hours to radians
    double gmst_radians = (GMST_hours / 24.0) * (2.0 * M_PI);
    
    SideReal_t sidereal_time = {
        .st = gmst_radians
    };
    
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
    
    // --- 1. Rotation Q_pitch (Latitude Tilt) ---
    // The celestial pole (+Z) must be rotated by an angle alpha = (90 deg - Latitude)
    // to bring it to the local zenith (which is 90 deg above the horizon).
    float alpha = HALF_PI - latitude; // Rotation angle around the Y-axis
    float sin_alpha_half = sinf(alpha / 2.0f);
    float cos_alpha_half = cosf(alpha / 2.0f);

    Quaternion_t q_pitch = {
        .w = cos_alpha_half,
        .x = 0.0f,
        .y = sin_alpha_half, // Y-axis rotation
        .z = 0.0f
    };

    // --- 2. Rotation Q_yaw (Longitude) ---
    // The rotation around the Z-axis is directly the longitude angle.
    float theta = longitude; // Rotation angle around the Z-axis
    float sin_theta_half = sinf(theta / 2.0f);
    float cos_theta_half = cosf(theta / 2.0f);

    Quaternion_t q_yaw = {
        .w = cos_theta_half,
        .x = 0.0f,
        .y = 0.0f,
        .z = sin_theta_half // Z-axis rotation
    };

    // --- 3. Combine: Q_loc = Q_yaw * Q_pitch (Last rotation applied first) ---
    // We assume latitude tilt (Q_pitch) happens first, so it is the right-hand term.
    q_loc = mech_product_q(q_yaw, q_pitch); 
    
    return q_loc;
}

/**
 * @brief Converts time into a quaternion
 * 
 * @param time SideReal_t of just the time of day
 * @return q_time rotation quaternion for this time
 */
Quaternion_t mech_time_to_q(SideReal_t time) {

    // The rotation angle is the full GMST angle.
    float angle_rad = (float)time.st; 
    
    // Quaternions use half the angle
    float half_angle = angle_rad / 2.0f;
    
    // Optimization: Since this is a Z-axis rotation, x and y are zero.
    float sin_half = sinf(half_angle);
    float cos_half = cosf(half_angle);
    
    Quaternion_t q_time = {
        .w = cos_half,
        .x = 0.0f,
        .y = 0.0f,
        .z = sin_half // Z-axis rotation
    };
    
    return q_time;
}

/**
 * @brief Finds conjugate of the quaternion
 * 
 * @param q Generic quaternion
 * @return q_conj Conjugate of q
 */
Quaternion_t mech_conjugate_q(Quaternion_t q) {
    
    Quaternion_t q_conj = {
        .w = q.w,
        .x = -q.x,
        .y = -q.y,
        .z = -q.z
    };

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
    Quaternion_t q_prod = {
        // Scalar (w) part: w1*w2 - v1 . v2
        .w = (q1.w * q2.w) - (q1.x * q2.x) - (q1.y * q2.y) - (q1.z * q2.z),
        
        // Vector (x, y, z) part: w1*v2 + w2*v1 + v1 x v2
        
        // x part (i): w1*x2 + x1*w2 + y1*z2 - z1*y2
        .x = (q1.w * q2.x) + (q1.x * q2.w) + (q1.y * q2.z) - (q1.z * q2.y),
        
        // y part (j): w1*y2 - x1*z2 + y1*w2 + z1*x2
        .y = (q1.w * q2.y) - (q1.x * q2.z) + (q1.y * q2.w) + (q1.z * q2.x),
        
        // z part (k): w1*z2 + x1*y2 - y1*x2 + z1*w2
        .z = (q1.w * q2.z) + (q1.x * q2.y) - (q1.y * q2.x) + (q1.z * q2.w)
    };
    return q_prod;
}

/**
 * @brief Returns the rotation v' = q v q*
 * 
 * @param q Rotation Quaternion
 * @param v Location vector
 * @return v_p Rotated vector
 */
Vector3f_t mech_rotate_v(Quaternion_t q, Vector3f_t v) {

    // 1. Create a pure quaternion V from the vector v (V = 0 + v.x*i + v.y*j + v.z*k)
    // The scalar part (w) is zero for a pure vector quaternion.
    Quaternion_t q_v = {.w = 0.0f, .x = v.x, .y = v.y, .z = v.z};

    // 2. Calculate the conjugate Q* (or Q_inv)
    Quaternion_t q_conj = mech_conjugate_q(q);

    // 3. Calculate intermediate product P = V * Q*
    Quaternion_t q_p_intermediate = mech_product_q(q_v, q_conj);

    // 4. Calculate final product V' = Q * P (i.e., Q * (V * Q*))
    Quaternion_t q_v_prime = mech_product_q(q, q_p_intermediate);

    // 5. Extract the vector part (x, y, z)
    // The scalar part (w) of q_v_prime should be 0 (or near zero) after this operation.
    Vector3f_t v_p = {
        .x = q_v_prime.x,
        .y = q_v_prime.y,
        .z = q_v_prime.z
    };

    return v_p;
}