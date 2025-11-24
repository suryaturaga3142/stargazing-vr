/*******************************************************************************
 * @file        monitor.c
 * @brief       Implements the functionality for the monitoring module.
 * @details     Watches over entire system with checkins for watchdogs.
 * 
 * @author      LED Chasers
 * @date        2025-11-23
 * 
 * @note        This module is designed to be driven by interrupts and is not
 *              intended to be called from a blocking main loop.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "monitor.h"
#include "hardware/watchdog.h"
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
static volatile uint8_t checkin_flags = 0;
const uint8_t ALL_SYSTEMS_GO = (SYS_MODULE_IMU | SYS_MODULE_GPS | SYS_MODULE_DISPLAY | SYS_MODULE_MAIN);
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Variables -------------------------- */

/* ----------------------------- Public Functions --------------------------- */


void monitor_checkin(system_module_t module) {
    // Atomically set the bit for this module
    // (In simple systems, |= is atomic enough, or use critical sections)
    checkin_flags |= module;
}

void monitor_update(void) {
    // Check if EVERYONE has reported in
    if (checkin_flags == ALL_SYSTEMS_GO) {
        watchdog_update(); // Kick the actual hardware watchdog
        checkin_flags = 0;
    }
    // If flags != ALL_SYSTEMS_GO, we do NOTHING.
    // Eventually, the hardware watchdog will time out and reset the RP2350.
}