/*******************************************************************************
 * @file        startup.c
 * @brief       Implements the startup routines for the embedded system.
 * @details     Module does everything related to calling the startup routines.
 * 
 * @author      LED Chasers
 * @date        2025-11-23
 * 
 * @note        This module is designed to be driven by main and not have any 
 *              direct calling to peripherals. It wil call functions from others.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "startup.h"
#include "user_ui.h"
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Variables -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */

/**
 * @brief Starts up all user interface tasks
 */
void startup_ui(void) {
    // Call a function from user_ui.c
    // Initialize the Pushbuttons (Drift Correct, etc.) as inputs with pull-ups.
    // Initialize the RGB LED pins with PWM (1kHz frequency).
    // Set the LED state to "Booting" (Pulsing White).
    // Start a low-priority timer or task (e.g., 10Hz) to poll button states (with debouncing) and update the LED PWM based on the current system state enum.
}

void startup_check_sd(void) {
    // Set LED state to "Loading" (Pulsing Blue).
    // Check the Card Detect (DET) pin.
    // If NOT detected:
    //   - Enter a waiting loop (max 30 seconds).
    //   - Flash LED Yellow/Red.
    //   - Periodically check DET. Pet the watchdog.
    //   - Timeout: If 30s passes without a card, transition to "Critical Error" (Fast Blinking Red) and halt.
}

/**
 * @brief Checks for the IMU through an I2C scanner and sets up
 */
void startup_imu(void) {

}

/**
 * @brief Initializes the LCD display and clears screen
 */
void startup_lcd(void) {

}

/**
 * @brief TIME INTENSIVE. Carries out data collection and sorting
 */
void startup_sd(void) {

}

/**
 * @brief Starts up GPS collection and sets up background update
 */
void startup_gps(void) {

}