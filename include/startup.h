/*******************************************************************************
 * @file        startup.h
 * @brief       Initial startup functions
 * @details     All the SD card pulling data, peripheral initialization, etc.
 * 
 * @see         startup.c for implementation details.
 * 
 * @author      LED Chasers
 * @date        2025-10-26
 * @version     1.0
 ******************************************************************************/

#ifndef STARTUP_H
#define STARTUP_H

#include <stdbool.h>

/**
 * @brief Executes the complete device startup sequence.
 * @details Initializes peripherals, loads data from SD card, processes star data,
 * acquires initial GPS fix, and prepares the system for the main loop.
 * @return true if startup was successful, false otherwise (enters error state).
 */
bool run_startup_sequence(void);

#endif /* STARTUP_H */