/*******************************************************************************
 * @file        main.cpp
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
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"

#include "config.h"
#include "structs.h"
#include "interrupts.h"

#include "display.h"
#include "gps.h"
#include "imu.h"
#include "sd_card.h"

#include "mechanics.h"
#include "monitor.h"
#include "rendering.h"
#include "user_ui.h"


int main()
{
    // PHASE 1: Power On Setup
    stdio_init_all();
    user_ui_init();
    printf("Starting up...\r\n");

    // Alerts if rebooted
    if (watchdog_caused_reboot()) {
        printf("Watchdog caused a reboot!\r\n");
        user_ui_set_state(LED_STATE_REBOOTED);
        sleep_ms(2000);
        user_ui_set_state(LED_STATE_BOOTING);
    }
    // Setup watchdog with 5sec timeout during startup procedures. Pet it during long processes.
    watchdog_enable(WATCHDOG_INIT_TIMEOUT_MS, true);

    // PHASE 2: Connectivity Check
    printf("Initializing peripherals...\r\nSearching SD Card...\r\n");
    bool sd_alive = sd_init();

    // If not present, run a 10 second warning while polling sd_check()
    if (!sd_alive) {
        printf("No SD Card detected! Please insert one.\r\nWaiting...\r\n");
        user_ui_set_state(LED_STATE_SD_LOADING);
        for (int i = 0; i < 100; i++) {
            sleep_ms(100);
            watchdog_update();
            sd_alive = sd_check();
            if (sd_alive) break;
        }
    }
    // If a timeout occured the error state is entered.
    if (!sd_alive) {
        watchdog_disable();
        printf("Error: SD Card Connectivity Timeout Occured. SD Card missing / unresponsive.\r\nPlease insert working SD Card and restart.\r\n");
        user_ui_set_state(LED_STATE_ERR_CRITICAL);
        for(;;) {
            __wfi();
        }
    }
    user_ui_set_state(LED_STATE_BOOTING);
    watchdog_update();

    printf("SD Card alive!\r\nChecking IMU...\r\n");
    bool imu_connected = imu_init();     // Check IMU presence and initialize, set to true for sd testing, set back to imu_init() otherwise 
    if (imu_connected) watchdog_update();
    else {
        watchdog_disable();
        printf("Error: IMU not detected!\r\nPlease check connections and restart.\r\n");
        user_ui_set_state(LED_STATE_ERR_CRITICAL);
        for (;;) {
            __wfi();
        }
    }

    printf("IMU detected!\r\nStarting LCD...\r\n");
    display_init(); // Initialize PIO related stuff, it'll be a fast function.
    watchdog_update();

    // PHASE 3: Heavy Lifting (PET THE WATCHDOG MANY TIMES!)
    printf("Initialized LCD!\r\nMounting & Bulk Reading SD Card...\r\n");
    bool data_loaded = sd_load_data(); // Mount and bulk read the SD card data
    if (data_loaded) watchdog_update();
    else {
        watchdog_disable();
        printf("Error: Unable to read SD Card. Check if stars.bin is present & correct?\r\n");
        user_ui_set_state(LED_STATE_ERR_CRITICAL);
        for (;;) {
            __wfi();
        }
    }
    printf("Data Read!\r\nDisabling card & sorting data...\r\n");
    disable_sdcard();

    watchdog_update();
    sd_buf_sort();  // Sorts data in 3 pass algorithm
    
    watchdog_update();
    printf("SD Card disabled & data sorted!\r\nInitializing GPS (This will take time)...\r\n");
    
    bool gps_fixed = gps_init();
    watchdog_disable();
    
    if (gps_fixed) {
        printf("GPS Lock found! Starting rendering...\r\nEnjoy!!\r\n");
        user_ui_set_state(LED_STATE_RUN);
    }
    else {
        printf("No GPS Lock found. Starting with no location fix.\r\nEnjoy!!\r\n");
        user_ui_set_state(LED_STATE_RUN_NO_FIX);
    }
    
    // PHASE 4: Handover process
    // Reconfigure watchdog for main loop checking and setup monitoring supervisor
    irq_set_priority(IO_IRQ_BANK0  , 0x10);   // IMU INT and PB
    irq_set_priority(TIMER0_IRQ_1  , 0x20);   // GPS data
    irq_set_priority(PWM_IRQ_WRAP_0, 0x30);   // RGB LED State
    irq_set_priority(TIMER0_IRQ_0  , 0x40);   // Monitor Checkin
    monitor_init();
    watchdog_enable(WATCHDOG_TIMEOUT_MS, true);

    // Loop
    while (true) {

        // Sleep until an interrupt fires
        __wfi();

        // Awake now bc interrupt fired. Do the events in order of priority.
        if (imu_check_and_read()) {
            monitor_checkin(SYS_MODULE_IMU);
            // printf("Game: %f %f %f %f\r\n", g_latest_imu_data.orientation.x, g_latest_imu_data.orientation.y, g_latest_imu_data.orientation.z, g_latest_imu_data.orientation.w);
            run_main_render();
            // Calculate draw list
            monitor_checkin(SYS_MODULE_DISPLAY);
            // Trigger DMA
        }

        // READ GPS IN THE SAME METHOD AS IMU
        if (gps_check_and_read()) {
            // Update the location quaternion
            // Update the RTC
        }
        monitor_checkin(SYS_MODULE_GPS);

        if (g_drift_correct_request) {
            user_ui_set_state(LED_STATE_DRIFT_CONFIRM);
            imu_recenter_yaw();
        }
        if (g_use_actual_gps) {
            // Overwrite the used location quaternion with either the new GPS quaternion or the in built one.
            // Note; might be better to use the toggle request to avoid excess branching
        }

        monitor_checkin(SYS_MODULE_MAIN);
    }

    // Should never reach here.
    for(;;) __wfi();

    return 0;
}