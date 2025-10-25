/*******************************************************************************
 * @file        display.c
 * @brief       Implements the initialization for the LCD Display module.
 * @details     Very high level for initializing and passing draw commands. 
 *              All low level complexity handled by PIO.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * 
 * @note        This module is designed to be driven by interrupts and is not
 *              intended to be called from a blocking main loop. Only the 
 *              initialization is.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

#include "display.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"

// --- Pin Configuration (Based on our SPI-1 plan) ---
// Note: We define these here because we are not using config.h for this test.
#define LCD_SPI_PORT    spi1
#define PIN_SCK         38  // SCL (SPI Clock)
#define PIN_MOSI        36  // SDA (SPI Data In)
#define PIN_MISO        37  // SDA-0 (SPI Data Out) - Unused but defined
#define PIN_CS          39
#define PIN_DC          35
#define PIN_RST         34
// BL (Backlight) is tied to 3.3V

// Display dimensions
#define LCD_WIDTH       320
#define LCD_HEIGHT      480

// ILI9486 Commands
#define ILI9486_CMD_SOFTWARE_RESET      0x01
#define ILI9486_CMD_SLEEP_OUT           0x11
#define ILI9486_CMD_COLUMN_ADDRESS_SET  0x2A
#define ILI9486_CMD_PAGE_ADDRESS_SET    0x2B
#define ILI9486_CMD_MEMORY_WRITE        0x2C
#define ILI9486_CMD_PIXEL_FORMAT_SET    0x3A
#define ILI9486_CMD_DISPLAY_ON          0x29

/**
 * @brief Toggles the reset pin to perform a hardware reset.
 */
static void lcd_reset(void) {
    // Datasheet: Reset pin must be low for at least 10us
    gpio_put(PIN_RST, 0); // Reset pin is active low
    sleep_ms(10); // 10ms is plenty
    gpio_put(PIN_RST, 1);
    // Datasheet: Wait 5ms after reset before sending commands
    sleep_ms(10); 
}

/**
 * @brief Selects the display (active low).
 */
static inline void lcd_cs_select(void) {
    gpio_put(PIN_CS, 0);
}

/**
 * @brief Deselects the display (idle high).
 */
static inline void lcd_cs_deselect(void) {
    gpio_put(PIN_CS, 1);
}

/**
 * @brief Sets the D/C pin to Command mode.
 */
static inline void lcd_dc_command(void) {
    gpio_put(PIN_DC, 0);
}

/**
 * @brief Sets the D/C pin to Data mode.
 */
static inline void lcd_dc_data(void) {
    gpio_put(PIN_DC, 1);
}

/**
 * @brief Sends a single command byte to the display.
 */
static void lcd_send_cmd(uint8_t cmd) {
    lcd_dc_command();
    lcd_cs_select();
    spi_write_blocking(LCD_SPI_PORT, &cmd, 1);
    lcd_cs_deselect();
}

/**
 * @brief Sends a single data byte to the display.
 */
static void lcd_send_data(uint8_t data) {
    lcd_dc_data();
    lcd_cs_select();
    spi_write_blocking(LCD_SPI_PORT, &data, 1);
    lcd_cs_deselect();
}

/**
 * @brief Sets the hardware "window" for subsequent memory writes.
 */
static void lcd_set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end) {
    // Column Address Set (0x2A)
    lcd_send_cmd(ILI9486_CMD_COLUMN_ADDRESS_SET);
    lcd_dc_data();
    lcd_cs_select();
    uint8_t col_data[4] = { (x_start >> 8) & 0xFF, x_start & 0xFF, (x_end >> 8) & 0xFF, x_end & 0xFF };
    spi_write_blocking(LCD_SPI_PORT, col_data, 4);
    lcd_cs_deselect();

    // Page Address Set (0x2B)
    lcd_send_cmd(ILI9486_CMD_PAGE_ADDRESS_SET);
    lcd_dc_data();
    lcd_cs_select();
    uint8_t row_data[4] = { (y_start >> 8) & 0xFF, y_start & 0xFF, (y_end >> 8) & 0xFF, y_end & 0xFF };
    spi_write_blocking(LCD_SPI_PORT, row_data, 4);
    lcd_cs_deselect();
}

// --- Public Functions ---

