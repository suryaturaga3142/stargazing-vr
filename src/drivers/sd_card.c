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
#include "sd_card.h"

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
        return false;
    }
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

        char *ext = strrchr(fno.fname, '.');
        if (!ext || strcasecmp(ext, ".bin") != 0)
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


        // if (count < 3) {   // checks if stars are stored correctly
        //     printf("Star %lu:\n", (unsigned long)count);
        //     printf("  RA  = %ld\n", star.ra_scaled);
        //     printf("  DEC = %ld\n", star.dec_scaled);
        //     printf("  PMRA  = %d\n", star.pmra_scaled);
        //     printf("  PMDEC = %d\n", star.pmdec_scaled);
        //     printf("  MAG   = %d\n", star.mag_scaled);
        // }
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


/**
 * @brief prints any error with FatFs
 * 
 * @return
 */
void print_error(FRESULT fr, const char *msg)
{
    const char *errs[] = {
            [FR_OK] = "Success",
            [FR_DISK_ERR] = "Hard error in low-level disk I/O layer",
            [FR_INT_ERR] = "Assertion failed",
            [FR_NOT_READY] = "Physical drive cannot work",
            [FR_NO_FILE] = "File not found",
            [FR_NO_PATH] = "Path not found",
            [FR_INVALID_NAME] = "Path name format invalid",
            [FR_DENIED] = "Permision denied",
            [FR_EXIST] = "Prohibited access",
            [FR_INVALID_OBJECT] = "File or directory object invalid",
            [FR_WRITE_PROTECTED] = "Physical drive is write-protected",
            [FR_INVALID_DRIVE] = "Logical drive number is invalid",
            [FR_NOT_ENABLED] = "Volume has no work area",
            [FR_NO_FILESYSTEM] = "Not a valid FAT volume",
            [FR_MKFS_ABORTED] = "f_mkfs aborted",
            [FR_TIMEOUT] = "Unable to obtain grant for object",
            [FR_LOCKED] = "File locked",
            [FR_NOT_ENOUGH_CORE] = "File name is too large",
            [FR_TOO_MANY_OPEN_FILES] = "Too many open files",
            [FR_INVALID_PARAMETER] = "Invalid parameter",
    };
    if (fr < 0 || fr >= sizeof errs / sizeof errs[0])
        printf("%s: Invalid error\n", msg);
    else
        printf("%s: %s\n", msg, errs[fr]);
}

/**
 * @brief other stuff from the template that could fix random errors
 * 
 * @return
 */

 void sdcard_io_high_speed() 
{
    spi_set_baudrate(spi0, 12000000);
    // fill in.
}

void init_sdcard_io() 
{
    sd_init();
    disable_sdcard();
    // fill in.
}
const char *month_name[] = {
    [1] = "Jan",
    [2] = "Feb",
    [3] = "Mar",
    [4] = "Apr",
    [5] = "May",
    [6] = "Jun",
    [7] = "Jul",
    [8] = "Aug",
    [9] = "Sep",
    [10] = "Oct",
    [11] = "Nov",
    [12] = "Dec",
};
typedef union {
    struct {
        unsigned int bisecond:5; // seconds divided by 2
        unsigned int minute:6;
        unsigned int hour:5;
        unsigned int day:5;
        unsigned int month:4;
        unsigned int year:7;
    };
} fattime_t;

// Current time in the FAT file system format.
static fattime_t fattime;

void set_fattime(int year, int month, int day, int hour, int minute, int second)
{
    fattime_t newtime;
    newtime.year = year - 1980;
    newtime.month = month;
    newtime.day = day;
    newtime.hour = hour;
    newtime.minute = minute;
    newtime.bisecond = second/2;
    int len = sizeof newtime;
    memcpy(&fattime, &newtime, len);
}

void advance_fattime(void)
{
    fattime_t newtime = fattime;
    newtime.bisecond += 1;
    if (newtime.bisecond == 30) {
        newtime.bisecond = 0;
        newtime.minute += 1;
    }
    if (newtime.minute == 60) {
        newtime.minute = 0;
        newtime.hour += 1;
    }
    if (newtime.hour == 24) {
        newtime.hour = 0;
        newtime.day += 1;
    }
    if (newtime.month == 2) {
        if (newtime.day >= 29) {
            int year = newtime.year + 1980;
            if ((year % 1000) == 0) { // we have a leap day in 2000
                if (newtime.day > 29) {
                    newtime.day -= 28;
                    newtime.month = 3;
                }
            } else if ((year % 100) == 0) { // no leap day in 2100
                if (newtime.day > 28)
                newtime.day -= 27;
                newtime.month = 3;
            } else if ((year % 4) == 0) { // leap day for other mod 4 years
                if (newtime.day > 29) {
                    newtime.day -= 28;
                    newtime.month = 3;
                }
            }
        }
    } else if (newtime.month == 9 || newtime.month == 4 || newtime.month == 6 || newtime.month == 10) {
        if (newtime.day == 31) {
            newtime.day -= 30;
            newtime.month += 1;
        }
    } else {
        if (newtime.day == 0) { // cannot advance to 32
            newtime.day = 1;
            newtime.month += 1;
        }
    }
    if (newtime.month == 13) {
        newtime.month = 1;
        newtime.year += 1;
    }

    fattime = newtime;
}

uint32_t get_fattime(void)
{
    union FattimeUnion {
        fattime_t time;
        uint32_t value;
    };

    union FattimeUnion u;
    u.time = fattime;
    return u.value;
}

int to_int(char *start, char *end, int base)
{
    int n = 0;
    for( ; start != end; start++)
        n = n * base + (*start - '0');
    return n;
}