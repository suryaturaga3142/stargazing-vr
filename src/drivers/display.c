#include "display.h"
#include "hardware/spi.h" 
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdio.h> 
#include "config.h" // Assumed to define SPI_PORT (e.g., spi1) and LCD_DC_PIN
#include "lcd.h"    // Assumed to define LCD_WIDTH/HEIGHT

// // --- DMA/SPI HARDWARE CONFIGURATION ---

// // NOTE: SPI_PORT must be defined in config.h 
// #define DMA_SPI_TX_DREQ (SPI_PORT == spi0 ? DREQ_SPI0_TX : DREQ_SPI1_TX)
// #define SPI_TX_ADDRESS ((volatile void*)&spi_get_hw(SPI_PORT)->dr)

// // --- GLOBAL DMA BUFFER & STATE DEFINITION ---

// // DEFINITION: This allocates the memory for the global DMA_list.
// // Although the "giant packet" concept is gone, this array is still used by 
// // display_dma_transfer/burst as a temporary pointer target.
// uint32_t DMA_list[(MAX_DMA_SIZE + 3) / 4];

// // Global variable tracking the total length of the packet in bytes.
// size_t g_dma_packet_length = 0;

// static int g_dma_chan; // DMA channel allocated at init


// // --- PRIVATE HELPER FUNCTIONS ---

// /**
//  * @brief Appends a 16-bit color value as two separate bytes (High then Low) to the DMA buffer.
//  * @param ptr Current pointer position in the DMA buffer (uint8_t*).
//  * @param color The 16-bit RGB565 color value.
//  * @return uint8_t* The updated pointer position after writing 2 bytes.
//  */
// static uint8_t* append_color_bytes(uint8_t *ptr, uint16_t color)
// {
//     // High Byte 
//     *ptr++ = (uint8_t)(color >> 8);
//     // Low Byte
//     *ptr++ = (uint8_t)color;
//     return ptr;
// }


// // --- CORE LOGIC FUNCTIONS ---

// /**
//  * @brief Sets the Data/Command (D/C) line state.
//  * @param is_data If true (1), D/C is HIGH (Data Mode). If false (0), D/C is LOW (Command Mode).
//  */
// void display_set_dc(bool is_data) {
//     gpio_put(LCD_DC_PIN, is_data);
// }

// /**
//  * @brief Sends a small block of data using blocking SPI transfer.
//  */
// void display_spi_blocking(const uint8_t *source_ptr, size_t length) {
//     spi_write_blocking(SPI_PORT, source_ptr, length);
// }

// /**
//  * @brief Blocks until the current DMA transfer on the assigned channel is complete.
//  * @details This is used to ensure the previous burst of pixel data is fully sent 
//  * before the CPU issues the next command.
//  */
// void display_dma_wait_for_finish(void) {
//     if (g_dma_chan >= 0) {
//         // Block until the DMA channel is free/finished
//         dma_channel_wait_for_finish_blocking(g_dma_chan);
//     }
// }
// /**
//  * @brief Initiates a SPI transfer using DMA, but without waiting for completion.
//  */
// void display_dma_burst(const uint8_t *source_ptr, size_t length) {
//     if (length == 0 || g_dma_chan < 0) return;

//     // --- REMOVED: dma_channel_wait_for_finish_blocking() and dma_channel_abort() ---
    
//     // Set the source address and the transfer count
//     dma_channel_set_read_addr(g_dma_chan, source_ptr, false);
//     dma_channel_set_trans_count(g_dma_chan, length, false);
    
//     // Start the transfer.
//     dma_channel_start(g_dma_chan);
// }


// /**
//  * @brief Resets the global DMA packet length counter to zero.
//  */
// void initialize_dma_packet(void) {
//     g_dma_packet_length = 0;
// }

// /**
//  * @brief Initializes the DMA channel for SPI-based display transfer.
//  */
// void display_dma_init(void) {
//     // 1. Initialize the D/C pin
//     gpio_init(LCD_DC_PIN);
//     gpio_set_dir(LCD_DC_PIN, GPIO_OUT);
    
//     // 2. Claim a free DMA channel
//     g_dma_chan = dma_claim_unused_channel(true);
    
//     if (g_dma_chan < 0) {
//         printf("ERROR: Failed to claim unused DMA channel!\n");
//         return;
//     }

//     // 3. Configure the channel for SPI TX
//     dma_channel_config c = dma_channel_get_default_config(g_dma_chan);
    
//     channel_config_set_transfer_data_size(&c, DMA_SIZE_8); 
//     channel_config_set_read_increment(&c, true);           
//     channel_config_set_write_increment(&c, false);          
//     channel_config_set_dreq(&c, DMA_SPI_TX_DREQ); 
    
