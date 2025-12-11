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
#include "config.h"
#include "monitor.h"
#include "interrupts.h"
#include "hardware/watchdog.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
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

/**
 * @brief Sets up a repeating timer for the supervisor
 * 
 */
bool monitor_init(void) {
    timer0_hw->inte |= TIMER_INTE_ALARM_0_BITS;
    irq_set_exclusive_handler(TIMER0_IRQ_0, irq_timer_monitor_callback);
    irq_set_enabled(TIMER0_IRQ_0, true);
    timer0_hw->alarm[0] = timer_hw->timerawl + (WATCHDOG_SUPERVISOR_INTERVAL_MS * 1000);
    return true;
}

/**
 * @brief Allows main to verify health of a certain part.
 * 
 * @param module The module checking in
 */
void monitor_checkin(system_module_t module) {
    // Atomically set the bit for this module
    // (In simple systems, |= is atomic enough, or use critical sections)
    checkin_flags |= module;
}

/**
 * @brief Called by a repeating timer ISR to verify all health
 * 
 */
void monitor_update(void) {
    // Check if EVERYONE has reported in
    if (checkin_flags == ALL_SYSTEMS_GO) {
        watchdog_update(); // Kick the actual hardware watchdog
        checkin_flags = 0;
    }
    // If flags != ALL_SYSTEMS_GO, we do NOTHING.
    // Eventually, the hardware watchdog will time out and reset the system.
}