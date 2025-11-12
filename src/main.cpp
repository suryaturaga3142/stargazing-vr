#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

// Include generated PIO header
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
#define LCD_WIDTH 320
#define LCD_HEIGHT 480

// --- ILI9486 Command Definitions ---
#define CMD_SLEEP_OUT 0x11
#define CMD_DISPLAY_ON 0x29
#define CMD_CASET 0x2A
#define CMD_PASET 0x2B
#define CMD_RAMWR 0x2C
#define CMD_PIXEL_FORMAT 0x3A
#define CMD_SWRST 0x01
#define CMD_MADCTL 0x0B

#define CMD_MAD_MY  0x80
#define CMD_MAD_MX  0x40
#define CMD_MAD_MV  0x20
#define CMD_MAD_ML  0x10
#define CMD_MAD_BGR 0x08
#define CMD_MAD_MH  0x04
#define CMD_MAD_RGB 0x00

// --- Color Definitions ---
#define COLOR_BLACK       0x0000
#define COLOR_NAVY        0x000F
#define COLOR_DARKGREEN   0x03E0
#define COLOR_DARKCYAN    0x03EF
#define COLOR_MAROON      0x7800
#define COLOR_PURPLE      0x780F
#define COLOR_OLIVE       0x7BE0
#define COLOR_LIGHTGREY   0xC618
#define COLOR_DARKGREY    0x7BEF
#define COLOR_BLUE        0x001F
#define COLOR_GREEN       0x07E0
#define COLOR_CYAN        0x07FF
#define COLOR_RED         0xF800
#define COLOR_MAGENTA     0xF81F
#define COLOR_YELLOW      0xFFE0
#define COLOR_WHITE       0xFFFF
#define COLOR_ORANGE      0xFD20
#define COLOR_GREENYELLOW 0xAFE5
#define COLOR_PINK        0xF81F

// Global PIO variables
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

//#define ILI9486_DRIVER
#define ST7796_DRIVER

// /**
//  * @brief Port of the TFT_eSPI pioinit() function to native picosdk
//  * This logic is taken directly from TFT_eSPI_RP2040.c
//  */
void lcd_pio_init() {
    // Find a free PIO instance
    if (!pio_can_add_program(lcd_pio, &lcd_parallel_program)) {
        lcd_pio = pio1;
        if (!pio_can_add_program(lcd_pio, &lcd_parallel_program)) {
            printf("Error: No room for PIO program\n");
            return;
        }
    }

    // Claim a state machine
    pio_sm = pio_claim_unused_sm(lcd_pio, true);
    pio_offset = pio_add_program(lcd_pio, &lcd_parallel_program);

    // Configure GPIOs for PIO
    pio_gpio_init(lcd_pio, PIN_LCD_DC);
    pio_gpio_init(lcd_pio, PIN_LCD_WR);
    for (int i = 0; i < 16; i++) {
        pio_gpio_init(lcd_pio, PIN_LCD_DATA_BASE + i);
    }

    // Set pin directions
    pio_sm_set_consecutive_pindirs(lcd_pio, pio_sm, PIN_LCD_DC, 1, true);
    pio_sm_set_consecutive_pindirs(lcd_pio, pio_sm, PIN_LCD_WR, 1, true);
    pio_sm_set_consecutive_pindirs(lcd_pio, pio_sm, PIN_LCD_DATA_BASE, 16, true);

    // Get default PIO config and modify it
    pio_sm_config c = lcd_parallel_program_get_default_config(pio_offset);

    sm_config_set_set_pins(&c, PIN_LCD_DC, 1);
    sm_config_set_sideset_pins(&c, PIN_LCD_WR);
    sm_config_set_out_pins(&c, PIN_LCD_DATA_BASE, 16);

    // Set clock divider
    sm_config_set_clkdiv_int_frac(&c, PIO_CLK_DIV, 0);

    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_out_shift(&c, false, false, 0);

    // Load the config
    pio_sm_init(lcd_pio, pio_sm, pio_offset + lcd_parallel_offset_start_tx, &c);

    // Start the state machine
    pio_sm_set_enabled(lcd_pio, pio_sm, true);

    // Pre-calculate instruction variants for speed
    pull_stall_mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + pio_sm);
    pio_instr_jmp8  = pio_encode_jmp(pio_offset + lcd_parallel_offset_start_8);
    pio_instr_fill  = pio_encode_jmp(pio_offset + lcd_parallel_offset_block_fill);
    pio_instr_addr  = pio_encode_jmp(pio_offset + lcd_parallel_offset_set_addr_window);
    pio_instr_set_dc = pio_encode_set((pio_src_dest)0, 1); //Sets D/C to be 1
    pio_instr_clr_dc = pio_encode_set((pio_src_dest)0, 0); //Sets D/C to be 0
}

