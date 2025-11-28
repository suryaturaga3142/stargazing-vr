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
#include "hardware/watchdog.h"  
#include "pico/stdlib.h"
#include "ff.h"
#include "diskio.h"
#include <stdio.h>
#include <string.h>
#include "sdcard.h"

// ...

/* ---------------------------- Private Constants --------------------------- */
static const double SCALER_COORD = 1000000.0;
static const double SCALER_MAG   = 1000.0;
// ...
FATFS fs_storage; // Global file system object
#define STAR_MAGIC 0x53544152  // "STAR" = 0x53 54 41 52
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
        return true;
    }
    return false;
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
bool sd_load_data(void) 
{
    FATFS *fs = &fs_storage;

    if (fs->id != 0) {
        print_error(FR_DISK_ERR, "Already mounted.");
        return false;
    }

    FRESULT fr = f_mount(fs, "", 1);
    if (fr != FR_OK) {
        print_error(fr, "Error mounting SD card");
        return false;
    }

    DIR dir;
    FILINFO fno;

    /* 1. Open root directory */
    fr = f_opendir(&dir, "/");
    if (fr != FR_OK) {
        print_error(fr, "opendir");
        f_mount(NULL, "", 1);  // unmount
        return false;
    }

    char found_file[64] = {0};

    /* 2. Scan directory for a .bin file with valid STAR header */
    while (1) {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK || fno.fname[0] == 0)
            break; // error OR end of dir

        if (fno.fattrib & AM_DIR)
            continue;

        if (!strstr(fno.fname, ".bin"))
            continue;

        FIL fil;
        fr = f_open(&fil, fno.fname, FA_READ);
        if (fr != FR_OK)
            continue;

        StarFileHeader_t header;
        UINT br;
        fr = f_read(&fil, &header, sizeof(header), &br);
        f_close(&fil);

        if (fr != FR_OK || br != sizeof(header))
            continue;

        /* Validate header */
        if (header.magic_number != STAR_MAGIC)
            continue;

        if (header.header_size < sizeof(StarFileHeader_t))
            continue;

        /* Found a valid STAR file */
        strcpy(found_file, fno.fname);
        break;
    }

    f_closedir(&dir);

    if (!found_file[0]) {
        printf("No valid STAR binary file found.\n");
        f_mount(NULL, "", 1);
        return false;
    }

    printf("Found STAR file: %s\n", found_file);

    /* 3. Open the validated STAR file */
    FIL fil;
    fr = f_open(&fil, found_file, FA_READ);
    if (fr != FR_OK) {
        print_error(fr, found_file);
        f_mount(NULL, "", 1);
        return false;
    }

    /* Skip header */
    f_lseek(&fil, sizeof(StarFileHeader_t));

    /* 4. Read star records */
    PackedStar_t star;
    UINT br;
    uint32_t count = 0;

    while (1) {
        fr = f_read(&fil, &star, sizeof(PackedStar_t), &br);

        if (fr != FR_OK) {
            print_error(fr, found_file);
            f_close(&fil);
            f_mount(NULL, "", 1);
            return false;
        }
        if (br == 0)
            break; // EOF

        /* Process star */
        // unpack_star(&star);
        count++;
    }

    f_close(&fil);

    printf("Loaded %u stars\n", count);

    /* Leave SD mounted — caller can unmount if desired */
    return true;
}


/**
 * @brief disables the SD card
 * 
 * @return true if both processes were successful
 */
bool disable_sdcard(void) 
{
    gpio_put(PIN_SD_CSN, 1);

    // Provide required extra clocks
    uint8_t temp = 0xFF;
    spi_write_blocking(spi0, &temp, 1);

    // Release MOSI line — must be high when idle
    gpio_set_function(PIN_SD_TX, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_SD_TX, GPIO_OUT);
    gpio_put(PIN_SD_TX, 1);
    // Unmount the SD card and deinitialize the interface.
    return true;
}

/**
 * @brief enables the SD card
 *
 * @return 
 */
void enable_sdcard() 
{
    gpio_put(PIN_SD_CSN, 0);
    gpio_set_function(PIN_SD_TX, GPIO_FUNC_SPI);
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
