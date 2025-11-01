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
#include "hardware/spi.h"
#include "hardware/gpio.h"
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...
const spi_inst_t* SPI_BUS = spi1;
const int SPI_IMU_SCK = 26; // SCK pin number for the IMU
const int SPI_IMU_CSn = 25; // CSn pin number for the IMU
const int SPI_IMU_RX = 24; // TX pin number for the IMU
const int IMU_INTR = 22; // GPIO pin to receive the interrupts from the IMU
const int SPI_BAUDRATE = 100000; //bytes/second //Fast mode

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */

/**
  @brief         Initialize pins of RP3250 for SPI communication
*/
void init_spi_for_imu()
{
    //Initialize GPIO pins for SPI
    gpio_init_mask((1 << SPI_IMU_SCK) | (1 << SPI_IMU_CSn) | (1 << SPI_IMU_RX));
    gpio_set_function_masked((1 << SPI_IMU_SCK) | (1 << SPI_IMU_CSn) | (1 << SPI_IMU_RX), GPIO_FUNC_SPI);

    //Enable interrupt when IMU_INTR pin is pulled low
    gpio_init(IMU_INTR);
    gpio_set_dir(IMU_INTR, false);
    gpio_add_raw_irq_handler(IMU_INTR, read_imu_data);
    irq_set_enabled(GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(IMU_INTR, GPIO_IRQ_EDGE_FALL, true);

    //Initialize SPI
    spi_init(SPI_BUS, SPI_BAUDRATE);
    spi_set_format(SPI_BUS, 16, 0, 0, SPI_MSB_FIRST); //Figure out number of bits per data transfer

    //Configure IMU to Generate interrupts at 100kHz
    

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
