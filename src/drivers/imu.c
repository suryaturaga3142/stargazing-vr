/*******************************************************************************
 * @file        imu.c
 * @brief       Implements the functionality for the IMU module.
 * @details     Complete set of functions needed to interact with the BNO080.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * 
 * @note        This module is designed to be driven by interrupts and is not
 *              intended to be called from a blocking main loop. Functions here
 *              need to be of highest priority to minimize latency.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "imu.h"
#include "pico/stdlib.h"
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Variables --------------------------- */
volatile IMUData_t g_latest_imu_data = {0};

/* ----------------------------- Public Functions --------------------------- */

/**
 * @brief Initializes the IMU in I2C and configures Game Rotation Vectors with interrupt
 * 
 * @return true if initialization was successful
 */
bool imu_init(void) {
    // Initialize the IMU module in I2C, configure game rotation vectors with interrupts at 200Hz.
    sleep_ms(3000);
    return true;
}