//     dma_channel_configure(
//         g_dma_chan,
//         &c,
//         SPI_TX_ADDRESS, 
//         NULL,            
//         0,               
//         false            
//     );
    
//     printf("DMA Channel %d initialized for SPI display.\n", g_dma_chan);
// }

// /**
//  * @brief Converts a scalar brightness (0-255) to a 16-bit RGB565 grayscale color.
//  */
// uint16_t get_grayscale_color(uint16_t scalar) {
//     uint8_t clamped_scalar = (uint8_t)(scalar > 255 ? 255 : scalar);
    
//     uint8_t red_blue_5bit = (uint8_t)((clamped_scalar * 31) / 255);
//     uint8_t green_6bit = (uint8_t)((clamped_scalar * 63) / 255);

//     uint16_t color = (uint16_t)(
//         (red_blue_5bit << 11) |   
//         (green_6bit << 5) |       
//         red_blue_5bit             
//     );

//     return color;
// }

// /**
//  * @brief Writes address parameters and pixel data for a single star into a temporary buffer.
//  * @details The caller is responsible for executing the transfers and managing the D/C line.
//  * @return size_t The number of bytes written for this star set (always 26 bytes).
//  */
// size_t buffer_star_data(uint32_t buffer[AMOUNT_OF_STARS], uint16_t magnitude[AMOUNT_OF_STARS], uint16_t color_mode, int index)
// {
//     // Local buffer to hold a single star's data (Address Parameters + Pixel Data)
//     uint8_t static_buffer[BYTES_PER_STAR_PACKET];
//     uint8_t *dma_ptr = static_buffer;
    
//     // Helper function to append 2 bytes of color data (used internally here)
//     uint8_t* local_append_color_bytes(uint8_t *ptr, uint16_t color)
//     {
//         *ptr++ = (uint8_t)(color >> 8);
//         *ptr++ = (uint8_t)color;
//         return ptr;
//     }

//     // Extract coordinates (using buffer[index])
//     int x_coord = (int16_t)(buffer[index] >> 16);
//     int y_coord = (int16_t)buffer[index];
//     int x_center = x_coord + (LCD_WIDTH / 2);
//     int y_center = y_coord + (LCD_HEIGHT / 2);

//     // Set 3x3 window bounds
//     uint16_t x_start = (uint16_t)(x_center - 1);
//     uint16_t x_end = (uint16_t)(x_center + 1);
//     uint16_t y_start = (uint16_t)(y_center - 1);
//     uint16_t y_end = (uint16_t)(y_center + 1);

//     // --- 1. CASET Parameters (4 bytes) ---
//     *dma_ptr++ = (uint8_t)(x_start >> 8); 
//     *dma_ptr++ = (uint8_t)x_start;
//     *dma_ptr++ = (uint8_t)(x_end >> 8); 
//     *dma_ptr++ = (uint8_t)x_end;

//     // --- 2. PASET Parameters (4 bytes) ---
//     *dma_ptr++ = (uint8_t)(y_start >> 8); 
//     *dma_ptr++ = (uint8_t)y_start;
//     *dma_ptr++ = (uint8_t)(y_end >> 8);
//     *dma_ptr++ = (uint8_t)y_end;
    
//     // --- 3. Pixel Color Generation and Append ---

//     if (color_mode == COLOR_BLACK) 
//     {
//         // Erase Mode: Fill all 9 pixels (18 bytes) with black
//         for (int i = 0; i < 9; i++) {
//             dma_ptr = local_append_color_bytes(dma_ptr, COLOR_BLACK);
//         }
//     } 
//     else 
//     {
//         // Draw Mode (Grayscale): Calculate gradient pattern
//         uint16_t scalar_mag = magnitude[index];
//         uint16_t center_color = get_grayscale_color(scalar_mag); 
//         uint16_t cross_color = get_grayscale_color((scalar_mag > 40) ? (scalar_mag - 40) : 0);
//         uint16_t corner_color = get_grayscale_color((scalar_mag > 80) ? (scalar_mag - 80) : 0);

//         // Pattern: [C] [X] [C] / [X] [S] [X] / [C] [X] [C] (Row-Major Order)
        
//         // Row 1 (y_start)
//         dma_ptr = local_append_color_bytes(dma_ptr, corner_color);  
//         dma_ptr = local_append_color_bytes(dma_ptr, cross_color);   
//         dma_ptr = local_append_color_bytes(dma_ptr, corner_color);  

//         // Row 2 (y_center)
//         dma_ptr = local_append_color_bytes(dma_ptr, cross_color);   
//         dma_ptr = local_append_color_bytes(dma_ptr, center_color);  
//         dma_ptr = local_append_color_bytes(dma_ptr, cross_color);   

