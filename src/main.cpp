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

#include "structs.h"
#include "config.h"
#include "interrupts.h"

#include "display.h"
#include "gps.h"
#include "imu.h"
#include "sd_card.h"
#include "lcd.h"

#include "mechanics.h"
#include "monitor.h"
#include "rendering.h"
#include "user_ui.h"


void init_spi_lcd() {
    gpio_set_function(PIN_CS, GPIO_FUNC_SIO);
    gpio_set_function(PIN_DC, GPIO_FUNC_SIO);
    gpio_set_function(PIN_nRESET, GPIO_FUNC_SIO);

    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_set_dir(PIN_nRESET, GPIO_OUT);

    gpio_put(PIN_CS, 1); // CS high
    gpio_put(PIN_DC, 0); // DC low
    gpio_put(PIN_nRESET, 1); // nRESET high

    // --- CRITICAL FIX: Use SPI1 ---
    // GPIO 10 and 11 are hardwired to SPI1 on the RP2040.
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SDI, GPIO_FUNC_SPI);
    
    // Initialize SPI1 (not SPI0)
    int baudrate = spi_init(spi1, 100000000);
    printf("%d\r\n", baudrate);
    spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

int main()
{

    // PHASE 1: Power On Setup
    stdio_init_all();
    user_ui_init();
    printf("Starting up...\r\n");

    // Alerts if rebooted
    if (watchdog_caused_reboot()) {
        printf("Watchdog caused a reboot!\r\n");
        g_use_gps_location = false; // Switch off in case GPS reading caused the timeout
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
    init_spi_lcd();
    LCD_Setup();
    LCD_Clear(0x0000);

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

    /*for (int i = 0; i < 50; i++) {
        if (all_stars[i].mag != 0.0f) printf("Final Star %d: Mag %f x: %f y: %f z: %f\r\n", i, all_stars[i].mag, all_stars[i].x, all_stars[i].y, all_stars[i].z);
    }*/

    /* TEST VALUES */
    /*Star_t st = { .x = 0.0f, .y = 0.0f, .z = 1.0f, .mag = -1.5f };
    all_stars[sky_database[0][0].start_index] = st;
    st = { .x = 0.0f, .y = 0.0f, .z = -1.0f, .mag = 0.0f };
    all_stars[sky_database[0][0].start_index + 1] = st;
    st = { .x = 0.0f, .y = 1.0f, .z = 0.0f, .mag = 1.0f };
    all_stars[sky_database[0][0].start_index + 2] = st;
    st = { .x = 0.0f, .y = -1.0f, .z = 0.0f, .mag = -1.5f };
    all_stars[sky_database[0][0].start_index + 3] = st;
    st = { .x = 1.0f, .y = 0.0f, .z = 0.0f, .mag = 0.0f };
    all_stars[sky_database[0][0].start_index + 4] = st;
    st = { .x = -1.0f, .y = 0.0f, .z = 0.0f, .mag = 1.5f };
    all_stars[sky_database[0][0].start_index + 5] = st;
    */

    watchdog_update();
    printf("SD Card disabled & data sorted!\r\nInitializing GPS (This will take time)...\r\n");
    
    bool gps_fixed = gps_init();
    watchdog_disable();
    
    if (gps_fixed && g_use_gps_location && g_latest_gps_data.is_valid) {
        printf("GPS Lock found! Starting rendering...\r\nEnjoy!!\r\n");
        printf("Data: %d \r\nLat: %f \r\nLon: %f \r\nDate: %d/%d/%d \r\nTime: %d:%d:%d \r\n", 
            g_latest_gps_data.is_valid, g_latest_gps_data.latitude, g_latest_gps_data.longitude, 
            g_latest_gps_data.time.day, g_latest_gps_data.time.month, g_latest_gps_data.time.year,
            g_latest_gps_data.time.hour, g_latest_gps_data.time.minute, g_latest_gps_data.time.second);
        user_ui_set_state(LED_STATE_RUN);
    }
    else {
        printf("No GPS Lock found / avoiding due to reboot. Starting with no location fix.\r\nEnjoy!!\r\n");
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
        
        // If the read flag is turned on, the IMU is automatically read using the SHTP protocol and updated in the background.
        if (imu_check_and_read()) {
            monitor_checkin(SYS_MODULE_IMU);
            // printf("Game: %f %f %f %f\r\n", 
            //        g_latest_imu_data.orientation.x, g_latest_imu_data.orientation.y, g_latest_imu_data.orientation.z, g_latest_imu_data.orientation.w);
            run_main_render();
            monitor_checkin(SYS_MODULE_DISPLAY);
        }

        // Whenever this is called, the system CAN lag for at most GPS_TIMEOUT_MS. A small freeze will be noticed whenever the background GPS update task is carried out.
        // This is because GPS update and rendering is done in main, and only one can occur at any given point! No, we did not use both cores lol.
        // This lag is the cost of running precalculations to avoid massive rendering times.
        if (g_use_gps_location && gps_check_and_read() && g_latest_gps_data.is_valid) {

            /*
            printf("Data: %d \r\nLat: %f \r\nLon: %f \r\nDate: %d/%d/%d \r\nTime: %d:%d:%d \r\n", 
                g_latest_gps_data.is_valid, g_latest_gps_data.latitude, g_latest_gps_data.longitude, 
                g_latest_gps_data.time.day, g_latest_gps_data.time.month, g_latest_gps_data.time.year,
                g_latest_gps_data.time.hour, g_latest_gps_data.time.minute, g_latest_gps_data.time.second);
            */
            Qfix_t Qfix_latest;
            Qfix_latest.loc   = mech_location_to_q(g_latest_gps_data.latitude, g_latest_gps_data.longitude);
            Qfix_latest.time  = mech_time_to_q(mech_utc_to_sidereal(g_latest_gps_data.time));
            Qfix_latest.total = mech_product_q(Qfix_last.loc, Qfix_last.time);
            Qfix_last = Qfix_latest;
        }
        monitor_checkin(SYS_MODULE_GPS);

        if (g_use_gps_location) {
            if (g_latest_gps_data.is_valid) user_ui_set_state(LED_STATE_RUN);
            else                            user_ui_set_state(LED_STATE_RUN_NO_FIX);
        }
        else user_ui_set_state(LED_STATE_RUN_J2000);

        if (g_drift_correct_request) {
            user_ui_set_state(LED_STATE_DRIFT_CONFIRM); // It will get overwritten fast so it's just a blink
            imu_recenter_yaw();
            monitor_checkin(SYS_MODULE_IMU);
            g_drift_correct_request = false;
        }
        if (g_location_toggle_request) {
            g_use_gps_location = !g_use_gps_location;
            g_location_toggle_request = false;
        }

        monitor_checkin(SYS_MODULE_MAIN);
        sleep_ms(100);
    }

    // Should never reach here.
    for(;;) __wfi();

    return 0;
}