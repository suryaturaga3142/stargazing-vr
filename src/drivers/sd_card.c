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
#include "pico/stdlib.h"
// ...

/* ---------------------------- Private Constants --------------------------- */
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


    return true;
}

/**
 * @brief Initializes SD card peripheral and pins. Does NOT mount or access data.
 * 
 * @return true if initialization was successful
 */
bool sd_init(void) {
    // Initialize SD card interface and pins here. Do not mount yet.
    // Start at slow speed, send the dummy bytes here, and then up the rate.
    sleep_ms(50);
    gpio_init(PIN_SD_CSN);
    gpio_init(PIN_SD_RX);
    gpio_init(PIN_SD_TX);
    gpio_init(PIN_SD_SCK);
    gpio_init(PIN_SD_DET);
    gpio_set_function(PIN_SD_CSN, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SD_RX, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SD_TX, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SD_SCK, GPIO_FUNC_SPI);

    // Add spi_init and spi_set_format calls here.

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
