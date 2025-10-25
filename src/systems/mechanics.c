/*******************************************************************************
 * @file        mechanics.c
 * @brief       Implements the functionality for the quaternion rotations.
 * @details     Complete set of functions to use CMSIS-DSP for rotations.
 *              Basically more extensive wrapper functions for CMSIS-DSP.
 * 
 * @author      LED Chasers
 * @date        2025-10-25
 * 
 * @note        This module is designed to be driven by interrupts and is not
 *              intended to be called from a blocking main loop. Use as needed.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "mechanics.h"

#include "dsp/quaternion_math_functions.h"
#include <math.h>
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...
const float earth_angular_velocity = (7.292115e-5 * 180 / 3.14159265358979323846); //degrees per second
const Vector3f_t earth_axis = {0.0f, 0.0f, 1.0f}; //Assuming Z axis is Earth's rotation axis

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */

/**************************************************************************** */
/*                       Earth Rotation Functions                             */
/**************************************************************************** */
/**
  @brief         Perform initial time and location rotations on star catalog
  @param[in]     current_date       current date in Julian Date format
  @param[in]     longitude          longitude of user in degrees
  @param[in]     latitude           latitude of user in degrees
*/
void rotate_stars_to_init_locations(JulianDate_t current_date, float longitude, float latitude)
{
    // Implement the algorithm to convert a UTC timestamp and longitude into Local Sidereal Time. 
    float LST = calculate_LST(current_date, longitude); //hours
    float rotation_seconds = 3600 * LST; //Get time difference from LST in seconds
    
    // Implement the logic to create the q_time and q_location quaternions. 
    Quaternion_t q_time = get_time_rotation_quaternion(rotation_seconds); //Quaternion representing rotation based on current day and time
    Quaternion_t q_location = get_location_rotation_quaternion(latitude); //Quaternion representing rotation based on current GPS location

    // Create a simple rendering loop that applies the combined q_location * q_time rotation to all 9,000 stars and draws them to the screen. 
    for (int i = 0; i < STAR_CATALOG_SIZE_MAX; i++)
    {
        Star_t star = all_stars[i];
        Quaternion_t q_star = {0.0f, star.x, star.y, star.z}; //Convert star to quaternion
        Quaternion_t q_rotated_star = apply_active_rotation(q_star, q_time); //Apply time rotation
        q_rotated_star = apply_active_rotation(q_rotated_star, q_location); //Apply latitude rotation
        Star_t rotated_star = {q_rotated_star.x, q_rotated_star.y, q_rotated_star.z, star.mag}; //Convert back to star
        all_stars[i] = rotated_star; //Updated star catalog with rotated star
    }

    //TODO: Add rendering to screen using SDL library for testing purposes

    return;
}

/**
  @brief         Perform initial time and location rotations on star catalog
  @param[in]     current_date       current date in Julian Date format
  @param[in]     longitude          longitude of user in degrees
  @param[in]     latitude           latitude of user in degrees
  @returns       LST (Local Sidereal Time) in hours
*/
float calculate_LST(JulianDate_t current_date, float longitude)
{
    // Convert Julian Date to D (days since J2000.0) and H (hours since 0h UT)
    int D = trunc(current_date.jd);
    float H = (current_date.jd - D) * 24.0;

    // Calculate GMST
    float GMST = (6.697374558 + (0.06570982441908 * D) + (1.00273790935 * H)); //hours;

    // Calculate LST
    float LST = GMST + longitude / 15.041; //Convert longitude to hours, 15.041 degrees per hour

    return LST;
}

/**
  @brief         Get quaternion to model the rotation of the Earth for a given amount of time
  @param[in]     rotation_seconds       The amount of time to rotate in seconds
  @returns       A quaternion representing the rotation
*/
Quaternion_t get_time_rotation_quaternion(float rotation_seconds)
{
    float angle = -1 * (rotation_seconds * earth_angular_velocity); //degrees
    Quaternion_t q_time = rotation_to_quaternion(angle, earth_axis);
    return q_time;
}

/**
  @brief         Get quaternion to model the rotation from global to horizon frame based on latitude
  @param[in]     latitude      The latitude to rotate to in degrees
  @returns       A quaternion representing the rotation
*/
Quaternion_t get_location_rotation_quaternion(latitude)
{
    float angle = 90 - latitude; //degrees
    Vector3f_t rotation_axis = {1.0f, 0.0f, 0.0f}; //Rotate around x axis
    Quaternion_t q_location = rotation_to_quaternion(angle, rotation_axis);
    return q_location;
}

