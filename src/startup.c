/*******************************************************************************
 * @file        startup.c
 * @brief       Implements the functionality for startup.
 * @details     All the actual functions for initialization.
 * 
 * @author      LED Chasers
 * @date        2025-10-26
 * 
 * @note        This module is designed to be called by main once.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "startup.h"
#include "sd_card.h"
#include "config.h"
#include "structs.h"
#include "globals.h"

#include <stdio.h>
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */

// Helper to round up division
static uint32_t div_round_up(uint32_t n, uint32_t d) {
    return (n + d - 1) / d;
}
// ...

/* ----------------------------- Public Variables -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */

bool load_star_data(void) {
    printf("Initializing SD card...\n");
    int init_status = sd_init();
    if (init_status != 0) {
        printf("SD card initialization failed with code: %d\n", init_status);
        return false;
    }

    // 1. Read the first block to get the header
    printf("Reading file header (Block 0)...\n");
    int read_status = sd_read_blocks(0, g_file_buffer, 1);
    if (read_status != 0) {
        printf("Failed to read block 0 with code: %d\n", read_status);
        return false;
    }

    // 2. Cast the buffer to the header struct and validate
    StarFileHeader_t *header = (StarFileHeader_t *)g_file_buffer;

    printf("Magic: 0x%08lX (Expected: 0x%08X)\n", header->magic_number, STAR_FILE_MAGIC);
    if (header->magic_number != STAR_FILE_MAGIC) {
        printf("ERROR: Magic number mismatch! Is this the right .bin file?\n");
        return false;
    }

    printf("Version: %u\n", header->version);
    printf("Header Size: %u bytes\n", header->header_size);
    printf("Star Count: %lu\n", header->star_count);

    // 3. Calculate total file size and blocks needed
    uint32_t total_data_size = header->header_size + (header->star_count * sizeof(PackedStar_t));
    uint32_t total_blocks_needed = div_round_up(total_data_size, SD_BLOCK_SIZE);

    printf("Total file size: %lu bytes\n", total_data_size);
    printf("Total blocks needed: %lu\n", total_blocks_needed);

    if (total_data_size > TEMP_STAR_BUFFER_SIZE) {
        printf("ERROR: Star file (%lu bytes) is larger than buffer (%d bytes)!\n",
               total_data_size, TEMP_STAR_BUFFER_SIZE);
        return false;
    }

    // 4. Read the rest of the file (if it's more than one block)
    if (total_blocks_needed > 1) {
        uint32_t blocks_remaining = total_blocks_needed - 1;
        uint32_t current_block = 1; // Start from block 1
        uint8_t *buffer_offset = g_file_buffer + SD_BLOCK_SIZE;

        printf("Reading remaining %lu blocks...\n", blocks_remaining);

        // Read in chunks of SDIO_MAX_BLOCKS
        while (blocks_remaining > 0) {
            uint32_t blocks_to_read = (blocks_remaining > SDIO_MAX_BLOCKS) ? SDIO_MAX_BLOCKS : blocks_remaining;
            
            read_status = sd_read_blocks(current_block, buffer_offset, blocks_to_read);
            if (read_status != 0) {
                printf("Failed to read blocks %lu-%lu with code: %d\n",
                       current_block, current_block + blocks_to_read - 1, read_status);
                return false;
            }

            blocks_remaining -= blocks_to_read;
            current_block += blocks_to_read;
            buffer_offset += blocks_to_read * SD_BLOCK_SIZE;
        }
    }

    printf("File loaded successfully.\n");

    // 5. Set the global pointers and counts
    uint8_t *star_data_ptr = g_file_buffer + header->header_size;
    g_star_array = (PackedStar_t *)star_data_ptr;
    g_star_count = header->star_count;

    return true;
}