/**
 * @brief Send a 16-bit word to the PIO TX FIFO
 */
inline void pio_write16(uint16_t data) {
    WAIT_FOR_STALL;
    TX_FIFO = data;
}

/**
 * @brief Send an 8-bit command to the LCD
 */
void writecommand(uint8_t cmd) {
    WAIT_FOR_STALL;
    lcd_pio->sm[pio_sm].instr = pio_instr_clr_dc; // Set DC low
    lcd_pio->sm[pio_sm].instr = pio_instr_jmp8;   // Jump to 8-bit send
    TX_FIFO = cmd;
    WAIT_FOR_STALL;
    lcd_pio->sm[pio_sm].instr = pio_instr_set_dc; // Set DC high
}

/**
 * @brief Send 8-bit data to the LCD
 */
void writedata(uint8_t data) {
    // DC pin is already high
    lcd_pio->sm[pio_sm].instr = pio_instr_jmp8; // Jump to 8-bit send
    TX_FIFO = data;
}

/**
 * @brief Send 16-bit data to the LCD
 */
void writedata16(uint16_t data) {
    // DC pin is already high
    // The PIO program defaults to 16-bit, so no jmp needed
    TX_FIFO = data;
}

/**
 * @brief Set the address window for pixel writes
 */
void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    WAIT_FOR_STALL;
    lcd_pio->sm[pio_sm].instr = pio_instr_addr;
    TX_FIFO = CMD_CASET;
    TX_FIFO = (x0 << 16) | x1;
    TX_FIFO = CMD_PASET;
    TX_FIFO = (y0 << 16) | y1;
    TX_FIFO = CMD_RAMWR;
}

/**
 * @brief Fill a block of pixels with a single color
 */
void pushBlock(uint16_t color, uint32_t len) {
    if (!len) return;
    WAIT_FOR_STALL;
    lcd_pio->sm[pio_sm].instr = pio_instr_fill;
    TX_FIFO = color;
    TX_FIFO = --len; // PIO sends n+1 pixels
}

/**
 * @brief Fill the entire screen with a color
 */
void fillScreen(uint16_t color) {
    // Use the driver-specific dimensions from the header
    setWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    pushBlock(color, (uint32_t)LCD_WIDTH * LCD_HEIGHT);
}

/**
 * @brief Send initialization commands to set up the ST7796 driver
 */
