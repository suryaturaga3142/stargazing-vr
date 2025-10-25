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

#include <stdint.h>

/**
 * @brief Initialize the SPI bus and the ILI9486 controller.
 *
 * Performs a hardware reset, sets up the SPI peripheral, and sends the
 * necessary commands to wake the display and set it to 24-bit (RGB888)
 * pixel format.
 */
void lcd_init(void);

/**
 * @brief Fills the entire 320x480 screen with a single 24-bit color.
 *
 * @param color The 24-bit (RGB888) color to fill with, in 0x00RRGGBB format.
 * Examples: 0x00FF0000 (RED), 0x0000FF00 (GREEN), 0x000000FF (BLUE)
 */
void lcd_fill_screen(uint32_t color);

#ifdef C1
#include <stdint.h>

/**
 * @brief Initialize the SPI bus and the ILI9486 controller.
 * * Performs a hardware reset, sets up the SPI peripheral, and sends the
 * necessary commands to wake the display and set it to 16-bit (RGB565)
 * pixel format.
 */
void lcd_init(void);

/**
 * @brief Fills the entire 320x480 screen with a single 16-bit color.
 * * @param color The 16-bit (RGB565) color to fill with.
 * Examples: 0xF800 (RED), 0x07E0 (GREEN), 0x001F (BLUE)
 */
void lcd_fill_screen(uint16_t color);

#endif

#endif /* DISPLAY_H */