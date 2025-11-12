#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

// 1. Include your generated PIO header
#include "lcd_parallel.pio.h"

// --- Pin Definitions (CHANGE TO MATCH BOARD WIRING) ---
#define PIN_LCD_DATA_BASE 2  // GP2-GP17 (Data)
#define PIN_LCD_DC 18        // GP18 (D/C)
#define PIN_LCD_WR 19        // GP19 (WR)

// --- Other LCD Pin Definitions (CHANGE TO MATCH BOARD WIRING) ---
#define PIN_LCD_RST 21       // Reset pin
#define PIN_LCD_CS 20        // Chip Select (can tie to GND if always active)
#define PIN_LCD_BL 22        // Backlight control

// --- PIO Definitions ---
#define PIO_DISP pio0
#define SM_DISP 0
#define PIO_CLK_DIV 4.0f

// --- ILI9486 Command Definitions ---
#define CMD_SLEEP_OUT 0x11
#define CMD_DISPLAY_ON 0x29
#define CMD_CASET 0x2A
#define CMD_PASET 0x2B
#define CMD_RAMWR 0x2C
#define CMD_PIXEL_FORMAT 0x3A

// --- Color Definitions ---
#define COLOR_BLACK 0x0000
#define TFT_BLACK       0x0000
#define TFT_NAVY        0x000F
#define TFT_DARKGREEN   0x03E0
#define TFT_DARKCYAN    0x03EF
#define TFT_MAROON      0x7800
#define TFT_PURPLE      0x780F
#define TFT_OLIVE       0x7BE0
#define TFT_LIGHTGREY   0xC618
#define TFT_DARKGREY    0x7BEF
#define TFT_BLUE        0x001F
#define TFT_GREEN       0x07E0
#define TFT_CYAN        0x07FF
#define TFT_RED         0xF800
#define TFT_MAGENTA     0xF81F
#define TFT_YELLOW      0xFFE0
#define TFT_WHITE       0xFFFF
#define TFT_ORANGE      0xFD20
#define TFT_GREENYELLOW 0xAFE5
#define TFT_PINK        0xF81F

/**
 * @brief Helper function to build 32-bit PIO packets.
 * @note This is the corrected version.
 */
static inline uint32_t build_packet(uint16_t payload, uint8_t dc)
{
    // CORRECT MAPPING:
    // [D/C bit at 16] [Payload[15..0] at bits 15-0] [15 unused bits] 
    return (((uint32_t)payload) | (((uint32_t)dc & 1u) << 16));
}

/**
 * @brief Sends the minimum required commands to wake the LCD.
 * @note This uses blocking PIO puts.
 */
void lcd_init_sequence(PIO pio, uint sm) {
    printf("Sending LCD Init Sequence...\n");

    // 1. Sleep Out (0x11h)
    pio_sm_put_blocking(pio, sm, build_packet(CMD_SLEEP_OUT, 0));
    sleep_ms(10); // Must wait > 5ms after Sleep Out
    printf("  - Sleep Out (0x11h) sent.\n");

    // 2. Set Pixel Format (0x3Ah) to 16-bit/pixel (0x55)
    // This is CRITICAL. The default is 18-bit.
    pio_sm_put_blocking(pio, sm, build_packet(CMD_PIXEL_FORMAT, 0));
    pio_sm_put_blocking(pio, sm, build_packet(0x55, 1)); // 0101 0101 (DBI=16bit, DPI=16bit)
    printf("  - Pixel Format (0x3Ah) set to 16-bit (0x55).\n");

    // 3. Display ON (0x29h)
    pio_sm_put_blocking(pio, sm, build_packet(CMD_DISPLAY_ON, 0));
    printf("  - Display ON (0x29h) sent.\n");
    sleep_ms(10);
}

/**
 * @brief Clears the entire 320x480 screen to black.
 * @note This uses blocking PIO puts and is slow.
 */