void lcd_driver_init()
{
    sleep_ms(120);

    writecommand(0x01); //Software reset
    sleep_ms(120);

    writecommand(0x11); //Sleep exit                                            
    sleep_ms(120);

    writecommand(0xF0); //Command Set control                                 
    writedata(0xC3);    //Enable extension command 2 partI

    writecommand(0xF0); //Command Set control                                 
    writedata(0x96);    //Enable extension command 2 partII

    writecommand(0x36); //Memory Data Access Control MX, MY, RGB mode                                    
    writedata(0x48);    //X-Mirror, Top-Left to right-Buttom, RGB  

    writecommand(0x3A); //Interface Pixel Format                                    
    writedata(0x55);    //Control interface color format set to 16


    writecommand(0xB4); //Column inversion 
    writedata(0x01);    //1-dot inversion

    writecommand(0xB6); //Display Function Control
    writedata(0x80);    //Bypass
    writedata(0x02);    //Source Output Scan from S1 to S960, Gate Output scan from G1 to G480, scan cycle=2
    writedata(0x3B);    //LCD Drive Line=8*(59+1)


    writecommand(0xE8); //Display Output Ctrl Adjust
    writedata(0x40);
    writedata(0x8A);	
    writedata(0x00);
    writedata(0x00);
    writedata(0x29);    //Source eqaulizing period time= 22.5 us
    writedata(0x19);    //Timing for "Gate start"=25 (Tclk)
    writedata(0xA5);    //Timing for "Gate End"=37 (Tclk), Gate driver EQ function ON
    writedata(0x33);

    writecommand(0xC1); //Power control2                          
    writedata(0x06);    //VAP(GVDD)=3.85+( vcom+vcom offset), VAN(GVCL)=-3.85+( vcom+vcom offset)
        
    writecommand(0xC2); //Power control 3                                      
    writedata(0xA7);    //Source driving current level=low, Gamma driving current level=High
        
    writecommand(0xC5); //VCOM Control
    writedata(0x18);    //VCOM=0.9

    sleep_ms(120);

    //ST7796 Gamma Sequence
    writecommand(0xE0); //Gamma"+"                                             
    writedata(0xF0);
    writedata(0x09); 
    writedata(0x0b);
    writedata(0x06); 
    writedata(0x04);
    writedata(0x15); 
    writedata(0x2F);
    writedata(0x54); 
    writedata(0x42);
    writedata(0x3C); 
    writedata(0x17);
    writedata(0x14);
    writedata(0x18); 
    writedata(0x1B); 
        
    writecommand(0xE1); //Gamma"-"                                             
    writedata(0xE0);
    writedata(0x09); 
    writedata(0x0B);
    writedata(0x06); 
    writedata(0x04);
    writedata(0x03); 
    writedata(0x2B);
    writedata(0x43); 
    writedata(0x42);
    writedata(0x3B); 
    writedata(0x16);
    writedata(0x14);
    writedata(0x17); 
    writedata(0x1B);

    sleep_ms(120);

    writecommand(0xF0); //Command Set control                                 
    writedata(0x3C);    //Disable extension command 2 partI

    writecommand(0xF0); //Command Set control                                 
    writedata(0x69);    //Disable extension command 2 partII

    sleep_ms(120);

    writecommand(0x29); //Display on 
}

/**
 * @brief Initialize the LCD controller
 */
void lcd_init() {
    // Init the PIO
    lcd_pio_init();

    // Init Reset Pin w/ manual reset or SW reset
    if (PIN_LCD_RST >= 0) {
        gpio_init(PIN_LCD_RST);
        gpio_set_dir(PIN_LCD_RST, GPIO_OUT);
        gpio_put(PIN_LCD_RST, 1);
        sleep_ms(5);
        gpio_put(PIN_LCD_RST, 0);
        sleep_ms(20);
        gpio_put(PIN_LCD_RST, 1);
        sleep_ms(150);
    } else {
        writecommand(CMD_SWRST);
        sleep_ms(150);
    }

    // Init CS Pin
    if (PIN_LCD_CS >= 0) {
        gpio_init(PIN_LCD_CS);
        gpio_set_dir(PIN_LCD_CS, GPIO_OUT);
        gpio_put(PIN_LCD_CS, 0); // CS Active Low
    }
    
    // Send Initialization Sequence
    lcd_driver_init();

    // Set rotation (1 = Landscape)
    writecommand(CMD_MADCTL);
    writedata(CMD_MAD_MV | CMD_MAD_RGB); // Landscape for ST7796
}


int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("Starting native picosdk LCD Test...\n");

    lcd_init();

    printf("LCD Init complete. Filling screen.\n");

    for (;;) {
        fillScreen(COLOR_BLACK);
        sleep_ms(500);
        fillScreen(COLOR_RED);
        sleep_ms(500);
        fillScreen(COLOR_GREEN);
        sleep_ms(500);
        fillScreen(COLOR_BLUE);
        sleep_ms(500);
    }

    printf("Test complete.\n");

    while (1) {
        sleep_ms(1000);
    }

    return 0;
}


// OLD VERSION OF TEST FUNCTION
// /**
//  * @brief Helper function to build 32-bit PIO packets.
//  * @note This is the corrected version.
//  */
// static inline uint32_t build_packet(uint16_t payload, uint8_t dc)
// {
//     // CORRECT MAPPING:
//     // [D/C bit at 16] [Payload[15..0] at bits 15-0] [15 unused bits] 
//     return (((uint32_t)payload) | (((uint32_t)dc & 1u) << 16));
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
//         /*while (pio_sm_is_tx_fifo_full(pio, sm))
//         {
//             //sleep_us(1);
//             tight_loop_contents();
//         }
//         *(io_wo_32 *)&pio->txf[sm] = packets[i];*/
//         pio_sm_put_blocking(PIO_DISP, SM_DISP, packets[i]);
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
