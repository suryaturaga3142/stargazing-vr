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
#include "config.h"
#include "imu.h"
#include "interrupts.h"

#include "bno08x.h"
#include "utils.h"

#include "hardware/watchdog.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
static BNO08x imu;
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Variables --------------------------- */
volatile bool g_imu_data_ready = false;    // Set by IMU ISR when new data is available
volatile IMUData_t g_latest_imu_data = {0};

/* ----------------------------- Public Functions --------------------------- */

/**
 * @brief Initializes the IMU in I2C and configures Game Rotation Vectors with interrupt
 * 
 * @return true if initialization was successful
 */
bool imu_init(void) {
    // Initialize the IMU module in I2C, configure game rotation vectors with interrupts at 200Hz.
    sleep_ms(3000);
    
    gpio_init(PIN_IMU_INT);
    gpio_init(PIN_IMU_RST);
    gpio_set_dir(PIN_IMU_RST, true);
    gpio_set_dir(PIN_IMU_INT, false);

    gpio_put(PIN_IMU_RST, 0);
    sleep_ms(10);
    gpio_put(PIN_IMU_RST, 1);
    sleep_ms(300);
    
    watchdog_update();

    i2c_inst* i2c_port = I2C_PORT;
    initI2C(i2c_port, false); //given by imu library

    if (!imu.begin(IMU_I2C_ADDR, i2c_port)) {
        printf("IMU not detected!\n");
        watchdog_update();
        // Scan bus to debug connection issues (does NOT auto-scan)
        scan_i2c_bus(); 
        printf("/r/n[IMU] Critical Failure: Connection Timeout.\n");
        return false;
    }
    
    watchdog_update();
    sleep_ms(500);

    printf("[IMU] Connected! Configuring Reports...\n");

    watchdog_update();

    if (!imu.enableGameRotationVector(TARGET_REFRESH_PERIOD_MS)) {
        printf("[IMU] Failed to enable Game Rotation Vector!\n");
        return false;
    }

    gpio_set_irq_enabled(PIN_IMU_INT, GPIO_IRQ_EDGE_FALL, true);

    g_imu_data_ready = false;

    printf("[IMU] Init Complete. Running at %d Hz.\n", TARGET_REFRESH_RATE_HZ);

    watchdog_update();

    return true;
}

/**
 * @brief Checks ready flag and reads IMU data with I2C. Call in main loop.
 * 
 * @return true if new data was processed successfully.
 */
bool imu_check_and_read(void) {

    if (!g_imu_data_ready && gpio_get(PIN_IMU_INT)) {
        return false;
    }

    if (imu.getSensorEvent()) {
        // Switch off flag before reading
        g_imu_data_ready = false;

        if (imu.getSensorEventID() == SH2_GAME_ROTATION_VECTOR) {
            g_latest_imu_data.orientation.w = imu.getGameQuatReal();
            g_latest_imu_data.orientation.x = imu.getGameQuatI();
            g_latest_imu_data.orientation.y = imu.getGameQuatJ();
            g_latest_imu_data.orientation.z = imu.getGameQuatK();

            g_latest_imu_data.velocity.x = 0.0f;
            g_latest_imu_data.velocity.y = 0.0f;
            g_latest_imu_data.velocity.z = 0.0f;

            g_latest_imu_data.timestamp_us = to_us_since_boot(get_absolute_time());
            return true;
        }
    }
    return false;
}