void lcd_clear_screen_black(PIO pio, uint sm) {
    // 1. Set window to full screen (320x480)
    uint32_t packets[] = {
        build_packet(CMD_CASET, 0), // CASET
        build_packet(0x00, 1),      // X-start MSB
        build_packet(0x00, 1),      // X-start LSB
        build_packet(0x01, 1),      // X-end MSB   (319)
        build_packet(0x3F, 1),      // X-end LSB
        build_packet(CMD_PASET, 0), // PASET
        build_packet(0x00, 1),      // Y-start MSB
        build_packet(0x00, 1),      // Y-start LSB
        build_packet(0x01, 1),      // Y-end MSB   (479)
        build_packet(0xDF, 1),      // Y-end LSB
        build_packet(CMD_RAMWR, 0)  // Memory Write
    };

    // Send the window-set commands
    for (int i = 0; i < 11; i++) {
        /*while (pio_sm_is_tx_fifo_full(pio, sm))
        {
            //sleep_us(1);
            tight_loop_contents();
        }
        *(io_wo_32 *)&pio->txf[sm] = packets[i];*/
        pio_sm_put_blocking(PIO_DISP, SM_DISP, packets[i]);
    }

    // 2. Send 153,600 black pixels (320 * 480 = 153600)
    printf("  - Streaming %d black pixels...\n", (320 * 480));
    uint32_t black_pixel = build_packet(COLOR_BLACK, 1);
    for (int i = 0; i < (320 * 480); i++) {
        /*while (pio_sm_is_tx_fifo_full(pio, sm))
        {
            tight_loop_contents();
        }
        *(io_wo_32 *)&pio->txf[sm] = black_pixel;*/
        pio_sm_put_blocking(PIO_DISP, SM_DISP, black_pixel);
    }
}


int main() {
    // Standard Pico init
    stdio_init_all();
    sleep_ms(2000); // Wait for a serial monitor to connect
    printf("--- LCD Black Screen Test ---\n");

    // --- Other LCD Pin Init ---
    gpio_init(PIN_LCD_CS);
    gpio_set_dir(PIN_LCD_CS, GPIO_OUT);
    gpio_put(PIN_LCD_CS, 0); // Active Low: Select the chip

    gpio_init(PIN_LCD_BL);
    gpio_set_dir(PIN_LCD_BL, GPIO_OUT);
    gpio_put(PIN_LCD_BL, 1); // Turn on backlight

    // --- Hardware Reset for LCD ---
    gpio_init(PIN_LCD_RST);
    gpio_set_dir(PIN_LCD_RST, GPIO_OUT);
    gpio_put(PIN_LCD_RST, 0); // Hold reset low
    sleep_ms(20);
    gpio_put(PIN_LCD_RST, 1); // Release reset
    sleep_ms(120);            // Wait for the display to be ready
    printf("LCD Reset complete.\n");

    
    // --- PIO Init ---
    uint offset = pio_add_program(PIO_DISP, &lcd_parallel_program);
    float clkdiv = 2.0f;
    
    lcd_parallel_program_init(
        PIO_DISP,
        SM_DISP,
        offset,
        PIN_LCD_DATA_BASE,  // This is 2
        PIN_LCD_WR,         // This is 19
        clkdiv
    );
    printf("PIO Initialized.\n");
    printf("  Data+D/C Pins: %d to %d\n", PIN_LCD_DATA_BASE, PIN_LCD_DATA_BASE + 16);
    printf("  WR Pin: %d\n", PIN_LCD_WR);

    // --- Send the init sequence to wake the display up
    lcd_init_sequence(PIO_DISP, SM_DISP);

    // --- Clear the screen to black ---
    printf("Clearing screen to black...\n");
    lcd_clear_screen_black(PIO_DISP, SM_DISP);
    printf("Screen should be black now.\n");

    // --- Done ---
    // The program will just sit here. The screen will stay black.
    while (true) {
        tight_loop_contents();
    }
}

// #include "utils.h"
// #include "bno08x.h"
// #include "sh2.h"
// #include "sh2_err.h"
// #include <stdio.h>
// #include "pico/stdlib.h"
// #include "hardware/pio.h"
// #include "hardware/gpio.h"
// #include "hardware/dma.h"
// #include "hardware/clocks.h"

// //#define IMU_TEST
// #define LCD_TEST

// #ifdef LCD_TEST

// // Includes your driver selection (ST7796_DRIVER)
// #include "config.h" 
// #include "lcd/pio_16bit_parallel.pio.h"

// // This will now include "ST7796_Defines.h" as selected in config.h
// #if defined(ILI9486_DRIVER)
//   #include "ILI9486_Defines.h"
// #elif defined(ST7796_DRIVER)
//   #include "ST7796_Defines.h"
// #endif

// // (ST7796_Defines.h lacks these, but ILI9486_Defines.h has them)
// #define TFT_BLACK       0x0000
// #define TFT_NAVY        0x000F
// #define TFT_DARKGREEN   0x03E0
// #define TFT_DARKCYAN    0x03EF
// #define TFT_MAROON      0x7800
// #define TFT_PURPLE      0x780F
// #define TFT_OLIVE       0x7BE0
// #define TFT_LIGHTGREY   0xC618
// #define TFT_DARKGREY    0x7BEF
// #define TFT_BLUE        0x001F
// #define TFT_GREEN       0x07E0
// #define TFT_CYAN        0x07FF
// #define TFT_RED         0xF800
// #define TFT_MAGENTA     0xF81F
// #define TFT_YELLOW      0xFFE0
// #define TFT_WHITE       0xFFFF
// #define TFT_ORANGE      0xFD20
// #define TFT_GREENYELLOW 0xAFE5
// #define TFT_PINK        0xF81F


