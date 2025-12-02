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

#ifdef __cplusplus
extern "C" {
#endif

SideReal_t   mech_utc_to_sidereal(UTCTime_t current_time);
Quaternion_t mech_location_to_q(float latitude, float longitude);
Quaternion_t mech_time_to_q(SideReal_t time);
Vector3f_t   mech_star_to_vec(Star_t star);
Quaternion_t mech_normalize_q(Quaternion_t q);
Quaternion_t mech_conjugate_q(Quaternion_t q);
Quaternion_t mech_product_q(Quaternion_t q1, Quaternion_t q2);
Vector3f_t   mech_rotate_v(Quaternion_t q, Vector3f_t v);

#ifdef __cplusplus
}
#endif


#endif /* MECHANICS_H */