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
 * @brief Initializes the SD card and loads the star database into RAM.
 * @details This function:
 * 1. Calls sd_init().
 * 2. Reads the star file header from block 0.
 * 3. Validates the magic number.
 * 4. Calculates the total file size and required blocks.
 * 5. Reads the entire file into the global g_file_buffer.
 * 6. Sets the global g_star_array pointer and g_star_count.
 *
 * @return true if the database was loaded successfully, false otherwise.
 */
bool load_star_data(void);

#endif /* STARTUP_H */