// // Global PIO variables
PIO lcd_pio = pio0;
uint pio_sm = 0;
uint pio_offset = 0;
uint32_t pio_instr_jmp8;
uint32_t pio_instr_fill;
uint32_t pio_instr_addr;
uint32_t pio_instr_set_dc;
uint32_t pio_instr_clr_dc;
uint32_t pull_stall_mask;

#define WAIT_FOR_STALL  lcd_pio->fdebug = pull_stall_mask; while (!(lcd_pio->fdebug & pull_stall_mask))
#define TX_FIFO  lcd_pio->txf[pio_sm]


// /**
//  * @brief Port of the TFT_eSPI pioinit() function to native picosdk
//  * This logic is taken directly from TFT_eSPI_RP2040.c
//  */
void lcd_pio_init() {
    // 1. Find a free PIO instance
    if (!pio_can_add_program(lcd_pio, &lcd_parallel_program)) {
        lcd_pio = pio1;
        if (!pio_can_add_program(lcd_pio, &lcd_parallel_program)) {
            printf("Error: No room for PIO program\n");
            return;
        }
    }

    // 2. Claim a state machine
    pio_sm = pio_claim_unused_sm(lcd_pio, true);
    pio_offset = pio_add_program(lcd_pio, &lcd_parallel_program);

    // 3. Configure GPIOs for PIO
    pio_gpio_init(lcd_pio, PIN_LCD_DC);
    pio_gpio_init(lcd_pio, PIN_LCD_WR);
    for (int i = 0; i < 16; i++) {
        pio_gpio_init(lcd_pio, PIN_LCD_DATA_BASE + i);
    }

    // 4. Set pin directions
    pio_sm_set_consecutive_pindirs(lcd_pio, pio_sm, PIN_LCD_DC, 1, true);
    pio_sm_set_consecutive_pindirs(lcd_pio, pio_sm, PIN_LCD_WR, 1, true);
    pio_sm_set_consecutive_pindirs(lcd_pio, pio_sm, PIN_LCD_DATA_BASE, 16, true);

    // 5. Get default PIO config and modify it
    pio_sm_config c = lcd_parallel_program_get_default_config(pio_offset);

    sm_config_set_set_pins(&c, PIN_LCD_DC, 1);
    sm_config_set_sideset_pins(&c, PIN_LCD_WR);
    sm_config_set_out_pins(&c, PIN_LCD_DATA_BASE, 16);

    // Set clock divider
    sm_config_set_clkdiv_int_frac(&c, PIO_CLK_DIV, 0);

    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_out_shift(&c, false, false, 0);

    // 6. Load the config
    pio_sm_init(lcd_pio, pio_sm, pio_offset + tft_io_offset_start_tx, &c);

    // 7. Start the state machine
    pio_sm_set_enabled(lcd_pio, pio_sm, true);

    // 8. Pre-calculate instruction variants for speed
    pull_stall_mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + pio_sm);
    pio_instr_jmp8  = pio_encode_jmp(pio_offset + tft_io_offset_start_8);
    pio_instr_fill  = pio_encode_jmp(pio_offset + tft_io_offset_block_fill);
    pio_instr_addr  = pio_encode_jmp(pio_offset + tft_io_offset_set_addr_window);
    pio_instr_set_dc = pio_encode_set((pio_src_dest)0, 1);
    pio_instr_clr_dc = pio_encode_set((pio_src_dest)0, 0);
}

// /**
//  * @brief Send a 16-bit word to the PIO TX FIFO
//  */
// inline void pio_write16(uint16_t data) {
//     WAIT_FOR_STALL;
//     TX_FIFO = data;
// }

// /**
//  * @brief Send an 8-bit command to the LCD
//  */
// void writecommand(uint8_t cmd) {
//     WAIT_FOR_STALL;
//     lcd_pio->sm[pio_sm].instr = pio_instr_clr_dc; // Set DC low
//     lcd_pio->sm[pio_sm].instr = pio_instr_jmp8;   // Jump to 8-bit send
//     TX_FIFO = cmd;
//     WAIT_FOR_STALL;
//     lcd_pio->sm[pio_sm].instr = pio_instr_set_dc; // Set DC high
// }

