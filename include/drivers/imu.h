/*******************************************************************************
 * @file        imu.h
 * @brief       Driver library for IMU.
 * @details     All functions needed to interact with BNO080 IMU.
 * 
 * @see         imu.c for implementation details.
 * 
 * @author      LED Chasers
 * @date        2025-11-02
 * @version     1.0
 ******************************************************************************/

#ifndef IMU_H
#define IMU_H

#include "structs.h"
#include <stdint.h>

//TODO: Probably need to add a flag that indicates whether this rotation has been appleid yet
Quaternion_t current_rotation_vector;  // The most recently returned quaternion from the IMU
volatile int imu_data_ready_flag;

//Struct to hold BNO085 packet data
typedef struct {
    uint8_t  channel;
    uint8_t  sequence;
    uint16_t payload_len;
    uint8_t  payload[255];
    bool     valid;
} bno085_packet_t;

//SHTP Channels
typedef enum {
  CHANNEL_COMMAND       = 0, //Bootloader
  CHANNEL_EXECUTABLE    = 1, //Main control channel during initialization
  CHANNEL_CONTROL       = 2, //Sensor hub control
  CHANNEL_REPORTS       = 3, //Primary data channel for sensor/feature reports
  CHANNEL_WAKE_REPORTS  = 4, //Same as channel 3 but only active when devices wakes from low power mode
  CHANNEL_GYRO_VECTOR   = 5  //Specialized channel for high-frequency gyro/rotation data
} shtp_channel_t;

//SHTP Commands
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
  REPORT_GRAVITY_VECTOR                 = 0x07, //Gravity direction & magnitude
  REPORT_GAME_ROTATION_VECTOR           = 0x08, //Quaternion without magnetometer correction
  REPORT_GEOMAG_ROTATION_VECTOR         = 0x06, //Quaternion using accel + mag (no gyro)
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
  @brief        Handle interrupt and raise flag that there is data ready to be read
  @note         Should be triggered with interrupt
*/
void imu_isr(void);

/**
  @brief         Read one full packet of data from BNO085 with SPI
  @param
*/
int bno085_read_packet(bno085_packet_t *pkt);

/**
    @brief          Clears every pending SHTP packet in FIFO and looks for packet on channel 3.
    @return         Returns true only when a channel-3 packet was found and stored in out_pkt.
 */
bool bno085_service_fifo(bno085_packet_t *out_pkt);

/**
    @brief          Clears every pending SHTP packet in FIFO and looks for packet on channel 2.
    @return         Returns true only when a channel-2 packet was found and stored in out_pkt.
 */
bool bno085_service_fifo_ch2(bno085_packet_t *out_pkt);

/**
  @brief        Parse IMU rotation vector data into quaternion
*/
void parse_imu_rotation_data();

/**
  @brief        Read data from the IMU
*/
void read_imu_data(void);


#endif /* IMU_H */