/**
  @brief         Model a rotation by applying a rotation quaternion to a point quaternion
  @param[in]     q_point        A quaternion representing the point to rotate
  @param[in]     q_rotation     A quaternion representing the rotation to apply
  @returns       A quaternion representing the point after the rotation
*/
Quaternion_t apply_active_rotation(const Quaternion_t q_point, const Quaternion_t q_rotation)
{
    Quaternion_t q_rotation_inverse;
    quaternion_inverse(&q_rotation, &q_rotation_inverse, 1); //Get inverse of rotation quaternion
    Quaternion_t q_temp, q_point_rotated;
    quaternion_product_single(q_rotation_inverse, q_point, &q_temp); //q_temp = q_rotation_inverse * q_point
    quaternion_product_single(q_temp, q_rotation_inverse, &q_point_rotated); //q_point_rotated = q_temp * q_rotation_inverse
    return q_point_rotated;
}

/**
  @brief         Create a quaternion that represents a rotation
  @param[in]     angle          The number of degrees to rotate about the axis
  @param[in]     q_rotation     The axis to rotate about (must be a unit vector)
  @returns       A quaternion representing the rotation
*/
Quaternion_t rotation_to_quaternion(float angle, Vector3f_t axis)
{
    //TODO: Error check for unit vector axis?
    Quaternion_t q_rotation;
    q_rotation.w = cos(angle / 2);
    q_rotation.x = sin(angle / 2) * axis.x;
    q_rotation.y = sin(angle / 2) * axis.y;
    q_rotation.z = sin(angle / 2) * axis.z;
    return q_rotation;
}

/**************************************************************************** */
/*                             Quaternion Functions                           */
/**************************************************************************** */
/**
  @brief         Floating-point quaternion Norm.
  @param[in]     pInputQuaternions       points to the input vector of quaternions
  @param[out]    pNorms                  points to the output vector of norms
  @param[in]     nbQuaternions           number of quaternions in each vector
  */
void quaternion_norm(const Quaternion_t *pInputQuaternions, 
    float32_t *pNorms,
    uint32_t nbQuaternions)
{
    float32_t inputQuaternionArray[4 * nbQuaternions];
    for (int i = 0; i < nbQuaternions; i++)
    {
        inputQuaternionArray[4 * i + 0] = pInputQuaternions[i].w;
        inputQuaternionArray[4 * i + 1] = pInputQuaternions[i].x;
        inputQuaternionArray[4 * i + 2] = pInputQuaternions[i].y;
        inputQuaternionArray[4 * i + 3] = pInputQuaternions[i].z;
    }
    arm_quaternion_norm_f32(inputQuaternionArray, pNorms, nbQuaternions);
    return;
}

/**
  @brief         Floating-point quaternion inverse.
  @param[in]     pInputQuaternions            points to the input vector of quaternions
  @param[out]    pInverseQuaternions          points to the output vector of inverse quaternions
  @param[in]     nbQuaternions                number of quaternions in each vector  
  */
void quaternion_inverse(const Quaternion_t *pInputQuaternions, 
    Quaternion_t *pInverseQuaternions,
    uint32_t nbQuaternions)
{
    float32_t inputQuaternionArray[4 * nbQuaternions], outputQuaternionArray[4 * nbQuaternions];
    for (int i = 0; i < nbQuaternions; i++)
    {
        inputQuaternionArray[4 * i + 0] = pInputQuaternions[i].w;
        inputQuaternionArray[4 * i + 1] = pInputQuaternions[i].x;
        inputQuaternionArray[4 * i + 2] = pInputQuaternions[i].y;
        inputQuaternionArray[4 * i + 3] = pInputQuaternions[i].z;
    }
    arm_quaternion_inverse_f32(inputQuaternionArray, outputQuaternionArray, nbQuaternions);
    for (int i = 0; i < nbQuaternions; i++)
    {
        pInverseQuaternions[i].w = outputQuaternionArray[4 * i + 0];
        pInverseQuaternions[i].x = outputQuaternionArray[4 * i + 1];
        pInverseQuaternions[i].y = outputQuaternionArray[4 * i + 2];
        pInverseQuaternions[i].z = outputQuaternionArray[4 * i + 3];
    }
    return;
}


/**
  @brief         Floating-point quaternion conjugates.
  @param[in]     pInputQuaternions            points to the input vector of quaternions
  @param[out]    pConjugateQuaternions        points to the output vector of conjugate quaternions
  @param[in]     nbQuaternions                number of quaternions in each vector
  */
