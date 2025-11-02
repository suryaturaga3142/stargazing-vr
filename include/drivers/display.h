/*******************************************************************************
 * @file        display.h
 * @brief       Public interface for the ILI9486 LCD Display module.
 * @details     Provides functions to initialize, generate DMA packets, and
 * manage DMA transfers for updating the display.
 *
 * @author      LED Chasers
 * @date        2025-10-14
 *
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

#ifndef DISPLAY_H
#define DISPLAY_H

#include "stdint.h"
#include "stdbool.h"        // For 'bool' type
#include "hardware/pio.h"
#include "hardware/dma.h"

/* ----------------------------- Public Constants --------------------------- */

/**
 * @brief The total number of stars supported by the display module.
 * This defines the size of the coordinate matrix.
 */
#define AMOUNT_OF_STARS 1000

/* ------------------------ Public Function Prototypes ---------------------- */

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


// --- DMA Getter Functions ---

/**
 * @brief Gets a pointer to the master DMA transfer list.
 *
 * @return A pointer to the uint32_t array of PIO packets.
 */
uint32_t *display_get_dma_ptr(void);

/**
 * @brief Gets the total number of 32-bit packets in the master DMA list.
 *
 * @return The number of packets to transfer.
 */
int display_get_dma_count(void);


// --- DMA Control Functions ---

/**
 * @brief Claims and configures a DMA channel for the display.
 * @details Sets up a DMA channel to transfer 32-bit words from memory
 * to the PIO TX FIFO, paced by the PIO's DREQ signal. It also
 * configures a DMA interrupt to fire on transfer completion.
 *
 * @param pio The PIO instance (pio0 or pio1) running the LCD program.
 * @param sm The State Machine number (0-3) running the LCD program.
 */
void display_dma_init(PIO pio, uint sm);

/**
 * @brief Checks if the display DMA is currently busy.
 *
 * @return true if a transfer is in progress, false otherwise.
 */
bool display_dma_is_busy(void);

/**
 * @brief Starts a new DMA transfer.
 * @details Configures the DMA with the latest buffer pointer and count
 * from the display module and then triggers the transfer.
 * It will not start a new transfer if one is already busy.
 */
void display_dma_start_transfer(void);


#endif // DISPLAY_H