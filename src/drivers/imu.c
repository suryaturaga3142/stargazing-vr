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
#include "stdint.h"
#include "hardware/i2c.h"
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...
const i2c_inst_t* I2C_BUS = i2c0;
const int I2C_SDA_PIN;
const int I2C_SCL_PIN;
const int I2C_BAUDRATE;
const uint8_t IMU_I2C_ADDRESS;

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */

/**
  @brief         Initialize pins of RP3250 for I2C communication
*/
void init_i2c()
{
    //i2c_init(I2C_BUS, I2C_BAUDRATE);

    return;
}

/**
  @brief        Read data from the IMU
  @note         Should be triggered with interrupt
*/
void read_imu_data()
{

    return;
}
