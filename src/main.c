// #include <stdio.h>
// #include "pico/stdlib.h"
// #include "hardware/pio.h"

// // 1. Include your generated PIO header
// #include "lcd_parallel.pio.h"

// // --- Pin Definitions (Using your specified layout) ---
// #define PIN_LCD_DATA_BASE 2  // GP2 (D/C), GP3-GP18 (Data)
// #define PIN_LCD_WR 19        // GP19 (WR)

// // --- Other LCD Pin Definitions (CHANGE THESE TO MATCH YOUR BOARD) ---
// #define PIN_LCD_RST 20       // Reset pin
// #define PIN_LCD_CS 21        // Chip Select (you can tie this to GND if always active)
// #define PIN_LCD_BL 22        // Backlight control (if you have one)

// // --- PIO Definitions ---
// #define PIO_DISP pio0
// #define SM_DISP 0

// // --- ILI9486 Command Definitions ---
// #define CMD_SLEEP_OUT 0x11
// #define CMD_DISPLAY_ON 0x29
// #define CMD_CASET 0x2A
// #define CMD_PASET 0x2B
// #define CMD_RAMWR 0x2C
// #define CMD_PIXEL_FORMAT 0x3A

// // --- Color Definitions ---
// #define COLOR_BLACK 0xF0A6u //0xFFFFu
// #define COLOR_RED 0xF800u

// /**
//  * @brief Helper function to build 32-bit PIO packets.
//  * @note This is the corrected version.
//  */
// static inline uint32_t build_packet(uint16_t payload, uint8_t dc)
// {
//     // CORRECT MAPPING:
//     // [D/C bit at 16] [Payload[15..0] at bits 15-0] [15 unused bits] 
//     return (((uint32_t)payload << 1) | ((uint32_t)dc & 1u));
// }

// /**
//  * @brief Sends the minimum required commands to wake the LCD.
//  * @note This uses blocking PIO puts.
//  */
// void lcd_init_sequence(PIO pio, uint sm) {
//     printf("Sending LCD Init Sequence...\n");

//     // 1. Sleep Out (0x11h)
//     pio_sm_put_blocking(pio, sm, build_packet(CMD_SLEEP_OUT, 0));
//     sleep_ms(10); // Must wait > 5ms after Sleep Out
//     printf("  - Sleep Out (0x11h) sent.\n");

//     // 2. Set Pixel Format (0x3Ah) to 16-bit/pixel (0x55)
//     // This is CRITICAL. The default is 18-bit.
//     pio_sm_put_blocking(pio, sm, build_packet(CMD_PIXEL_FORMAT, 0));
//     pio_sm_put_blocking(pio, sm, build_packet(0x55, 1)); // 0101 0101 (DBI=16bit, DPI=16bit)
//     printf("  - Pixel Format (0x3Ah) set to 16-bit (0x55).\n");

//     // 3. Display ON (0x29h)
//     pio_sm_put_blocking(pio, sm, build_packet(CMD_DISPLAY_ON, 0));
//     printf("  - Display ON (0x29h) sent.\n");
//     sleep_ms(10);
// }

// /**
//  * @brief Clears the entire 320x480 screen to black.
//  * @note This uses blocking PIO puts and is slow.
//  */
// void lcd_clear_screen_black(PIO pio, uint sm) {
//     // 1. Set window to full screen (320x480)
//     uint32_t packets[] = {
//         build_packet(CMD_CASET, 0), // CASET
//         build_packet(0x00, 1),      // X-start MSB
//         build_packet(0x00, 1),      // X-start LSB
//         build_packet(0x01, 1),      // X-end MSB   (319)
//         build_packet(0x3F, 1),      // X-end LSB
//         build_packet(CMD_PASET, 0), // PASET
//         build_packet(0x00, 1),      // Y-start MSB
//         build_packet(0x00, 1),      // Y-start LSB
//         build_packet(0x01, 1),      // Y-end MSB   (479)
//         build_packet(0xDF, 1),      // Y-end LSB
//         build_packet(CMD_RAMWR, 0)  // Memory Write
//     };

//     // Send the window-set commands
//     for (int i = 0; i < 11; i++) {
//         while (pio_sm_is_tx_fifo_full(pio, sm))
//         {
//             sleep_us(1);
//             //tight_loop_contents();
//         }
//         *(io_wo_32 *)&pio->txf[sm] = packets[i];
//         //pio_sm_put_blocking(PIO_DISP, SM_DISP, packets[i]);
//     }