void lcd_init(void) {
    // --- 1. Initialize GPIO pins ---
    gpio_init(PIN_CS);
    gpio_init(PIN_DC);
    gpio_init(PIN_RST);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    
    gpio_put(PIN_CS, 1); // CS idle high
    gpio_put(PIN_DC, 1); // D/C idle high
    gpio_put(PIN_RST, 1); // RST idle high

    // --- 2. Initialize SPI peripheral ---
    // Set SPI speed to 10 MHz.
    // The ILI9486 max SPI speed is ~15 MHz.
    spi_init(LCD_SPI_PORT, 10 * 1000 * 1000); 
    
    // Set SPI format (8 bits, CPOL 0, CPHA 0)
    spi_set_format(LCD_SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // Map SPI pins to GPIO
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);  // 38
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI); // 36
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI); // 37 (not used, but mapped)

    // --- 3. Reset and configure the display ---
    lcd_reset(); // Hardware reset

    // Command 1: Exit Sleep Mode
    lcd_send_cmd(ILI9486_CMD_SLEEP_OUT);
    sleep_ms(120); // Must wait 120ms after SLEEP_OUT

    // Command 2: Set Pixel Format
    // *** FIX: Changed 0x55 (16-bit) to 0x66 (24-bit) ***
    lcd_send_cmd(ILI9486_CMD_PIXEL_FORMAT_SET);
    lcd_send_data(0x66); // 0x66 = 24-bit/pixel (RGB888)

    // Command 3: Turn the display on
    lcd_send_cmd(ILI9486_CMD_DISPLAY_ON);
    sleep_ms(50);
}

void lcd_fill_screen(uint32_t color) {
    // Set the window to the entire screen
    lcd_set_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

    // Send the "Memory Write" command
    lcd_send_cmd(ILI9486_CMD_MEMORY_WRITE);

    // Set D/C to Data mode and select chip
    lcd_dc_data();
    lcd_cs_select();

    // *** FIX: Create a buffer for 3-byte (24-bit) pixels ***
    #define PIXEL_PER_LINE 320
    #define BYTES_PER_PIXEL 3
    #define BUFFER_SIZE (PIXEL_PER_LINE * BYTES_PER_PIXEL) // 320 * 3 = 960 bytes
    
    static uint8_t line_buffer[BUFFER_SIZE];
    
    // Unpack the 24-bit color
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    // Fill the line buffer with this color
    for (int i = 0; i < BUFFER_SIZE; i += 3) {
        line_buffer[i]     = r;
        line_buffer[i + 1] = g;
        line_buffer[i + 2] = b;
    }

    // Write the buffer to the screen for each line
    for (int y = 0; y < LCD_HEIGHT; y++) {
        spi_write_blocking(LCD_SPI_PORT, line_buffer, BUFFER_SIZE);
    }
    
    lcd_cs_deselect();
}




#ifdef CC
/* ----------------------------- Private Includes --------------------------- */
#include "display.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"

// --- Pin Configuration (Based on our SPI-1 plan) ---
// Note: We define these here because we are not using config.h for this test.
#define LCD_SPI_PORT    spi1
#define PIN_SCK         38  // SCL (SPI Clock)
#define PIN_MOSI        36  // SDA (SPI Data In)
#define PIN_MISO        37  // SDA-0 (SPI Data Out) - Unused but defined
#define PIN_CS          39
#define PIN_DC          35
#define PIN_RST         34
// BL (Backlight) is tied to 3.3V

// Display dimensions
#define LCD_WIDTH       320
#define LCD_HEIGHT      480

// ILI9486 Commands
#define ILI9486_CMD_SOFTWARE_RESET      0x01
#define ILI9486_CMD_SLEEP_OUT           0x11
#define ILI9486_CMD_COLUMN_ADDRESS_SET  0x2A
#define ILI9486_CMD_PAGE_ADDRESS_SET    0x2B
#define ILI9486_CMD_MEMORY_WRITE        0x2C
#define ILI9486_CMD_PIXEL_FORMAT_SET    0x3A
#define ILI9486_CMD_DISPLAY_ON          0x29

/**
 * @brief Toggles the reset pin to perform a hardware reset.
 */
static void lcd_reset(void) {
    // Datasheet: Reset pin must be low for at least 10us
    gpio_put(PIN_RST, 0); // Reset pin is active low
    sleep_ms(10); // 10ms is plenty
    gpio_put(PIN_RST, 1);
    // Datasheet: Wait 5ms after reset before sending commands
    sleep_ms(10); 
}

/**
 * @brief Selects the display (active low).
 */
static inline void lcd_cs_select(void) {
    gpio_put(PIN_CS, 0);
}

/**
 * @brief Deselects the display (idle high).
 */
static inline void lcd_cs_deselect(void) {
    gpio_put(PIN_CS, 1);
}

/**
 * @brief Sets the D/C pin to Command mode.
 */
static inline void lcd_dc_command(void) {
    gpio_put(PIN_DC, 0);
}

/**
 * @brief Sets the D/C pin to Data mode.
 */
