/*******************************************************************************
 * @file        sd_card.c
 * @brief       Implements the functionality for the SD Card module.
 * @details     Complete set of functions to use the SD Card with SDIO.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * 
 * @note        This module is designed to be used only during initialization
 *              to pull all the data into buffers.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "sd_card.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/spi.h"
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */

/**
 * @brief Assumes peripherals initialized and checks DET for SD card presence
 * 
 * @return true if the SD card was detected
 */
bool sd_check(void) {
    // Only check if the SD card is present. Do not mount or initialize here.
    return true;
}

/**
 * @brief Initializes SD card peripheral and pins. Does not mount.
 * 
 * @return true if initialization was successful
 */
bool sd_init(void) 
{
    
    // Initialize SD card interface and pins here. Do not mount yet.
    // gpio_init(PIN_SDIO_CLK);
    // gpio_set_function(PIN_SDIO_CLK, GPIO_FUNC_PIO0);
    gpio_set_function(PIN_SPI_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);

    gpio_set_function(PIN_SPI_DET, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_SPI_DET, GPIO_IN);


    gpio_set_function(PIN_SPI_CS, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_SPI_CS, GPIO_OUT);
    gpio_put(PIN_SPI_CS, true);

    spi_init(spi0, 400000);
    spi_set_format(spi0,8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    return true;
}

/**
 * @brief Mounts the SD card and performs a bulk read of the buffer
 * 
 * @return true if both processes were successful
 */
bool sd_load_data(void) {
    // Mount the SD card, read all necessary data into buffers.
    return true;
}

/**
 * @brief Sorts the bulk read buffer with a 3 pass algorithm
 * 
 * @return true if sorting was successful
 */
bool sd_buf_sort(void) {
    // Unpack the bulk read buffer and sort it with 3 pass algorithm.
    return true;
}

/**
 * @brief Unmounts the SD card and deinitializes the peripheral
 * 
 * @return true if both processes were successful
 */
bool sd_deinit(void) {
    // Unmount the SD card and deinitialize the interface.
    return true;
}