#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

// Include generated PIO header
#include "lcd_parallel.pio.h"

// --- Pin Definitions ---
// DATA: GP2 through GP17 (16 pins)
// DC:   GP18 (1 pin)
// WR:   GP19 (1 pin)
#define PIN_LCD_DATA_BASE 2  
#define PIN_LCD_DC 18        
#define PIN_LCD_WR 19        
#define PIN_LCD_RST 30       
#define PIN_LCD_CS 31        

// --- Configuration ---
#define PIO_CLK_DIV 10.0f   // 125MHz / 10 = 12.5MHz PIO clock (Safe for start)
#define LCD_WIDTH 20//240
#define LCD_HEIGHT 20//320

// --- Colors ---
#define COLOR_BLACK 0x0000
#define COLOR_RED   0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE  0x001F
#define COLOR_WHITE 0xFFFF

// Global PIO variables
PIO lcd_pio = pio0;
uint pio_sm = 0;
uint pio_offset = 0;
uint32_t pull_stall_mask;

#define WAIT_FOR_STALL  lcd_pio->fdebug = pull_stall_mask; while (!(lcd_pio->fdebug & pull_stall_mask))
#define TX_FIFO  lcd_pio->txf[pio_sm]

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

    // --- Configure GPIOs ---
    // Initialize Data Pins (GP2-GP17) + DC (GP18)
    for (int i = 0; i <= 16; i++) {
        pio_gpio_init(lcd_pio, PIN_LCD_DATA_BASE + i);
    }
    // Initialize WR Pin (GP19)
    pio_gpio_init(lcd_pio, PIN_LCD_WR);

    // Set pin directions (All Output)
    // 17 pins for Data+DC
    pio_sm_set_consecutive_pindirs(lcd_pio, pio_sm, PIN_LCD_DATA_BASE, 17, true); 
    // 1 pin for WR
    pio_sm_set_consecutive_pindirs(lcd_pio, pio_sm, PIN_LCD_WR, 1, true);        

    // Get default PIO config
    pio_sm_config c = lcd_parallel_program_get_default_config(pio_offset);

    // --- PIN MAPPING (CRITICAL) ---
    // OUT pins: 17 pins starting at Base. Maps to 'out pins, 17'
    sm_config_set_out_pins(&c, PIN_LCD_DATA_BASE, 17);
    
    // SET pin: 1 pin at WR. Maps to 'set pins, 0/1'
    sm_config_set_set_pins(&c, PIN_LCD_WR, 1); 

    // Disable Side-set (We are using SET instructions now)
    // sm_config_set_sideset_pins(&c, PIN_LCD_WR); // COMMENTED OUT

    // Set clock divider
    sm_config_set_clkdiv(&c, PIO_CLK_DIV);

    // FIFO Config (Join TX for deep buffer)
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    
    // Shift Config: Left shift, Auto-pull OFF (Manual pull in asm), Threshold 32
    sm_config_set_out_shift(&c, true, false, 32);

    // Load the config
    pio_sm_init(lcd_pio, pio_sm, pio_offset, &c);

    // Start the state machine
    pio_sm_set_enabled(lcd_pio, pio_sm, true);

    // Calculate stall mask for sync
    pull_stall_mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + pio_sm);
}

// --- Helper Functions ---

// uint16_t reversebits(uint16_t data)
// {
//     uint16_t new_val = 0;
//     for (int i = 0; i < 16; i++)
//     {
//         new_val |= (data & (1u << (15 - i))) << i;
//     }    
//     return new_val;
// }

// Send Command: DC (Bit 16) = 0
void writecommand(uint8_t cmd) {
    //WAIT_FOR_STALL;
    //cmd = reversebits(cmd);
    //TX_FIFO = (uint32_t)cmd; // Upper bits 0, so Bit 16 is 0 (Command)
    uint bit, pin;
    for (int i = 0; i < 16; i++)
    {
        bit = cmd & (1 << i);
        pin = i + PIN_LCD_DATA_BASE;
        gpio_put(pin, bit);
    }
    gpio_put(PIN_LCD_DC, 0);
    gpio_put(PIN_LCD_WR, 0);
    sleep_us(1000);
    gpio_put(PIN_LCD_WR, 1);
    sleep_us(1000);
}

