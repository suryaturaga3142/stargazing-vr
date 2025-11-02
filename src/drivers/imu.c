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
#include "hardware/spi.h"
#include "hardware/gpio.h"
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...
spi_inst_t* SPI_BUS = spi1;
const int SPI_IMU_SCK = 26; // SCK pin number for the IMU
const int SPI_IMU_CSn = 25; // CSn pin number for the IMU
const int SPI_IMU_RX = 24; // RX pin number for the IMU
const int SPI_IMU_TX = 27; // TX pin number for the IMU
const int IMU_INTR = 22; // GPIO pin to receive the interrupts from the IMU
const int SPI_BAUDRATE = 100000; //bytes/second

/* ----------------------------- Private Variables -------------------------- */
uint8_t sequence_num = 1;

//Possibly delete these
static Quaternion_t current_orientation = {1.0f, 0.0f, 0.0f, 0.0f};  // Identity quaternion
static Vector3f_t current_acceleration = {0.0f, 0.0f, 0.0f};
static Vector3f_t current_angular_velocity = {0.0f, 0.0f, 0.0f};
static uint8_t calibration_status = 0;

/* ----------------------------- Private Functions -------------------------- */
/**
 * @brief Process quaternion data from BNO085
 * @param payload Pointer to payload data
 * @param length Length of payload
 */
static void process_quaternion_data(uint8_t* payload, uint16_t length)
{
    if (length < 12) return; // Need at least 12 bytes for quaternion
    
    // BNO085 sends quaternion data as signed int16 values
    // Need to convert to float by dividing by 2^14
    int16_t qw = ((int16_t)payload[0]) | ((int16_t)payload[1] << 8);
    int16_t qx = ((int16_t)payload[2]) | ((int16_t)payload[3] << 8);
    int16_t qy = ((int16_t)payload[4]) | ((int16_t)payload[5] << 8);
    int16_t qz = ((int16_t)payload[6]) | ((int16_t)payload[7] << 8);
    
    // Convert to float (divide by 2^14)
    float scale = 1.0f / 16384.0f; // 2^14
    Quaternion_t quat = {
        .w = qw * scale,
        .x = qx * scale,
        .y = qy * scale,
        .z = qz * scale
    };
    
    // Store or process quaternion data
    update_orientation(&quat);
}

/**
 * @brief Process accelerometer data from BNO085
 * @param payload Pointer to payload data
 * @param length Length of payload
 */
static void process_accelerometer_data(uint8_t* payload, uint16_t length)
{
    if (length < 6) return; // Need at least 6 bytes for accelerometer
    
    // BNO085 sends accelerometer data as signed int16 values in m/s^2
    int16_t ax = ((int16_t)payload[0]) | ((int16_t)payload[1] << 8);
    int16_t ay = ((int16_t)payload[2]) | ((int16_t)payload[3] << 8);
    int16_t az = ((int16_t)payload[4]) | ((int16_t)payload[5] << 8);
    
    // Convert to float (scale factor depends on configured range)
    float scale = 1.0f / 100.0f; // Example: if range is ±8g
    Vector3f_t accel = {
        .x = ax * scale,
        .y = ay * scale,
        .z = az * scale
    };
    
    // Store or process acceleration data
    update_acceleration(&accel);
}

/**
 * @brief Process gyroscope data from BNO085
 * @param payload Pointer to payload data
 * @param length Length of payload
 */
static void process_gyroscope_data(uint8_t* payload, uint16_t length)
{
    if (length < 6) return; // Need at least 6 bytes for gyroscope
    
    // BNO085 sends gyroscope data as signed int16 values in rad/s
    int16_t gx = ((int16_t)payload[0]) | ((int16_t)payload[1] << 8);
    int16_t gy = ((int16_t)payload[2]) | ((int16_t)payload[3] << 8);
    int16_t gz = ((int16_t)payload[4]) | ((int16_t)payload[5] << 8);
    
    // Convert to float (scale factor depends on configured range)
    float scale = 1.0f / 16.0f; // Example: if range is ±2000 dps
    Vector3f_t gyro = {
        .x = gx * scale,
        .y = gy * scale,
        .z = gz * scale
    };
    
    // Store or process gyroscope data
    update_angular_velocity(&gyro);
}

/**
 * @brief Process command response from BNO085
 * @param payload Pointer to payload data
 * @param length Length of payload
 */
