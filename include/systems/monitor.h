/*******************************************************************************
 * @file        monitor.h
 * @brief       Contains definitions for all watchdog related monitoring.
 * @details     All modules will call check-in from here for health monitoring.
 * 
 * @see         monitor.c for implementation details.
 * 
 * @author      LED Chasers
 * @date        2025-11-23
 * @version     1.0
 ******************************************************************************/

#ifndef MONITOR_H
#define MONITOR_H

#include <stdint.h>

// Assign a unique bit to each critical subsystem
typedef enum {
    SYS_MODULE_IMU     = (1 << 0),
    SYS_MODULE_GPS     = (1 << 1),
    SYS_MODULE_DISPLAY = (1 << 2),
    SYS_MODULE_MAIN    = (1 << 3)
} system_module_t;

// Called by drivers to confirm health
void monitor_checkin(system_module_t module);

// Called by main loop to verify all health and pet the watchdog
void monitor_update(void);

#endif /* MONITOR_H */