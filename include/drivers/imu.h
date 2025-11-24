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

#include "structs.h"
#include "config.h"

extern volatile IMUData_t g_latest_imu_data;      // Updated by the 100Hz IMU ISR.


#endif /* IMU_H */