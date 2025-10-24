/*******************************************************************************
 * @file        main.c
 * @brief       Implements the functionality for the main module.
 * @details     This is the main file for the project.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * 
 * @note        This module is designed to be the main module to drive everything.
 *              Don't put interrupts here. Initialize everything, go through the
 *              startup process, and execute the lowest level non critical blocking
 *              tasks that potentially cause priority blocking when in handlers.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include <stdio.h>
#include "gps.h"
#include "imu.h"

#include "pico/stdlib.h"

/* ---------------------------- Private Constants --------------------------- */
#define GPS_RAW_TEST
// #define IMU_TEST
// #define LCD_SPI_TEST
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */

#ifdef GPS_RAW_TEST

int main() {

    return 0;
}

#endif


#ifdef IMU_TEST

int main() {

    return 0;
}

#endif


#ifdef LCD_SPI_TEST

int main() {

    return 0;
}

#endif