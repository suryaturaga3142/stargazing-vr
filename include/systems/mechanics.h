/*******************************************************************************
 * @file        mechanics.h
 * @brief       Library to declare all quaternion mechanics.
 * @details     Handles CMSIS derived functions for rotations and working with
 *              backend of rendering.
 * 
 * @see         mechanics.c for implementation details.
 * 
 * @author      LED Chasers
 * @date        2025-10-24
 * @version     1.0
 ******************************************************************************/

#ifndef MECHANICS_H
#define MECHANICS_H

#include "structs.h"

/**
  @brief         Floating-point quaternion Norm.
  @param[in]     pInputQuaternions       points to the input vector of quaternions
  @param[out]    pNorms                  points to the output vector of norms
  @param[in]     nbQuaternions           number of quaternions in each vector
 */
void quaternion_norm(const Quaternion_t *pInputQuaternions, 
    float32_t *pNorms,
    uint32_t nbQuaternions);


/**
  @brief         Floating-point quaternion inverse.
  @param[in]     pInputQuaternions            points to the input vector of quaternions
  @param[out]    pInverseQuaternions          points to the output vector of inverse quaternions
  @param[in]     nbQuaternions                number of quaternions in each vector  
  */
void quaternion_inverse(const Quaternion_t *pInputQuaternions, 
    Quaternion_t *pInverseQuaternions,
    uint32_t nbQuaternions);


/**
  @brief         Floating-point quaternion conjugates.
  @param[in]     pInputQuaternions            points to the input vector of quaternions
  @param[out]    pConjugateQuaternions        points to the output vector of conjugate quaternions
  @param[in]     nbQuaternions                number of quaternions in each vector
  */
void quaternion_conjugate(const Quaternion_t *inputQuaternions, 
    Quaternion_t *pConjugateQuaternions,
    uint32_t nbQuaternions);


/**
  @brief         Floating-point normalization of quaternions.
  @param[in]     pInputQuaternions            points to the input vector of quaternions
  @param[out]    pNormalizedQuaternions       points to the output vector of normalized quaternions
  @param[in]     nbQuaternions                number of quaternions in each vector
  */
void quaternion_normalize(const Quaternion_t *inputQuaternions, 
    Quaternion_t *pNormalizedQuaternions,
    uint32_t nbQuaternions);


/**
  @brief         Floating-point product of two quaternions.
  @param[in]     qa       First quaternion
  @param[in]     qb       Second quaternion
  @param[out]    r        Product of two quaternions
 */
void quaternion_product_single(const Quaternion_t qa, 
    const Quaternion_t qb, 
    Quaternion_t *r);


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
    uint32_t nbQuaternions);


// I don't think we need these
//
// /**
//  * @brief Conversion of quaternion to equivalent rotation matrix.
//  * @param[in]       pInputQuaternions points to an array of normalized quaternions
//  * @param[out]      pOutputRotations points to an array of 3x3 rotations (in row order)
//  *
//  * <b>Format of rotation matrix</b>
//  * \par
//  * The quaternion a + ib + jc + kd is converted into rotation matrix:
//  *   a^2 + b^2 - c^2 - d^2                 2bc - 2ad                 2bd + 2ac
//  *               2bc + 2ad     a^2 - b^2 + c^2 - d^2                 2cd - 2ab
//  *               2bd - 2ac                 2cd + 2ab     a^2 - b^2 - c^2 + d^2
//  *
//  * Rotation matrix is saved in row order : R00 R01 R02 R10 R11 R12 R20 R21 R22
//  */
// void quaternion2rotation(const Quaternion_t *pInputQuaternions, 
//     float32_t *pOutputRotations);


// /**
//  * @brief Conversion of a rotation matrix to equivalent quaternion.
//  * @param[in]       pInputRotations points to an array 3x3 rotation matrix (in row order)
//  * @param[out]      pOutputQuaternions points to an array of quaternions
// */
// void rotation2quaternion(const float32_t *pInputRotations, 
//     Quaternion_t *pOutputQuaternions);

#endif /* MECHANICS_H */