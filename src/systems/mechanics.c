/*******************************************************************************
 * @file        mechanics.c
 * @brief       Implements the functionality for the quaternion rotations.
 * @details     Complete set of functions to use CMSIS-DSP for rotations.
 *              Basically more extensive wrapper functions for CMSIS-DSP.
 * 
 * @author      LED Chasers
 * @date        2025-10-24
 * 
 * @note        This module is designed to be driven by interrupts and is not
 *              intended to be called from a blocking main loop. Use as needed.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "mechanics.h"

#include "dsp/quaternion_math_functions.h"
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */
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