// Send Data: DC (Bit 16) = 1
void writedata(uint16_t data) {
    //WAIT_FOR_STALL;
    //data = reversebits(data);
    //TX_FIFO = (1u << 16) | (uint32_t)data; // Bit 16 is 1 (Data)
    uint bit, pin;
    for (int i = 0; i < 16; i++)
    {
        bit = data & (1 << i);
        pin = i + PIN_LCD_DATA_BASE;
        gpio_put(pin, bit);
    }
    gpio_put(PIN_LCD_DC, 1);
    gpio_put(PIN_LCD_WR, 0);
    sleep_us(1000);
    gpio_put(PIN_LCD_WR, 1);
    sleep_us(1000);
}

// Send 16-bit Data: DC (Bit 16) = 1
void writedata16(uint16_t data) {
    //WAIT_FOR_STALL;
    //TX_FIFO = (1u << 16) | (uint32_t)data;
    uint bit, pin;
    for (int i = 0; i < 16; i++)
    {
        bit = data & (1 << i);
        pin = i + PIN_LCD_DATA_BASE;
        gpio_put(pin, bit);
    }
    gpio_put(PIN_LCD_DC, 1);
    gpio_put(PIN_LCD_WR, 0);
    sleep_us(1000);
    gpio_put(PIN_LCD_WR, 1);
    sleep_us(1000);
}

// Set Drawing Window (Matches Trace: 2A -> Data -> 2B -> Data -> 2C)
void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Column Address Set
    writecommand(0x2A);
    writedata(x0 >> 8); writedata(x0 & 0xFF);
    writedata(x1 >> 8); writedata(x1 & 0xFF);

    // Page Address Set
    writecommand(0x2B);
    writedata(y0 >> 8); writedata(y0 & 0xFF);
    writedata(y1 >> 8); writedata(y1 & 0xFF);

    // Memory Write (The trigger!)
    writecommand(0x2C); 
    // After this, DC stays High for the pixel stream
}

// --- Initialization Sequence (From SPI Trace) ---
void lcd_driver_init() {
    // 1. Software Reset
    writecommand(0x01);
    sleep_ms(120);

    // 2. Power Control B (CF)
    writecommand(0xCF);
    writedata(0x00); writedata(0xD9); writedata(0x30);

    // 3. Power On Sequence (ED)
    writecommand(0xED);
    writedata(0x64); writedata(0x03); writedata(0x12); writedata(0x81);

    // 4. Driver Timing Control A (E8)
    writecommand(0xE8);
    writedata(0x85); writedata(0x10); writedata(0x7A);

    // 5. Power Control A (CB)
    writecommand(0xCB);
    writedata(0x39); writedata(0x2C); writedata(0x00); writedata(0x34); writedata(0x02);

    // 6. Pump Ratio Control (F7)
    writecommand(0xF7);
    writedata(0x20);

    // 7. Driver Timing Control B (EA)
    writecommand(0xEA);
    writedata(0x00); writedata(0x00);

    // 8. Power Control 1 (C0)
    writecommand(0xC0);
    writedata(0x21);

    // 9. Power Control 2 (C1)
    writecommand(0xC1);
    writedata(0x12);

    // 10. VCOM Control 1 (C5)
    writecommand(0xC5);
    writedata(0x39); writedata(0x37);

    // 11. VCOM Control 2 (C7)
    writecommand(0xC7);
    writedata(0xAB);

    // 12. Memory Access Control (36)
    writecommand(0x36);
    writedata(0x48); // MX + BGR

    // 13. Pixel Format Set (3A)
    writecommand(0x3A);
    writedata(0x55); // 16-bit

    // 14. Frame Rate Control (B1)
    writecommand(0xB1);
    writedata(0x00); writedata(0x1B);

    // 15. Display Function Control (B6)
    writecommand(0xB6);
    writedata(0x0A); writedata(0xA2);

    // 16. Gamma Disable (F2)
    writecommand(0xF2);
    writedata(0x00);

    // 17. Gamma Curve (26)
    writecommand(0x26);
    writedata(0x01);

    // 18. Positive Gamma (E0)
    writecommand(0xE0);
    writedata(0x0F); writedata(0x23); writedata(0x1F); writedata(0x0B);
    writedata(0x0E); writedata(0x08); writedata(0x4B); writedata(0xA8);
    writedata(0x3B); writedata(0x0A); writedata(0x14); writedata(0x06);
    writedata(0x10); writedata(0x09); writedata(0x00);

    // 19. Negative Gamma (E1)
    writecommand(0xE1);
    writedata(0x00); writedata(0x1C); writedata(0x20); writedata(0x04);
    writedata(0x10); writedata(0x08); writedata(0x34); writedata(0x47);
    writedata(0x44); writedata(0x05); writedata(0x0B); writedata(0x09);
    writedata(0x2F); writedata(0x36); writedata(0x0F);

    writecommand(0x2B);
    writedata(0x00); writedata(0x00); writedata(0x01); writedata(0x3F);

    writecommand(0x2A);
    writedata(0x00); writedata(0x00); writedata(0x00); writedata(0xEF);

    writecommand(0x11);
    writecommand(0x29);
    
    sleep_ms(120);

    writecommand(0x36);
    writedata(0x08); 
}