//     // 2. Send 153,600 black pixels (320 * 480 = 153600)
//     printf("  - Streaming %d black pixels...\n", (320 * 480));
//     uint32_t black_pixel = build_packet(COLOR_BLACK, 1);
//     for (int i = 0; i < (320 * 480); i++) {
//         /*while (pio_sm_is_tx_fifo_full(pio, sm))
//         {
//             tight_loop_contents();
//         }
//         *(io_wo_32 *)&pio->txf[sm] = black_pixel;*/
//         pio_sm_put_blocking(PIO_DISP, SM_DISP, black_pixel);
//     }
// }


// int main() {
//     // Standard Pico init
//     stdio_init_all();
//     sleep_ms(2000); // Wait for a serial monitor to connect
//     printf("--- LCD Black Screen Test ---\n");

//     // --- Other LCD Pin Init ---
//     gpio_init(PIN_LCD_CS);
//     gpio_set_dir(PIN_LCD_CS, GPIO_OUT);
//     gpio_put(PIN_LCD_CS, 0); // Active Low: Select the chip

//     gpio_init(PIN_LCD_BL);
//     gpio_set_dir(PIN_LCD_BL, GPIO_OUT);
//     gpio_put(PIN_LCD_BL, 1); // Turn on backlight

//     // --- Hardware Reset for LCD ---
//     gpio_init(PIN_LCD_RST);
//     gpio_set_dir(PIN_LCD_RST, GPIO_OUT);
//     gpio_put(PIN_LCD_RST, 0); // Hold reset low
//     sleep_ms(20);
//     gpio_put(PIN_LCD_RST, 1); // Release reset
//     sleep_ms(120);            // Wait for the display to be ready
//     printf("LCD Reset complete.\n");

    
//     // --- PIO Init ---
//     uint offset = pio_add_program(PIO_DISP, &lcd_parallel_program);
//     float clkdiv = 2.0f;
    
//     lcd_parallel_program_init(
//         PIO_DISP,
//         SM_DISP,
//         offset,
//         PIN_LCD_DATA_BASE,  // This is 2
//         PIN_LCD_WR,         // This is 19
//         clkdiv
//     );
//     printf("PIO Initialized.\n");
//     printf("  Data+D/C Pins: %d to %d\n", PIN_LCD_DATA_BASE, PIN_LCD_DATA_BASE + 16);
//     printf("  WR Pin: %d\n", PIN_LCD_WR);

//     // --- Send the init sequence to wake the display up
//     lcd_init_sequence(PIO_DISP, SM_DISP);

//     // --- Clear the screen to black ---
//     printf("Clearing screen to black...\n");
//     lcd_clear_screen_black(PIO_DISP, SM_DISP);
//     printf("Screen should be black now.\n");

//     // --- Done ---
//     // The program will just sit here. The screen will stay black.
//     while (true) {
//         tight_loop_contents();
//     }
// }

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h" // We'll leave this include for later
#include "lcd_parallel.pio.h"

// --- Pin Definitions ---
#define PIN_LCD_DATA_BASE 2 // GP2 (D/C), GP3-GP18 (Data)
#define PIN_LCD_WR 19       // GP19 (WR)
#define PIN_LCD_RST 20      // Reset pin
#define PIN_LCD_CS 21       // Chip Select (you can tie this to GND if always active)
#define PIN_LCD_BL 22       // Backlight control

// --- PIO Definitions ---
#define PIO_DISP pio0
#define SM_DISP 0

// --- ILI9486 Command Definitions ---
#define CMD_SLEEP_OUT 0x11
#define CMD_DISPLAY_ON 0x29
#define CMD_CASET 0x2A
#define CMD_PASET 0x2B
#define CMD_RAMWR 0x2C
#define CMD_PIXEL_FORMAT 0x3A

// --- Color Definitions ---
#define COLOR_BLACK 0x0000u
#define COLOR_RED   0xF800u
#define COLOR_GREEN 0x07E0u
#define COLOR_BLUE  0x001Fu

/**
 * @brief Helper function to build 32-bit PIO packets for our pin map.
 * DC bit goes to pin_base+0 (LSB, Bit 0)
 * D0-D15 go to pin_base+1 to pin_base+16
 */
static inline uint32_t build_packet(uint16_t payload, uint8_t dc)
{
    // [D15..D0 at Bit 16..1] [DC at Bit 0]
    return (((uint32_t)payload << 1) | ((uint32_t)dc & 1u));
}

/**
 * @brief Sends the minimum required commands to wake the LCD.
 */
