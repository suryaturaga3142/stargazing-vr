/*******************************************************************************
 * @file        display.h
 * @brief       Driver for LCD Displays
 * @details     A very high level C file for managing only the LCD and PIO
 *              initialization.
 * 
 * @see         display.c for implementation details.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * @version     1.0
 ******************************************************************************/

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include "structs.h"
#include "config.h"
#include "stdint.h"       
#include "hardware/pio.h"
#include "hardware/dma.h"

#define AMOUNT_OF_STARS 1000

#ifdef __cplusplus
extern "C" {
#endif

void display_init_star_cache(void);
// --- Display Logic Functions ---

/**
 * @brief Initializes the internal cache of star positions.
 * @note Call this once during setup before any draw operations.
 */
void display_init_star_cache(void);

/**
 * @brief Calculates the differences between old and new star positions and
 * populates the internal DMA transfer list.
 * @details This function compares the new matrix with the stored old one.
 * It generates a list of "erase" packets for moved stars,
 * followed by a list of "draw" packets for new positions.
 *
 * @param matrix A 2D array of [AMOUNT_OF_STARS][2] containing the new
 * (x, y) coordinates for every star.
 */
void place_new_stars(int matrix[AMOUNT_OF_STARS][2]);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H */