void lcd_init() {
    // 1. Setup Pins and PIO
    //lcd_pio_init();
    for (int i = PIN_LCD_DATA_BASE; i < PIN_LCD_DC; i++)
    {
        gpio_init(i);
        gpio_set_dir(i, GPIO_OUT);
        gpio_put(i, 0);
    }
    gpio_init(PIN_LCD_DC);
    gpio_set_dir(PIN_LCD_DC, GPIO_OUT);
    gpio_put(PIN_LCD_DC, 0);
    gpio_init(PIN_LCD_WR);
    gpio_set_dir(PIN_LCD_WR, GPIO_OUT);
    gpio_put(PIN_LCD_WR, 1);

    // 2. Hardware Reset
    if (PIN_LCD_RST >= 0) {
        gpio_init(PIN_LCD_RST);
        gpio_set_dir(PIN_LCD_RST, GPIO_OUT);
        gpio_put(PIN_LCD_RST, 1);
        sleep_ms(5);
        gpio_put(PIN_LCD_RST, 0);
        sleep_ms(20);
        gpio_put(PIN_LCD_RST, 1);
        sleep_ms(150);
    }

    // 3. Chip Select (Always Active Low)
    if (PIN_LCD_CS >= 0) {
        gpio_init(PIN_LCD_CS);
        gpio_set_dir(PIN_LCD_CS, GPIO_OUT);
        gpio_put(PIN_LCD_CS, 0); 
    }

    // 4. Send Init Commands
    lcd_driver_init();

    gpio_put(PIN_LCD_CS, 1);
}

void fillScreen(uint16_t color) {
    // 1. Define window (Full Screen)
    setWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

    // 2. Stream Pixels
    // Note: setWindow sent 0x2C (Write RAM), so we are now in Data Mode.
    // We keep Bit 16 High (Data) for all pixels.
    uint32_t total_pixels = (uint32_t)LCD_WIDTH * LCD_HEIGHT;
    
    // OPTIMIZATION: Prepare the 32-bit word once
    uint32_t pixel_packet = (1u << 16) | color;

    for (uint32_t i = 0; i < total_pixels; i++) {
        //WAIT_FOR_STALL;
        //TX_FIFO = pixel_packet;
        writedata16(color);
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("Starting LCD Color Test (PIO Manual Mode)...\n");

    // Initialize LCD
    lcd_init();
    printf("Initialization Complete.\n");

    gpio_put(PIN_LCD_CS, 0);

    // Color Cycle Loop
    while (1) {
        printf("Filling Red...\n");
        fillScreen(COLOR_RED);
        sleep_ms(500);

        printf("Filling Green...\n");
        fillScreen(COLOR_GREEN);
        sleep_ms(500);

        printf("Filling Blue...\n");
        fillScreen(COLOR_BLUE);
        sleep_ms(500);

        printf("Filling White...\n");
        fillScreen(COLOR_WHITE);
        sleep_ms(500);

        printf("Filling Black...\n");
        fillScreen(COLOR_BLACK);
        sleep_ms(500);
    }
}