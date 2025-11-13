/*******************************************************************************
 * @file        main.c
 * @brief       Implements the functionality for the main module.
 * @details     This is the main file for the project.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * 
 * @note        This module is designed to be the main module to drive everything.
 *              Don't put interrupts here. Initialize everything, go through the
 *              startup process, and execute the lowest level non critical blocking
 *              tasks that potentially cause priority blocking when in handlers.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/i2c.h"
#include <stdio.h>

#include "globals.h" // Includes all project headers
#include "gps.h"     // The driver we are testing
#include "imu.h"

#include "pico/stdlib.h"

/* ---------------------------- Private Constants --------------------------- */
// #define LCD_SPI_TEST
// #define GPS_DUMB_TEST
// #define GPS_RAW_TEST
// #define IMU_TEST
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
//#include "ff.h"       // FatFS header
//#include "f_util.h"   // FatFS utility header
#include "sd_card.h"  // The library's header

// Buffer to store file contents
#define BUFFER_SIZE 512
char my_buffer[BUFFER_SIZE];

#define GPS_RAW_TEST

// Forward declaration for our config function
void setup_sd_card_config();

#ifdef PIO_SDIO
int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("--- RP2350 PIO-SDIO File Read Test ---\n");

    // 1. Apply our custom PIO configuration
    setup_sd_card_config();

    // 2. Get the SD card object (already configured)
    sd_card_t *pSD = sd_get_by_num(0);

    // 3. Mount the filesystem
    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1); // 1 = mount now
    if (fr != FR_OK) {
        printf("ERROR: Failed to mount filesystem! (Code %d)\n", fr);
        while(true);
    }
    printf("Filesystem mounted.\n");

    // 4. Open the file
    FIL fil;
    const char* filename = "test.txt";
    fr = f_open(&fil, filename, FA_READ);
    if (fr != FR_OK) {
        printf("ERROR: Failed to open file '%s'. (Code %d)\n", filename, fr);
        f_unmount(pSD->pcName);
        while(true);
    }
    printf("File '%s' opened.\n", filename);

    // 5. Read the file into your buffer
    UINT bytes_read;
    fr = f_read(&fil, my_buffer, sizeof(my_buffer) - 1, &bytes_read);
    if (fr != FR_OK) {
        printf("ERROR: Failed to read from file! (Code %d)\n", fr);
        f_close(&fil);
        f_unmount(pSD->pcName);
        while(true);
    }

    // 6. Null-terminate the buffer and print the contents
    my_buffer[bytes_read] = '\0';
    printf("Successfully read %u bytes.\n", bytes_read);
    printf("--- File Content ---\n");
    printf("%s\n", my_buffer);
    printf("--- End of File ---\n");

    // 7. Clean up
    f_close(&fil);
    f_unmount(pSD->pcName);
    printf("Test complete. SD card unmounted.\n");

    while(true) {
        sleep_ms(1000);
    }
    return 0;
}



// This function gets the library's default config
// and then we modify it.
void setup_sd_card_config() {
    // Get the pointer to the default SD card structure
    sd_card_t *pSD = sd_get_by_num(0); 

    // --- THIS IS THE MAGIC SWITCH ---
    // Tell the library to use the PIO implementation
    pSD->use_pio = true;

    // --- CONFIGURE YOUR PINS ---
    // Set these to match your board's wiring
    pSD->sdio_if.clk_gpio = 7; // Example: CLK on GPIO 2
    pSD->sdio_if.cmd_gpio = 2; // Example: CMD on GPIO 3
    pSD->sdio_if.d0_gpio = 3;  // Example: DAT0 on GPIO 4
    pSD->sdio_if.d1_gpio = 4;  // Example: DAT1 on GPIO 5
    pSD->sdio_if.d2_gpio = 5;  // Example: DAT2 on GPIO 6
    pSD->sdio_if.d3_gpio = 6;  // Example: DAT3 on GPIO 7

    // --- CONFIGURE THE PIO INSTANCE ---
    // Tell it which PIO block and State Machine to use
    pSD->sdio_if.pio_inst = pio0; // Use pio0
    pSD->sdio_if.sm_inst = 0;     // Use State Machine 0
    pSD->sdio_if.dma_chan = 0;    // Use DMA channel 0
}

#endif





















#ifdef LCD_SPI_TEST

#include "pico/stdlib.h"
#include <stdio.h>
#include "display.h"

// Define 16-bit (RGB565) colors
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_WHITE   0xFFFF
#define COLOR_BLACK   0x0000

int main() {
    // Initialize standard I/O (for printf over USB)
    stdio_init_all();
    sleep_ms(4000); // Wait for terminal to connect

    printf("=====================================\n");
    printf("     ILI9486 SPI Test\n");
    printf("=====================================\n");

    // Initialize the LCD
    lcd_init();
    printf("LCD Init complete.\n");

    // Run a color cycle test
    while (true) {
        printf("Filling screen RED\n");
        lcd_fill_screen(COLOR_RED);
        sleep_ms(1000);

        printf("Filling screen GREEN\n");
        lcd_fill_screen(COLOR_GREEN);
        sleep_ms(1000);

        printf("Filling screen BLUE\n");
        lcd_fill_screen(COLOR_BLUE);
        sleep_ms(1000);
        
        printf("Filling screen BLACK\n");
        lcd_fill_screen(COLOR_BLACK);
        sleep_ms(1000);
    }

    return 0;
}


#endif


#ifdef GPS_DUMB_TEST

int main() {
    // Initialize standard I/O (for printf over USB)
    stdio_init_all();
    sleep_ms(2000); // Wait for terminal to connect
    printf("=====================================\n");
    printf("Starting GPS Raw NMEA Test...\n");
    printf("=====================================\n");

    // Initialize the GPS driver
    gps_init();
    printf("GPS driver initialized. Forcing NMEA output at 9600 baud...\n");
    printf("Listening for NMEA sentences from NEO-M10:\n\n");

    for(;;) {

    }
    return 0;
}

#endif

#ifdef GPS_RAW_TEST

int main() {
    // Initialize standard I/O (for printf over USB)
    stdio_init_all();
    sleep_ms(2000); // Wait for terminal to connect
    printf("=====================================\n");
    printf("Starting GPS Raw NMEA Test...\n");
    printf("=====================================\n");

    // Initialize the GPS driver
    gps_init();
    printf("GPS driver initialized. Forcing NMEA output at 38400 baud...\n");
    printf("Listening for NMEA sentences from NEO-M10:\n\n");

    // Main application loop
    while (true) {
        // This function checks for a new line, prints it, and parses it.
        // It does all the work for this test.
        gps_update();

        // Note: We don't need a sleep_ms() here because the test
        // is interrupt-driven. gps_update() will return immediately
        // if no new line is ready, keeping the loop tight.
    }

    return 0;
}

#endif


#ifdef IMU_TEST

int main() {

    return 0;
}

#endif
