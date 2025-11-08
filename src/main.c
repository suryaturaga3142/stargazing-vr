#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

// Include your driver and the generated PIO header
#include "display.h"
#include "lcd_parallel.pio.h"

// --- Pin Definitions ---
// As you specified:
// Data pins: GP2 - GP17 (16 pins)
// D/C pin:   GP18 (This is Data Base + 16)
// WR pin:    GP19
#define PIN_DATA_BASE 2
#define PIN_WR 19

// --- PIO Definitions ---
#define PIO_DISP pio0
#define SM_DISP 0

// This is the matrix your display.c expects
int star_matrix[AMOUNT_OF_STARS][2];

int main() {
    // --- 1. Standard Init ---
    stdio_init_all();
    sleep_ms(2000); // Wait for serial monitor to connect
    printf("--- Simple Single Star Test ---\n");

    // --- 2. Initialize PIO ---
    printf("Initializing PIO...\n");
    uint offset = pio_add_program(PIO_DISP, &lcd_parallel_program);
    
    // Set clock divider to 1.0f for max speed
    lcd_parallel_program_init(
        PIO_DISP,
        SM_DISP,
        offset,
        PIN_DATA_BASE,
        PIN_WR,
        1.0f 
    );

    // --- 3. Initialize Display Driver ---
    printf("Initializing Display Driver...\n");
    // This sets up the DMA channel
    display_dma_init(PIO_DISP, SM_DISP);
    
    // This sets the internal 'old_star_data' buffer to all (-1, -1)
    display_init_star_cache();

    // --- 4. Prepare the Star Data ---
    printf("Preparing star matrix...\n");
    
    // First, set all 1000 stars in our *new* list to (-1, -1).
    // This matches the 'old_star_data' cache, so the driver
    // will think 999 stars have not "moved" and will do nothing.
    for (int i = 0; i < AMOUNT_OF_STARS; i++) {
        star_matrix[i][0] = -1;
        star_matrix[i][1] = -1;
    }

    // Now, set just ONE star (star #0) to the center of the display.
    // The display is 320x480, so the center is (160, 240).
    star_matrix[0][0] = 160;
    star_matrix[0][1] = 240;

    // --- 5. Process and Draw the Star ---
    printf("Calling place_new_stars()...\n");
    
    // This function will compare the old cache (-1, -1) with our new matrix.
    // It will find only ONE star changed (from -1,-1 to 160,240)
    // and will generate the packets to draw it.
    place_new_stars(star_matrix);

    printf("Starting DMA transfer...\n");
    display_dma_start_transfer();

    // Wait for the DMA to finish sending the data
    while (display_dma_is_busy()) {
        tight_loop_contents();
    }

    printf("Transfer complete. A single star should be visible at (160, 240).\n");
    printf("Halting.\n");

    // Stop here
    while (true) {
        sleep_ms(1000);
    }

    return 0;
}