static void process_command_response(uint8_t* payload, uint16_t length)
{
    if (length < 1) return;
    
    uint8_t command = payload[0];
    uint8_t status = (length > 1) ? payload[1] : 0;
    
    // Handle command response
    switch (command) {
        case 0x01: // Example: Calibration response
            handle_calibration_response(status);
            break;
        // Add other command responses as needed
    }
}

/* ----------------------------- Public Functions --------------------------- */

/**
  @brief         Initialize pins of RP3250 for SPI communication
*/
void init_spi_for_imu()
{
    //Initialize GPIO pins for SPI
    gpio_init_mask((1 << SPI_IMU_SCK) | (1 << SPI_IMU_CSn) | (1 << SPI_IMU_RX) | (1 << SPI_IMU_TX));
    gpio_set_function_masked((1 << SPI_IMU_SCK) | (1 << SPI_IMU_CSn) | (1 << SPI_IMU_RX) | (1 << SPI_IMU_TX), GPIO_FUNC_SPI);
    gpio_put(SPI_IMU_CSn, 1);

    //Enable interrupt when IMU_INTR pin is pulled low
    gpio_init(IMU_INTR);
    gpio_set_dir(IMU_INTR, false);
    gpio_add_raw_irq_handler(IMU_INTR, read_imu_data);
    irq_set_enabled(IO_IRQ_BANK0, true);
    //irq_set_enabled(GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(IMU_INTR, GPIO_IRQ_EDGE_FALL, true);

    //Initialize SPI
    spi_init(SPI_BUS, SPI_BAUDRATE);
    spi_set_format(SPI_BUS, 8, 0, 0, SPI_MSB_FIRST); //Figure out number of bits per data transfer

    //Configure IMU to generate interrupts at 100Hz
    //TODO: Turn packet into a struct?
    uint8_t packet[] = {
        // SHTP header (4 bytes)
        0x0F, 0x00,      //Length = 15 bytes (LSB first)
        CHANNEL_CONTROL, //Channel 2
        sequence_num++, 

        // Payload (Set Feature Command)
        CMD_SET_FEATURE,        //Command ID
        REPORT_ROTATION_VECTOR, //Report ID
        0x00,                   //Feature flags
        0xA0, 0x86, 0x01, 0x00, //Report interval = 10000 us (100 Hz)
        0x00, 0x00,             //Change sensitivty
        0x00, 0x00,             //Batch interval
    };
    gpio_put(SPI_IMU_CSn, 0); //Pull chip select low
    spi_write_blocking(SPI_BUS, packet, sizeof(packet));

    //TODO: Check packet is received by checking for a feature response (0xF1) on channel 2

    return;
}

/**
  @brief        Read data from the IMU
  @note         Should be triggered with interrupt
*/
//TODO: Change so SPI reads are done outside of ISR function and ISR only sets flag that data is ready (to prevent missing data packets)
void read_imu_data()
{
    gpio_acknowledge_irq(IMU_INTR, GPIO_IRQ_EDGE_FALL);

    // Pull CS low to start transaction
    gpio_put(SPI_IMU_CSn, 0);
    
    // Read header (4 bytes)
    uint8_t header[4];
    spi_read_blocking(SPI_BUS, 0, header, 4);
    
    // Parse header
    uint16_t packet_length = ((uint16_t)header[0] | ((uint16_t)header[1] << 8)) & 0x7FFF; //Exclude bit 15w which is the continuation bit
    uint8_t channel = header[2];
    uint8_t seq_num = header[3];
    
    uint16_t payload_length = packet_length - 4;

    // Allocate buffer for payload
    uint8_t payload[payload_length];
    
    // Read payload
    if (payload_length > 0) {
        spi_read_blocking(SPI_BUS, 0, payload, payload_length);
    }
    
    // Pull CS high to end transaction
    gpio_put(SPI_IMU_CSn, 1);
    
    // Process the packet based on channel
    /*switch (channel) {
        case 0x01: // Command Response
            process_command_response(payload, length);
            break;
            
        case 0x02: // Quaternion Data
            process_quaternion_data(payload, length);
            break;
            
        case 0x03: // Accelerometer
            process_accelerometer_data(payload, length);
            break;
            
        case 0x04: // Gyroscope
            process_gyroscope_data(payload, length);
            break;
            
        default:
            // Unknown channel
            break;
    }*/
    
    return;
}
