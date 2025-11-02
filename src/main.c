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
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...
#define IMU_SPI_TEST

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */
#ifdef IMU_SPI_TEST

#include "pico/stdlib.h"
#include <stdio.h>
#include "imu.h"

int main() {

    // Initialize standard I/O (for printf over USB)
    stdio_init_all();
    sleep_ms(4000); // Wait for terminal to connect

    printf("=====================================\n");
    printf("     BNO085 SPI Test\n");
    printf("=====================================\n");

    // Initialize the IMU
    init_spi_for_imu();
    printf("\nIMU data ready flag: %d", imu_data_ready_flag);

    while (true)
    {
        //printf("\nIMU data ready flag: %d", imu_data_ready_flag);
        if (imu_data_ready_flag)
        {
            parse_imu_rotation_data();

            //read_imu_data();
            //printf("\nw: %f,\tx: %f,\ty: %f,\tz: %f", current_rotation_vector.w, current_rotation_vector.x, current_rotation_vector.y, current_rotation_vector.z);
            //printf("\nFlag value: %d\n", imu_data_ready_flag);
        }
    }

    return 0;
}

#endif