static inline void lcd_dc_data(void) {
    gpio_put(PIN_DC, 1);
}

/**
 * @brief Sends a single command byte to the display.
 */
static void lcd_send_cmd(uint8_t cmd) {
    lcd_dc_command();
    lcd_cs_select();
    spi_write_blocking(LCD_SPI_PORT, &cmd, 1);
    lcd_cs_deselect();
}

/**
 * @brief Sends a single data byte to the display.
 */
static void lcd_send_data(uint8_t data) {
    lcd_dc_data();
    lcd_cs_select();
    spi_write_blocking(LCD_SPI_PORT, &data, 1);
    lcd_cs_deselect();
}

/**
 * @brief Sets the hardware "window" for subsequent memory writes.
 */
static void lcd_set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end) {
    // Column Address Set (0x2A)
    lcd_send_cmd(ILI9486_CMD_COLUMN_ADDRESS_SET);
    lcd_dc_data();
    lcd_cs_select();
    uint8_t col_data[4] = { (x_start >> 8) & 0xFF, x_start & 0xFF, (x_end >> 8) & 0xFF, x_end & 0xFF };
    spi_write_blocking(LCD_SPI_PORT, col_data, 4);
    lcd_cs_deselect();

    // Page Address Set (0x2B)
    lcd_send_cmd(ILI9486_CMD_PAGE_ADDRESS_SET);
    lcd_dc_data();
    lcd_cs_select();
    uint8_t row_data[4] = { (y_start >> 8) & 0xFF, y_start & 0xFF, (y_end >> 8) & 0xFF, y_end & 0xFF };
    spi_write_blocking(LCD_SPI_PORT, row_data, 4);
    lcd_cs_deselect();
}

// --- Public Functions ---

void lcd_init(void) {
    // --- 1. Initialize GPIO pins ---
    gpio_init(PIN_CS);
    gpio_init(PIN_DC);
    gpio_init(PIN_RST);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    
    gpio_put(PIN_CS, 1); // CS idle high
    gpio_put(PIN_DC, 1); // D/C idle high
    gpio_put(PIN_RST, 1); // RST idle high

    // --- 2. Initialize SPI peripheral ---
    
    // *** FIX: Changed 62 MHz to 10 MHz. ***
    // The ILI9486 max SPI speed is ~15 MHz. 62 MHz was too fast.
    // 10 MHz is fast but very safe.
    spi_init(LCD_SPI_PORT, 10 * 1000 * 1000); 
    
    // Set SPI format (8 bits, CPOL 0, CPHA 0)
    spi_set_format(LCD_SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // Map SPI pins to GPIO
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);  // 38
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI); // 36
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI); // 37 (not used, but mapped)

    // --- 3. Reset and configure the display ---
    // This sequence is based on the ILI9486 datasheet power-on sequence.
    lcd_reset(); // Hardware reset

    // Command 1: Exit Sleep Mode
    lcd_send_cmd(ILI9486_CMD_SLEEP_OUT);
    sleep_ms(120); // Must wait 120ms after SLEEP_OUT

    // Command 2: Set Pixel Format to 16 bits per pixel (RGB565)
    lcd_send_cmd(ILI9486_CMD_PIXEL_FORMAT_SET);
    lcd_send_data(0x55); // 0x55 = 16-bit/pixel

    // Command 3: Turn the display on
    lcd_send_cmd(ILI9486_CMD_DISPLAY_ON);
    sleep_ms(50);
}

void lcd_fill_screen(uint16_t color) {
    // Set the window to the entire screen
    lcd_set_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

    // Send the "Memory Write" command
    lcd_send_cmd(ILI9486_CMD_MEMORY_WRITE);

    // Set D/C to Data mode and select chip
    lcd_dc_data();
    lcd_cs_select();

    // Prepare a small buffer (e.g., one line)
    // This is much faster than sending two bytes at a time.
    #define BUFFER_SIZE 640 // 320 pixels * 2 bytes/pixel
    uint8_t line_buffer[BUFFER_SIZE];
    
    // Split the 16-bit color into two 8-bit bytes (Big Endian)
    uint8_t color_hi = (color >> 8) & 0xFF;
    uint8_t color_lo = color & 0xFF;

    for (int i = 0; i < BUFFER_SIZE; i += 2) {
        line_buffer[i]     = color_hi;
        line_buffer[i + 1] = color_lo;
    }

    // Write the buffer to the screen for each line
    for (int y = 0; y < LCD_HEIGHT; y++) {
        spi_write_blocking(LCD_SPI_PORT, line_buffer, BUFFER_SIZE);
    }
    
    lcd_cs_deselect();
}

#endif