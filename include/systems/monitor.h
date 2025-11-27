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
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Assign a unique bit to each critical subsystem
typedef enum {
    SYS_MODULE_IMU     = (1 << 0),
    SYS_MODULE_GPS     = (1 << 1),
    SYS_MODULE_DISPLAY = (1 << 2),
    SYS_MODULE_MAIN    = (1 << 3)
} system_module_t;

bool monitor_init(void);
void monitor_checkin(system_module_t module);
void monitor_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MONITOR_H */