/*******************************************************************************
 * @file        imu.h
 * @brief       Driver library for IMU.
 * @details     All functions needed to interact with BNO080 IMU.
 * 
 * @see         imu.c for implementation details.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * @version     1.0
 ******************************************************************************/

#ifndef IMU_H
#define IMU_H

/**
  @brief         Initialize pins of RP3250 for I2C communication
*/
void init_i2c();

/**
  @brief        Read data from the IMU
  @note         Should be triggered with interrupt
*/
void read_imu_data();

#endif /* IMU_H */