//         // Row 3 (y_end)
//         dma_ptr = local_append_color_bytes(dma_ptr, corner_color);  
//         dma_ptr = local_append_color_bytes(dma_ptr, cross_color);   
//         dma_ptr = local_append_color_bytes(dma_ptr, corner_color);  
//     }
    
//     // Return the number of bytes written (8 address bytes + 18 pixel data bytes = 26 bytes)
//     return (size_t)(dma_ptr - static_buffer);
// }

// // NOTE: The previous erase_stars and draw_stars packet builders are now replaced 
// // by the new burst logic in rendering.c calling buffer_star_data.

//WITH SPI ONLY

void Clear_The_star(void)
{
    LCD_Clear(0x0000);
}

void erase_stars(StarPosition_t buffer[AMOUNT_OF_STARS], int count)
{
    // Define half-width and half-height for conversion
    const int X_HALF = 160; 
    const int Y_HALF = 240; 

    for (int i = 0; i< count; i++)
    {
        // 1. Extract Center-Based Coordinates (xc, yc)
        // x_c (center-based X): Bits 31 to 16
        int x_c = buffer[i].x_proj; 
        int y_c = buffer[i].z_proj; 

        // 2. Convert to Display Coordinates (xd, yd)
        int x_d = x_c + X_HALF; 
        int y_d = Y_HALF - y_c; // Flips the Y-axis and shifts the origin to top-left

        // 3. Erase a 3x3 square of pixels around the new display coordinates (x_d, y_d)
        // Note: Coordinates are typically cast or constrained to display limits (0 to 319/479)
        
        // Row y_d + 1
        // LCD_DrawPoint(x_d - 1, y_d + 1, COLOR_BLACK); 
        // LCD_DrawPoint(x_d,     y_d + 1, COLOR_BLACK); 
        // LCD_DrawPoint(x_d + 1, y_d + 1, COLOR_BLACK);
        
        // // Row y_d 
        // LCD_DrawPoint(x_d - 1, y_d,     COLOR_BLACK); 
        // LCD_DrawPoint(x_d,     y_d,     COLOR_BLACK); 
        // LCD_DrawPoint(x_d + 1, y_d,     COLOR_BLACK);
        
        // // Row y_d - 1
        // LCD_DrawPoint(x_d - 1, y_d - 1, COLOR_BLACK); 
        // LCD_DrawPoint(x_d,     y_d - 1, COLOR_BLACK); 
        // LCD_DrawPoint(x_d + 1, y_d - 1, COLOR_BLACK);
        LCD_DrawFillRectangle(x_d, y_d, x_d + 1, y_d + 1, COLOR_BLACK);
    }
}

void draw_stars(StarPosition_t buffer[AMOUNT_OF_STARS], int count)
{
    // Define half-width and half-height for conversion
    const int X_HALF = 160; 
    const int Y_HALF = 240; 
    
    // Assuming COLOR_WHITE is defined globally (e.g., 0xFFFFFF in 262K mode)

    for (int i = 0; i < count; i++)
    {
        // 1. Extract Center-Based Coordinates (xc, yc)
        // x_c (center-based X): Bits 31 to 16
        int x_c = buffer[i].x_proj; 
        int y_c = buffer[i].z_proj;

        // 2. Convert to Display Coordinates (xd, yd)
        int x_d = x_c + X_HALF; 
        int y_d = Y_HALF - y_c; // Flips Y axis, shifts origin to top-left

        // 3. Draw a 3x3 square of pixels (star) around the new display coordinates (x_d, y_d)
        
        // Row y_d + 1
        // LCD_DrawPoint(x_d - 1, y_d + 1, COLOR_WHITE); 
        // LCD_DrawPoint(x_d,     y_d + 1, COLOR_WHITE); 
        // LCD_DrawPoint(x_d + 1, y_d + 1, COLOR_WHITE);
        
        // // Row y_d 
        // LCD_DrawPoint(x_d - 1, y_d,     COLOR_WHITE); 
        // LCD_DrawPoint(x_d,     y_d,     COLOR_WHITE); 
        // LCD_DrawPoint(x_d + 1, y_d,     COLOR_WHITE);
        
        // // Row y_d - 1
        // LCD_DrawPoint(x_d - 1, y_d - 1, COLOR_WHITE); 
        // LCD_DrawPoint(x_d,     y_d - 1, COLOR_WHITE); 
        // LCD_DrawPoint(x_d + 1, y_d - 1, COLOR_WHITE);
        LCD_DrawFillRectangle(x_d, y_d, x_d + 1, y_d + 1, COLOR_WHITE);
        //sleep_ms(1);
    }
    //sleep_ms(100);
}
