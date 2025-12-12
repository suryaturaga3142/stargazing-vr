/*******************************************************************************
 * @file        display.h
 * @brief       DMA-Driven Graphics Interface for LCD
 * @details     Provides functions for initialization, coordinate scaling,
 *              grayscale generation, and building the DMA command packet
 *              for high-speed display updates.
 * * @author      LED Chasers
 * @date        2025-11-20
 * @version     2.1 (Split DMA Packet Architecture)
 ******************************************************************************/

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include <stddef.h> // For size_t
#include <stdint.h>
#include "structs.h" 
#include "config.h" 
#include "hardware/dma.h"
#include "rendering.h"

/* ---------------------------- Global Constants --------------------------- */

// LCD Resolution Definitions
#define SCREEN_WIDTH           320
#define SCREEN_HEIGHT          480

#define X_HALF (SCREEN_WIDTH / 2)
#define Y_HALF (SCREEN_HEIGHT / 2)

// Star Rendering Limits
#define AMOUNT_OF_STARS     9000

// Color Definitions (RGB565)
#define COLOR_WHITE         0xFFFFu
#define COLOR_BLACK         0x0000u

// DMA Buffer Size (Star Packet size = ~29 bytes)
#define BYTES_PER_STAR_PACKET 32 // Safe buffer size for one star (3x3 window)
// NOTE: We only need enough space to hold the data portion now, as commands are sent manually.
#define MAX_DMA_SIZE (AMOUNT_OF_STARS * 2 * BYTES_PER_STAR_PACKET) 

#ifdef __cplusplus
extern "C" {
#endif

bool display_init(void);
void erase_stars(StarPosition_t buffer[AMOUNT_OF_STARS], int count);
void draw_stars (StarPosition_t buffer[AMOUNT_OF_STARS], int count);

/* ----------------------------- Global Variables -------------------------- */

// The central buffer used for all DMA transactions 
extern uint32_t DMA_list[];
// The current valid length of the packet in bytes (reset by initialize_dma_packet)
extern size_t g_dma_packet_length;

/* ----------------------------- Core DMA/Packet Functions ------------------- */


void display_dma_wait_for_finish(void);
/**
 * @brief Sets the Data/Command (D/C) line state.
 * @param is_data If true (1), D/C is HIGH (Data Mode). If false (0), D/C is LOW (Command Mode).
 * @note This function requires the LCD_DC_PIN to be defined in config.h.
 */
void display_set_dc(bool is_data);

/**
 * @brief Resets the global DMA packet length counter to zero.
 * @note MUST be called once during setup AND once at the start of every
 * full frame drawing cycle before building packets.
 */
void initialize_dma_packet(void);

void beautiful_background();
void draw_paused_stars(StarPosition_t buffer[AMOUNT_OF_STARS], int count);
uint16_t quality_adjust(uint8_t magnitude);



void Clear_The_star(void);
/**
 * @brief Initializes the DMA channel for SPI-based display transfer.
 * @note Must be called once during system setup (e.g., in main.cpp).
 */
void display_dma_init(void);

/**
 * @brief Initiates a SPI transfer using DMA, but without waiting for completion.
 * @details This is only used for sending the large pixel data blocks.
 * @param source_ptr Pointer to the start of the data block in RAM.
 * @param length The number of bytes to transfer.
 * @note THIS DOES NOT HANDLE COMMANDS. D/C MUST BE HIGH (Data Mode).
 */
void display_dma_burst(const uint8_t *source_ptr, size_t length);

/**
 * @brief Sends a small block of data using blocking SPI transfer.
 * @details Used for sending single command bytes (D/C=0) or small parameter groups.
 * @param source_ptr Pointer to the data block.
 * @param length The number of bytes to transfer.
 * @note D/C line must be set appropriately *before* calling this function.
 */
void display_spi_blocking(const uint8_t *source_ptr, size_t length);


/**
 * @brief Converts a scalar brightness (0-255) to a 16-bit RGB565 grayscale color.
 * @param scalar Brightness value (0 = black, 255 = white).
 * @return uint16_t The 16-bit RGB565 color value.
 */
uint16_t get_grayscale_color(uint16_t scalar);

/**
 * @brief Prepares address and pixel data for erasure into the DMA list.
 * @details NOTE: THIS FUNCTION NOW ONLY WRITES ADDRESS PARAMETERS AND PIXEL DATA. 
 * THE COMMANDS (0x2A, 0x2B, 0x2C) MUST BE SENT BY THE CALLER (rendering.c).
 * * @param buffer Array containing packed (X_proj[31:16], Y_proj[15:0]) star coordinates.
 * @param start_index The star index to start buffering from.
 * @return size_t The number of bytes written for this star set.
 */
size_t buffer_star_data(uint32_t buffer[AMOUNT_OF_STARS], uint16_t magnitude[AMOUNT_OF_STARS], uint16_t color_mode, int start_index);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H */