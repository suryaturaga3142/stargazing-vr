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
#include <stdint.h>

typedef enum {
  CHANNEL_COMMAND       = 0, //Bootloader
  CHANNEL_EXECUTABLE    = 1, //Main control channel during initialization
  CHANNEL_CONTROL       = 2, //Sensor hub control
  CHANNEL_REPORTS       = 3, //Primary data channel for sensor/feature reports
  CHANNEL_WAKE_REPORTS  = 4, //Same as channel 3 but only active when devices wakes from low power mode
  CHANNEL_GYRO_VECTOR   = 5  //Specialized channel for high-frequency gyro/rotation data
} shtp_channel_t;

typedef enum {
  CMD_GET_FEATURE_RESPONSE  = 0xF1, //Response to a set feature cmd
  CMD_GET_FEATURE           = 0xF2, //Request current feature config
  CMD_PRODUCT_ID_REQUEST    = 0xF8, //Query product indentification info
  CMD_PRODUCT_ID_RESPONSE   = 0xF9, //Returns sensor name, SW/HW version, etc.
  CMD_RESET                 = 0xFA, //Request a software reset
  CMD_CONTROL               = 0xFB, //General hub control control (power, calibration ops)
  CMD_CONTROL_RESPONSE      = 0xFC, //Response to a control cmd
  CMD_SET_FEATURE           = 0xFD, //Enable a sensor report
  CMD_ERROR_RESPONSE        = 0xFF  //Indicates invalid or failed command
} shtp_command_t;

//Sensor Report IDs for BNO085 (SHTP protocol)
typedef enum {
  REPORT_ACCELEROMETER                  = 0x01, //Linear acceleration (m/s^2)
  REPORT_GYROSCOPE                      = 0x02, //Angular velocity (rad/s)
  REPORT_MAGNETOMETER                   = 0x03, //Magnetic field (uT)
  REPORT_LINEAR_ACCELERATION            = 0x04, //Acceleration w/ gravity removed
  REPORT_ROTATION_VECTOR                = 0x05, //Quaternion orientation (fusion of accel + gyro + mag)
  REPORT_GRAVITY_VECTOR                 = 0x06, //Gravity direction & magnitude
  REPORT_GAME_ROTATION_VECTOR           = 0x07, //Quaternion without magnetometer correction
  REPORT_GEOMAG_ROTATION_VECTOR         = 0x08, //Quaternion using accel + mag (no gyro)
  REPORT_ACCELEROMETER_UNCALIBRATED     = 0x09, //Raw accel data before offset calibration
  REPORT_GYROSCOPE_UNCALIBRATED         = 0x0A, //Raw gyro data before offset calibration
  REPORT_MAGNETOMETER_UNCALIBRATED      = 0x0B, //Raw mag data before offset calibration
  REPORT_STEP_DETECTOR                  = 0x0C, //Single-bit step event
  REPORT_STEP_COUNTER                   = 0x0D, //Step count (integer)
  REPORT_SIGNIFICANT_MOTION             = 0x0E, //Detects major motion events
  REPORT_STABILITY_CLASSIFIER           = 0x0F, //"Stable", "Motion", etc. classification
  REPORT_ACTIVITY_CLASSIFIER            = 0x10, //Walking, running, etc.
  REPORT_RAW_ACCELEROMETER              = 0x11, //Raw sensor accel ADC values
  REPORT_RAW_GYROSCOPE                  = 0x12, //Raw sensor gyro ADC values
  REPORT_RAW_MAGNETOMETER               = 0x13, //Raw sensor mag ADC values
  REPORT_PERSONAL_ACTIVITY_CLASSIFIER   = 0x14  //Extended motion classification
} bno085_report_id_t;

/* ---------------------------- Public Functions --------------------------- */

/**
  @brief         Initialize pins of RP3250 for SPI communication
*/
void init_spi_for_imu(void);

/**
  @brief        Read data from the IMU
  @note         Should be triggered with interrupt
*/
void read_imu_data(void);


/**
  @brief        Update the current orientation quaternion
  @param[in]    quat      Pointer to new orientation quaternion
*/
void update_orientation(const Quaternion_t* quat);


/**
  @brief        Update the current acceleration vector
  @param[in]    accel     Pointer to new acceleration vector
*/
void update_acceleration(const Vector3f_t* accel);


/**
  @brief        Update the current angular velocity vector
  @param[in]    gyro      Pointer to new angular velocity vector
*/
void update_angular_velocity(const Vector3f_t* gyro);


/**
  @brief        Handle calibration response from IMU
  @param[in]    status     Calibration status byte
*/
void handle_calibration_response(uint8_t status);

#endif /* IMU_H */