void lcd_init_sequence(PIO pio, uint sm) {
    printf("Sending LCD Init Sequence...\n");

    // 1. Sleep Out (0x11h)
    pio_sm_put_blocking(pio, sm, build_packet(CMD_SLEEP_OUT, 0));
    sleep_ms(10); // Must wait > 5ms after Sleep Out
    printf("  - Sleep Out (0x11h) sent.\n");

    // 2. Set Pixel Format (0x3Ah) to 16-bit/pixel (0x55)
    pio_sm_put_blocking(pio, sm, build_packet(CMD_PIXEL_FORMAT, 0));
    pio_sm_put_blocking(pio, sm, build_packet(0x55, 1)); // 0101 0101 (DBI=16bit, DPI=16bit)
    printf("  - Pixel Format (0x3Ah) set to 16-bit (0x55).\n");

    // 3. Display ON (0x29h)
    pio_sm_put_blocking(pio, sm, build_packet(CMD_DISPLAY_ON, 0));
    printf("  - Display ON (0x29h) sent.\n");
    sleep_ms(10);
}

/**
 * @brief Draws a few pixels. This is a slow test for PIO logic.
 * THIS VERSION IS ACTUALLY SLOW AND WONT DEADLOCK.
 */
void lcd_test_pixels(PIO pio, uint sm) {
    // 1. Set window to just the first 20 pixels
    uint32_t packets[] = {
        build_packet(CMD_CASET, 0), // CASET
        build_packet(0x00, 1),      // X-start MSB
        build_packet(0x00, 1),      // X-start LSB
        build_packet(0x00, 1),      // X-end MSB   (19)
        build_packet(0x13, 1),      // X-end LSB
        build_packet(CMD_PASET, 0), // PASET
        build_packet(0x00, 1),      // Y-start MSB
        build_packet(0x00, 1),      // Y-start LSB
        build_packet(0x00, 1),      // Y-end MSB   (0)
        build_packet(0x00, 1),      // Y-end LSB
        build_packet(CMD_RAMWR, 0)  // Memory Write
    };

    printf("  - Sending window set commands (politely)...\n");
    for (int i = 0; i < 11; i++) {
        // This is the fix. We wait politely.
        while (pio_sm_is_tx_fifo_full(pio, sm)) {
            sleep_us(1); 
        }
        *(io_wo_32 *)&pio->txf[sm] = packets[i];

        sleep_ms(10);
    }

    // 2. Send 20 pixels, one by one, very slowly
    printf("  - Sending 20 test pixels...\n");
    uint32_t red_pixel = build_packet(COLOR_RED, 1);
    
    for (int i = 0; i < 20; i++) {
        printf("    Sending pixel %d\n", i);
        
        // This is also polite.
        while (pio_sm_is_tx_fifo_full(pio, sm)) {
            sleep_us(1);
        }
        *(io_wo_32 *)&pio->txf[sm] = red_pixel;
        
        // This delay is extra-safe for debugging
        sleep_ms(10); 
    }
}


int main() {
    stdio_init_all();
    sleep_ms(3000); // Wait for a serial monitor
    printf("--- PIO Logic Test ---\n");

    // --- Other LCD Pin Init ---
    gpio_init(PIN_LCD_CS);
    gpio_set_dir(PIN_LCD_CS, GPIO_OUT);
    gpio_put(PIN_LCD_CS, 0); 

    gpio_init(PIN_LCD_BL);
    gpio_set_dir(PIN_LCD_BL, GPIO_OUT);
    gpio_put(PIN_LCD_BL, 1);

    // --- Hardware Reset for LCD ---
    gpio_init(PIN_LCD_RST);
    gpio_set_dir(PIN_LCD_RST, GPIO_OUT);
    gpio_put(PIN_LCD_RST, 0);
    sleep_ms(20);
    gpio_put(PIN_LCD_RST, 1);
    sleep_ms(120);
    printf("LCD Reset complete.\n");

    
    // --- PIO Init ---
    uint offset = pio_add_program(PIO_DISP, &lcd_parallel_program);
    
    // ** SET CLKDIV BACK TO SLOW **
    float clkdiv = 150.0f;
    
    lcd_parallel_program_init(
        PIO_DISP, SM_DISP, offset,
        PIN_LCD_DATA_BASE,
        PIN_LCD_WR,
        clkdiv
    );
    printf("PIO Initialized (SLOWLY).\n");

    // --- Send the init sequence
    lcd_init_sequence(PIO_DISP, SM_DISP);

    // --- Send just a few pixels
    printf("Testing PIO by sending a few pixels...\n");
    lcd_test_pixels(PIO_DISP, SM_DISP);
    printf("Test complete.\n");

    // --- Done ---
    while (true) {
        tight_loop_contents();
    }
}