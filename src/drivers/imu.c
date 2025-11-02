/*******************************************************************************
 * @file        imu.c
 * @brief       Implements the functionality for the IMU module.
 * @details     Complete set of functions needed to interact with the BNO080.
 * 
 * @author      LED Chasers
 * @date        2025-11-02
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

//TODO: Probably need to add a flag that indicates whether this rotation has been appleid yet
Quaternion_t current_rotation_vector = {1.0f, 0.0f, 0.0f, 0.0f};  // The most recently returned quaternion from the IMU

/* ----------------------------- Private Functions -------------------------- */
// ...

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
        REPORT_GAME_ROTATION_VECTOR, //Report ID
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
    if ((channel == CHANNEL_REPORTS) && (payload[0] == REPORT_GAME_ROTATION_VECTOR)) {
        //Divide by 2^14 to normalize quaternion
        current_rotation_vector.w = (payload[1] | (payload[2] << 8)) / 16384.0f;
        current_rotation_vector.x = (payload[3] | (payload[4] << 8)) / 16384.0f;
        current_rotation_vector.y = (payload[5] | (payload[6] << 8)) / 16384.0f;
        current_rotation_vector.z = (payload[7] | (payload[8] << 8)) / 16384.0f;
    }
    
    return;
}
