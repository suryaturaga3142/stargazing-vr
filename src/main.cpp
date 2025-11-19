#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

// Include generated PIO header
#include "lcd_parallel.pio.h"

// --- Pin Definitions (CHANGE TO MATCH BOARD WIRING) ---
#define PIN_LCD_DATA_BASE 2  // GP2-GP17 (Data)
#define PIN_LCD_DC 18        // GP18 (D/C) //Needs to be consecutive with Data pins for DMA-Compatible PIO
#define PIN_LCD_WR 19        // GP19 (WR)

// --- Other LCD Pin Definitions (CHANGE TO MATCH BOARD WIRING) ---
#define PIN_LCD_RST 21       // Reset pin
#define PIN_LCD_CS 20        // Chip Select (can tie to GND if always active)
#define PIN_LCD_BL 22        // Backlight control

// --- PIO Definitions ---
//#define PIO_DISP pio0
//#define SM_DISP 0
#define PIO_CLK_DIV 20.0f
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
//uint32_t pio_instr_jmp8;
//uint32_t pio_instr_fill;
//uint32_t pio_instr_addr;
//uint32_t pio_instr_set_dc;
//uint32_t pio_instr_clr_dc;
uint32_t pull_stall_mask;

#define WAIT_FOR_STALL  lcd_pio->fdebug = pull_stall_mask; while (!(lcd_pio->fdebug & pull_stall_mask))
#define TX_FIFO  lcd_pio->txf[pio_sm]

#define DMA_COMPATIBLE_PIO_TEST
// #define DMA_TEST
//#define TEST_STARS

#ifdef DMA_COMPATIBLE_PIO_TEST
/************************************************************* */
// DMA Compatible Version of Test Function                     */
/************************************************************* */
/**
 * @brief Port of the TFT_eSPI pioinit() function to native picosdk
 * This logic is taken directly from TFT_eSPI_RP2040.c
*/
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
    //pio_sm_set_consecutive_pindirs(lcd_pio, pio_sm, PIN_LCD_DC, 1, true);
    pio_sm_set_consecutive_pindirs(lcd_pio, pio_sm, PIN_LCD_WR, 1, true);
    pio_sm_set_consecutive_pindirs(lcd_pio, pio_sm, PIN_LCD_DATA_BASE, 17, true);

    // Get default PIO config and modify it
    pio_sm_config c = lcd_parallel_program_get_default_config(pio_offset);

    sm_config_set_set_pins(&c, PIN_LCD_WR, 1);
    //sm_config_set_sideset_pins(&c, PIN_LCD_WR);
    sm_config_set_out_pins(&c, PIN_LCD_DATA_BASE, 17);

    // Set clock divider
    sm_config_set_clkdiv_int_frac(&c, PIO_CLK_DIV, 0);

    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_out_shift(&c, false, false, 0);

    // Load the config
    pio_sm_init(lcd_pio, pio_sm, pio_offset, &c);

    // Start the state machine
    pio_sm_set_enabled(lcd_pio, pio_sm, true);

    // Pre-calculate instruction variants for speed
    pull_stall_mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + pio_sm);
    //pio_instr_jmp8  = pio_encode_jmp(pio_offset + lcd_parallel_offset_start_8);
    //pio_instr_fill  = pio_encode_jmp(pio_offset + lcd_parallel_offset_block_fill);
    //pio_instr_addr  = pio_encode_jmp(pio_offset + lcd_parallel_offset_set_addr_window);
    //pio_instr_set_dc = pio_encode_set((pio_src_dest)0, 1); //Sets D/C to be 1
    //pio_instr_clr_dc = pio_encode_set((pio_src_dest)0, 0); //Sets D/C to be 0
}

/**
 * @brief Send an 8-bit command to the LCD
 */
void writecommand(uint8_t cmd) {
    WAIT_FOR_STALL;
    TX_FIFO = (uint32_t) cmd;
    WAIT_FOR_STALL;
}

/**
 * @brief Send 16-bit data to the LCD
 */
void writedata16(uint16_t data) {
    WAIT_FOR_STALL;
    TX_FIFO = (1u << 16) | data;
}

/**
 * @brief Send 8-bit data to the LCD
 */
void writedata(uint8_t data) {
    writedata16(data);
}

/**
 * @brief Set the address window for pixel writes
 */
void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    WAIT_FOR_STALL;
    writecommand(CMD_CASET);
    writedata16(x0);
    writedata16(x1);
    writecommand(CMD_PASET);
    writedata16(y0);
    writedata16(y1);
    writecommand(CMD_RAMWR);
}

/**
 * @brief Fill a block of pixels with a single color
 */
void pushBlock(uint16_t color, uint32_t len) {
    if (!len) return;
    for (uint32_t i = 0; i < len; i++)
    {
        WAIT_FOR_STALL;
        writedata16(color);
    }
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

#ifndef DMA_TEST

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
        fillScreen(COLOR_WHITE);
        sleep_ms(500);
    }

    printf("Test complete.\n");

    while (1) {
        sleep_ms(1000);
    }

    return 0;
}

#endif

#ifdef DMA_TEST
#include "display.h"

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("Starting native picosdk LCD Test...\n");

    lcd_init();

    printf("LCD Init complete.");

    display_dma_init(lcd_pio, pio_sm);

    printf("DMA Init complete. Filling screen.\n");
    
    //When pulling these two functions, they work just fine
    // color_entire_screen(COLOR_WHITE); 
    // display_dma_start_transfer();
    // sleep_ms(2000);
    // color_entire_screen(COLOR_BLACK);
    // display_dma_start_transfer();

    for (;;) {
        color_entire_screen(COLOR_BLACK); //get through the building packet
        display_dma_start_transfer();
        sleep_ms(2000);
        // color_entire_screen(COLOR_RED);
        // display_dma_start_transfer();
        // sleep_ms(500);
        // color_entire_screen(COLOR_GREEN);
        // display_dma_start_transfer();
        // sleep_ms(500);
        // color_entire_screen(COLOR_BLUE);
        // display_dma_start_transfer();
        // sleep_ms(500);
        while(!dma_is_complete()){
           
        }
        color_entire_screen(COLOR_WHITE);
        display_dma_start_transfer();
        sleep_ms(2000);
    }
    
    return 0;
}

#endif

#ifdef TEST_STARS
#include "display.h"
    int main()
    {
        stdio_init_all();
        sleep_ms(2000); 
        lcd_init();
        printf("LCD has finished Intializing");
        display_dma_init(lcd_pio, pio_sm);
        display_init_star_cache();
        //initialize matrix
        int matrix[AMOUNT_OF_STARS][2];
        //putting stars in matrix

        matrix[0][0] = 100;
        matrix[0][1] = 50;

        matrix[1][0] = 240;
        matrix[1][1] = 160;

        matrix[2][0] = 400;
        matrix[2][1] = 300;

        matrix[3][0] = 10;
        matrix[3][1] = 10;

        color_entire_screen(COLOR_BLACK);

        display_dma_start_transfer();

        sleep_ms(100);
        place_new_stars(matrix);
        display_dma_start_transfer();

        while(1){
            tight_loop_contents();
        }
    }

#endif
#endif