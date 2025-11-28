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
#include "pico/stdlib.h"
// ...

/* ---------------------------- Private Constants --------------------------- */
static const double SCALER_COORD = 1000000.0;
static const double SCALER_MAG   = 1000.0;
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */

/**
 * @brief Assumes peripherals initialized, checks if the SD card is present and alive.
 * 
 * @return true if the SD card was detected
 */
bool sd_check(void) {
    if (gpio_get(PIN_SD_DET)) {
        return false;
    }

    // Start spi in slow speed, send dummy bytes, check for response.
    // return false if no response or invalid response.
    // Do not mount or access data here.
    // If successful, up the rate and return true.

    spi_set_baudrate(SPI_PORT, SD_CARD_INIT_HZ);

    // Wake up the card with 74+ clock cycles with CS high
    gpio_put(PIN_SD_CSN, 1);
    uint8_t dummy = 0xFF;
    for (int i = 0; i < 10; i++) {
        spi_write_blocking(spi0, &dummy, 1);
    }
    gpio_put(PIN_SD_CSN, 0);

    
    // Sandy finish and test this please




    return true;
}

/**
 * @brief Initializes SD card peripheral and pins. Does NOT mount or access data.
 * 
 * @return true if initialization was successful
 */
bool sd_init(void) {
    
    gpio_init(PIN_SD_CSN);
    gpio_init(PIN_SD_RX);
    gpio_init(PIN_SD_TX);
    gpio_init(PIN_SD_SCK);
    gpio_init(PIN_SD_DET);

    gpio_set_function(PIN_SD_RX, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SD_TX, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SD_SCK, GPIO_FUNC_SPI);

    gpio_set_dir(PIN_SD_CSN, GPIO_OUT);
    gpio_put(PIN_SD_CSN, 1); // Deselect

    spi_init(SPI_PORT, SD_CARD_INIT_HZ);
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    return (sd_check());
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
 * @brief Unmounts the SD card and deinitializes the peripheral
 * 
 * @return true if both processes were successful
 */
bool sd_deinit(void) {
    // Unmount the SD card and deinitialize the interface.
    return true;
}

/**
 * @brief Sorts the bulk read buffer with a 3 pass algorithm
 * 
 * @return true if sorting was successful
 */
bool sd_buf_sort(void) {
    // Unpack the bulk read buffer and sort it with 3 pass algorithm.
    // Surya's job. nothing to actually do with the SD card here.
    return true;
}