void quaternion_conjugate(const Quaternion_t *inputQuaternions, 
    Quaternion_t *pConjugateQuaternions,
    uint32_t nbQuaternions)
{
    float32_t inputQuaternionArray[4 * nbQuaternions], outputQuaternionArray[4 * nbQuaternions];
    for (int i = 0; i < nbQuaternions; i++)
    {
        inputQuaternionArray[4 * i + 0] = inputQuaternions[i].w;
        inputQuaternionArray[4 * i + 1] = inputQuaternions[i].x;
        inputQuaternionArray[4 * i + 2] = inputQuaternions[i].y;
        inputQuaternionArray[4 * i + 3] = inputQuaternions[i].z;
    }
    arm_quaternion_conjugate_f32(inputQuaternionArray, outputQuaternionArray, nbQuaternions);
    for (int i = 0; i < nbQuaternions; i++)
    {
        pConjugateQuaternions[i].w = outputQuaternionArray[4 * i + 0];
        pConjugateQuaternions[i].x = outputQuaternionArray[4 * i + 1];
        pConjugateQuaternions[i].y = outputQuaternionArray[4 * i + 2];
        pConjugateQuaternions[i].z = outputQuaternionArray[4 * i + 3];
    }
    return;
}


/**
  @brief         Floating-point normalization of quaternions.
  @param[in]     pInputQuaternions            points to the input vector of quaternions
  @param[out]    pNormalizedQuaternions       points to the output vector of normalized quaternions
  @param[in]     nbQuaternions                number of quaternions in each vector
  */
void quaternion_normalize(const Quaternion_t *inputQuaternions, 
    Quaternion_t *pNormalizedQuaternions,
    uint32_t nbQuaternions)
{
    float32_t inputQuaternionArray[4 * nbQuaternions], outputQuaternionArray[4 * nbQuaternions];
    for (int i = 0; i < nbQuaternions; i++)
    {
        inputQuaternionArray[4 * i + 0] = inputQuaternions[i].w;
        inputQuaternionArray[4 * i + 1] = inputQuaternions[i].x;
        inputQuaternionArray[4 * i + 2] = inputQuaternions[i].y;
        inputQuaternionArray[4 * i + 3] = inputQuaternions[i].z;
    }
    arm_quaternion_normalize_f32(inputQuaternionArray, outputQuaternionArray, nbQuaternions);
    for (int i = 0; i < nbQuaternions; i++)
    {
        pNormalizedQuaternions[i].w = outputQuaternionArray[4 * i + 0];
        pNormalizedQuaternions[i].x = outputQuaternionArray[4 * i + 1];
        pNormalizedQuaternions[i].y = outputQuaternionArray[4 * i + 2];
        pNormalizedQuaternions[i].z = outputQuaternionArray[4 * i + 3];
    }
    return;
}


/**
  @brief         Floating-point product of two quaternions.
  @param[in]     qa       First quaternion
  @param[in]     qb       Second quaternion
  @param[out]    r        Product of two quaternions
 */
void quaternion_product_single(const Quaternion_t qa, 
    const Quaternion_t qb, 
    Quaternion_t *r)
{
    float32_t inputQaArray[4] = {qa.w, qa.x, qa.y, qa.z},
            inputQbArray[4] = {qb.w, qb.x, qb.y, qb.z}, 
            outputQuaternionArray[4];
    
    arm_quaternion_product_single_f32(inputQaArray, inputQbArray, outputQuaternionArray);
    (*r).w = outputQuaternionArray[0];
    (*r).x = outputQuaternionArray[1];
    (*r).y = outputQuaternionArray[2];
    (*r).z = outputQuaternionArray[3];
    return;
}


/**
  @brief         Floating-point elementwise product two quaternions (qa[0] * qb[0], qa[1] * qb[1], etc).
  @param[in]     qa                  First array of quaternions
  @param[in]     qb                  Second array of quaternions
  @param[out]    r                   Elementwise product of quaternions
  @param[in]     nbQuaternions       Number of quaternions in each vector
  */
void quaternion_product(const Quaternion_t *qa, 
    const Quaternion_t *qb, 
    Quaternion_t *r,
    uint32_t nbQuaternions)
{
    float32_t inputQaArray[4 * nbQuaternions], inputQbArray[4 * nbQuaternions], outputQuaternionArray[4 * nbQuaternions];
    for (int i = 0; i < nbQuaternions; i++)
    {
        inputQaArray[4 * i + 0] = qa[i].w;
        inputQaArray[4 * i + 1] = qa[i].x;
        inputQaArray[4 * i + 2] = qa[i].y;
        inputQaArray[4 * i + 3] = qa[i].z;
        inputQbArray[4 * i + 0] = qb[i].w;
        inputQbArray[4 * i + 1] = qb[i].x;
        inputQbArray[4 * i + 2] = qb[i].y;
        inputQbArray[4 * i + 3] = qb[i].z;
    }
    arm_quaternion_product_f32(inputQaArray, inputQbArray, outputQuaternionArray, nbQuaternions);
    for (int i = 0; i < nbQuaternions; i++)
    {
        r[i].w = outputQuaternionArray[4 * i + 0];
        r[i].x = outputQuaternionArray[4 * i + 1];
        r[i].y = outputQuaternionArray[4 * i + 2];
        r[i].z = outputQuaternionArray[4 * i + 3];
    }
    return;
}