// /**
//  * @brief Send 8-bit data to the LCD
//  */
// void writedata(uint8_t data) {
//     // DC pin is already high
//     lcd_pio->sm[pio_sm].instr = pio_instr_jmp8; // Jump to 8-bit send
//     TX_FIFO = data;
// }

// /**
//  * @brief Send 16-bit data to the LCD
//  */
// void writedata16(uint16_t data) {
//     // DC pin is already high
//     // The PIO program defaults to 16-bit, so no jmp needed
//     TX_FIFO = data;
// }

// /**
//  * @brief Set the address window for pixel writes
//  */
// void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
//     WAIT_FOR_STALL;
//     lcd_pio->sm[pio_sm].instr = pio_instr_addr;
//     TX_FIFO = TFT_CASET;
//     TX_FIFO = (x0 << 16) | x1;
//     TX_FIFO = TFT_PASET;
//     TX_FIFO = (y0 << 16) | y1;
//     TX_FIFO = TFT_RAMWR;
// }

// /**
//  * @brief Fill a block of pixels with a single color
//  */
// void pushBlock(uint16_t color, uint32_t len) {
//     if (!len) return;
//     WAIT_FOR_STALL;
//     lcd_pio->sm[pio_sm].instr = pio_instr_fill;
//     TX_FIFO = color;
//     TX_FIFO = --len; // PIO sends n+1 pixels
// }

// /**
//  * @brief Fill the entire screen with a color
//  */
// void fillScreen(uint16_t color) {
//     // Use the driver-specific dimensions from the header
//     setWindow(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
//     pushBlock(color, (uint32_t)TFT_WIDTH * TFT_HEIGHT);
// }

// /**
//  * @brief Initialize the LCD controller
//  */
// void lcd_init() {
//     // Init the PIO
//     lcd_pio_init();

//     // Init Reset Pin
//     if (TFT_RST >= 0) {
//         gpio_init(TFT_RST);
//         gpio_set_dir(TFT_RST, GPIO_OUT);
//         gpio_put(TFT_RST, 1);
//         sleep_ms(5);
//         gpio_put(TFT_RST, 0);
//         sleep_ms(20);
//         gpio_put(TFT_RST, 1);
//         sleep_ms(150);
//     } else {
//         writecommand(TFT_SWRST);
//         sleep_ms(150);
//     }

//     // Init CS Pin
//     if (TFT_CS >= 0) {
//         gpio_init(TFT_CS);
//         gpio_set_dir(TFT_CS, GPIO_OUT);
//         gpio_put(TFT_CS, 0); // CS Active Low
//     }
    
//     // --- Send Initialization Sequence ---
//     // This is a C pre-processor trick. The #include
//     // pastes the contents of the .h file here.
    
//     // Define delay() as sleep_ms() for the init files
//     #define delay(ms) sleep_ms(ms)
    
//     #if defined(ILI9486_DRIVER)
//       #include "ILI9486_Init.h"
//     #elif defined(ST7796_DRIVER)
//       // The ST7796 init file has begin/end_tft_write()
//       // which we don't have. We'll just define them as empty
//       // macros for the include to work.
//       #define begin_tft_write()
//       #define end_tft_write()
//       #include "ST7796_Init.h"
//     #endif

//     // Undefine the macros
//     #undef delay
//     #undef begin_tft_write
//     #undef end_tft_write

//     // Set rotation (1 = Landscape)
//     writecommand(TFT_MADCTL);
//     #if defined(ILI9486_DRIVER)
//       writedata(TFT_MAD_BGR | TFT_MAD_MV); // Landscape for ILI9486
//     #elif defined(ST7796_DRIVER)
//       // From ST7796_Rotation.h, case 1:
//       writedata(TFT_MAD_MV | TFT_MAD_COLOR_ORDER); // Landscape for ST7796
//     #endif
// }


// int main() {
//     stdio_init_all();
//     sleep_ms(2000);
//     printf("Starting native picosdk LCD Test...\n");

//     lcd_init();

//     printf("LCD Init complete. Filling screen.\n");

//     for (;;) {
//         fillScreen(TFT_BLACK);
//         sleep_ms(500);
//         fillScreen(TFT_RED);
//         sleep_ms(500);
//         fillScreen(TFT_GREEN);
//         sleep_ms(500);
//         fillScreen(TFT_BLUE);
//         sleep_ms(500);
//     }

//     printf("Test complete.\n");

//     while (1) {
//         sleep_ms(1000);
//     }

//     return 0;
// }

// #endif
