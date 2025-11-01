/*******************************************************************************
 * @file        startup.c
 * @brief       Implements the functionality for startup.
 * @details     All the actual functions for initialization. the entire file
 *              buffer resides as a static here. Implement sorting here.
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
#include <string.h>
#include <math.h>
#include <stdlib.h>
// ...

/* ---------------------------- Private Constants --------------------------- */

#define M_PI		3.14159265358979323846
// ...

/* ----------------------------- Private Variables -------------------------- */

// --- Global Star Database ---
// Define the large buffer. It MUST be 32-bit aligned for the DMA.
static uint8_t g_file_buffer[TEMP_STAR_BUFFER_SIZE] __attribute__((aligned(4)));
// ...

/* ----------------------------- Private Functions -------------------------- */

/**
 * @brief Calculates the patch indices for a given star position.
 * @param ra_deg Right Ascension in degrees (0 to 360).
 * @param dec_deg Declination in degrees (-90 to +90).
 * @param ra_idx Output pointer for the RA index (0 to SKY_PATCH_RA_DIVISIONS-1).
 * @param dec_idx Output pointer for the Dec index (0 to SKY_PATCH_DEC_DIVISIONS-1).
 */
static inline void _get_patch_indices(float ra_deg, float dec_deg, int* ra_idx, int* dec_idx) {
    // Wrap RA to be safe [0, 360)
    ra_deg = fmodf(ra_deg, 360.0f);
    if (ra_deg < 0) ra_deg += 360.0f;

    *ra_idx = (int)floorf(ra_deg / (360.0f / SKY_PATCH_RA_DIVISIONS));
    // Clamp index just in case of floating point edge cases (e.g., exactly 360)
    if (*ra_idx >= SKY_PATCH_RA_DIVISIONS) *ra_idx = SKY_PATCH_RA_DIVISIONS - 1;
    if (*ra_idx < 0) *ra_idx = 0; // Should not happen with fmod above, but safety first

    // Shift Dec range from [-90, +90] to [0, 180] before calculating index
    // Note the small epsilon to handle exactly +90 declination correctly
    *dec_idx = (int)floorf((dec_deg + 90.0f) / (180.0f / SKY_PATCH_DEC_DIVISIONS));
    // Clamp index
    if (*dec_idx < 0) *dec_idx = 0;
    if (*dec_idx >= SKY_PATCH_DEC_DIVISIONS) *dec_idx = SKY_PATCH_DEC_DIVISIONS - 1;
}

/**
 * @brief Unpacks scaled integer star data into floating-point values.
 * @param packed_star Pointer to the packed star data.
 * @param ra Output pointer for RA in degrees.
 * @param dec Output pointer for Dec in degrees.
 * @param pmra Output pointer for Proper Motion RA (mas/yr).
 * @param pmdec Output pointer for Proper Motion Dec (mas/yr).
 * @param mag Output pointer for Magnitude.
 */
static inline void _unpack_star(const PackedStar_t* packed_star,
                                float* ra_deg, float* dec_deg, float* pmra, float* pmdec, float* mag)
{
    // Define inverse scaling factors (matches Python script)
    // Using double for intermediate position calculation for slightly better precision
    const double INV_POS_SCALE_DEG = 180.0 / 2147483648.0; // Converts scaled int32 to degrees
    const float INV_PM_SCALE = 1.0f / 100.0f;
    const float INV_MAG_SCALE = 1.0f / 1000.0f;

    *ra_deg = (float)((double)packed_star->ra_scaled * INV_POS_SCALE_DEG);
    *dec_deg = (float)((double)packed_star->dec_scaled * INV_POS_SCALE_DEG);
    *pmra = (float)packed_star->pmra_scaled * INV_PM_SCALE;
    *pmdec = (float)packed_star->pmdec_scaled * INV_PM_SCALE;
    *mag = (float)packed_star->mag_scaled * INV_MAG_SCALE;
}

/**
 * @brief Converts spherical coordinates (RA/Dec in degrees) to Cartesian (x,y,z) on a unit sphere.
 * @param ra_deg Right Ascension in degrees.
 * @param dec_deg Declination in degrees.
 * @param x Output pointer for X coordinate.
 * @param y Output pointer for Y coordinate.
 * @param z Output pointer for Z coordinate.
 */
static inline void _spherical_to_cartesian(float ra_deg, float dec_deg, float* x, float* y, float* z) {
    const float DEG_TO_RAD = (float)M_PI / 180.0f;
    float ra_rad = ra_deg * DEG_TO_RAD;
    float dec_rad = dec_deg * DEG_TO_RAD;

    float cos_dec = cosf(dec_rad);
    *x = cosf(ra_rad) * cos_dec;
    *y = sinf(ra_rad) * cos_dec; // Assuming Z is North Pole (Dec=90), X is RA=0 on Equator
    *z = sinf(dec_rad);
}

//------------------------------------------------------------------------------
// Star Database Compilation (The 3-Pass Method)
//------------------------------------------------------------------------------

/**
 * @brief Processes the raw packed star data and sorts it into the final render-ready database.
 * @param packed_data Pointer to the raw PackedStar_t data loaded into the temporary buffer.
 * @param count The number of stars loaded (g_star_count).
 * @return true if successful, false otherwise.
 */
static bool _compile_star_database(const PackedStar_t* packed_data, uint32_t count) {
    printf("Compiling star database (%lu stars)...\n", count);
    if (count == 0 || count > STAR_CATALOG_SIZE_MAX) {
        printf("ERROR: Invalid star count for compilation.\n");
        return false;
    }

    // --- Pass 1: Count stars per patch ---
    printf("Pass 1: Counting stars per patch...\n");
    memset(sky_database, 0, sizeof(sky_database)); // Clear counts and indices
    for (uint32_t i = 0; i < count; ++i) {
        // Unpack just enough to get position for sorting
        float ra_deg, dec_deg, dummy_pmra, dummy_pmdec, dummy_mag;
        _unpack_star(&packed_data[i], &ra_deg, &dec_deg, &dummy_pmra, &dummy_pmdec, &dummy_mag);

        // TODO: Apply Proper Motion here to get historical position if needed
        // For now, sorting based on J2000 epoch position
        float sort_ra_deg = ra_deg;
        float sort_dec_deg = dec_deg;

        int ra_idx, dec_idx;
        _get_patch_indices(sort_ra_deg, sort_dec_deg, &ra_idx, &dec_idx);

        sky_database[ra_idx][dec_idx].star_count++;

        // Pet watchdog periodically during long loops
        if ((i % 1000) == 0) {
             // watchdog_update(); // Assuming watchdog is active
        }
    }

    // --- Pass 2: Calculate start indices for each patch ---
    printf("Pass 2: Calculating patch start indices...\n");
    uint32_t current_index = 0;
    for (int i = 0; i < SKY_PATCH_RA_DIVISIONS; ++i) {
        for (int j = 0; j < SKY_PATCH_DEC_DIVISIONS; ++j) {
            sky_database[i][j].start_index = current_index;
            current_index += sky_database[i][j].star_count;
        }
    }
    // Verify total count matches expected count
    if (current_index != count) {
         printf("ERROR: Total stars in patches (%lu) != loaded count (%lu) after Pass 2!\n", current_index, count);
         return false;
    }

    // --- Pass 3: Unpack, Convert, and Place stars into final sorted array ---
    printf("Pass 3: Processing and placing stars...\n");
    // Create temporary counters for placement within each patch, initialized to 0
    uint16_t patch_placement_count[SKY_PATCH_RA_DIVISIONS][SKY_PATCH_DEC_DIVISIONS];
    memset(patch_placement_count, 0, sizeof(patch_placement_count));

    for (uint32_t i = 0; i < count; ++i) {
        // 1. Unpack fully
        float ra_deg, dec_deg, pmra, pmdec, mag;
        _unpack_star(&packed_data[i], &ra_deg, &dec_deg, &pmra, &pmdec, &mag);

        // 2. TODO: Apply Proper Motion here to get historical position
        //    (e.g., historical_ra = ra - pmra * years_since_epoch)
        //    This requires knowing the reference epoch of the FITS data
        //    and the target epoch for your static sphere.
        //    For now, we use the J2000 coords directly.
        float final_ra_deg = ra_deg;
        float final_dec_deg = dec_deg;

        // 3. Find its patch again (using the same coords as in Pass 1)
        int ra_idx, dec_idx;
        _get_patch_indices(final_ra_deg, final_dec_deg, &ra_idx, &dec_idx);

        // 4. Calculate destination index using the patch's start_index and the current count for that patch
        uint32_t patch_start = sky_database[ra_idx][dec_idx].start_index;
        uint16_t current_offset = patch_placement_count[ra_idx][dec_idx];
        uint32_t dest_idx = patch_start + current_offset;

        if (dest_idx >= count) {
             printf("ERROR: Calculated destination index %lu out of bounds (count %lu) during placement!\n", dest_idx, count);
             return false;
        }

        // 5. Convert final spherical coordinates to Cartesian for rendering
        _spherical_to_cartesian(final_ra_deg, final_dec_deg,
                                &all_stars[dest_idx].x,
                                &all_stars[dest_idx].y,
                                &all_stars[dest_idx].z);
        all_stars[dest_idx].mag = mag;

        // 6. Increment the placement counter for this specific patch
        patch_placement_count[ra_idx][dec_idx]++;

        // Pet watchdog periodically
        if ((i % 1000) == 0) {
            // watchdog_update(); // Assuming watchdog is active
        }
    }

    // Final Validation (Optional but Recommended)
    printf("Validating placement...\n");
    for (int i = 0; i < SKY_PATCH_RA_DIVISIONS; ++i) {
        for (int j = 0; j < SKY_PATCH_DEC_DIVISIONS; ++j) {
             if (patch_placement_count[i][j] != sky_database[i][j].star_count) {
                 printf("ERROR: Mismatch in placed stars for patch (%d, %d)! Expected %u, Placed %u\n",
                         i, j, sky_database[i][j].star_count, patch_placement_count[i][j]);
                 return false;
             }
        }
    }

    printf("Star database compilation successful.\n");
    return true;
}


//------------------------------------------------------------------------------
// SD Card Data Loading Function (Called by run_startup_sequence)
//------------------------------------------------------------------------------
// Helper to round up division
static uint32_t div_round_up(uint32_t n, uint32_t d) {
    return (n + d - 1) / d;
}

/**
 * @brief Loads the star catalog header and data from SD card into RAM buffer.
 * @details This function reads the entire `stars.bin` file into the static
 * `file_buffer` and sets the global `g_star_count`.
 * @return Pointer to the start of the PackedStar_t data within the buffer,
 * or NULL on failure.
 */
static const PackedStar_t* load_star_data_from_sd(void) {
    printf("Initializing SD card...\n");
    // watchdog_update(); // Pet before potential long init
    int init_status = sd_init();
    if (init_status != 0) {
        printf("SD card initialization failed with code: %d\n", init_status);
        // set_led_state(STATE_CRITICAL_ERROR);
        return NULL;
    }
    printf("SD card initialized.\n");

    StarFileHeader_t file_header;
    uint32_t bytes_read;

    // --- 1. Read the file header ---
    // Read just the header first to get the star count
    printf("Reading file header...\n");
    // watchdog_update();
    // Use sector 0 for the header (assuming header fits in first block)
    if (sd_read_blocks(0, g_file_buffer, 1) != 0) {
         printf("ERROR: Failed to read block 0 for header.\n");
         // set_led_state(STATE_CRITICAL_ERROR);
         return NULL;
    }
    memcpy(&file_header, g_file_buffer, sizeof(file_header)); // Copy header from buffer

    // --- 2. Validate header ---
    printf("Validating header...\n");
    // (Your existing validation code...)
    if (file_header.magic_number != STAR_FILE_MAGIC ||
        file_header.star_count == 0 ||
        file_header.star_count > STAR_CATALOG_SIZE_MAX)
    {
        printf("ERROR: Invalid file header.\n");
        // set_led_state(STATE_CRITICAL_ERROR);
        return NULL;
    }
    g_star_count = file_header.star_count; // Store the actual count globally
    printf("Header valid: %lu stars.\n", g_star_count);

    // --- 3. Read the star data payload into the static buffer ---
    uint32_t header_size_actual = sizeof(StarFileHeader_t); // Use actual size
    uint32_t payload_size = g_star_count * sizeof(PackedStar_t);
    uint32_t total_file_size = header_size_actual + payload_size;
    uint32_t total_blocks_needed = div_round_up(total_file_size, SD_BLOCK_SIZE);

    printf("Total file size: %lu bytes (%lu blocks)\n", total_file_size, total_blocks_needed);

    if (total_file_size > TEMP_STAR_BUFFER_SIZE) {
         printf("ERROR: Total file size (%lu) exceeds static buffer size (%d)!\n", total_file_size, TEMP_STAR_BUFFER_SIZE);
         // set_led_state(STATE_CRITICAL_ERROR);
         return NULL;
    }

    // Read the entire file now (including header again, simpler than seeking)
    printf("Reading entire file (%lu blocks)...\n", total_blocks_needed);
    uint32_t blocks_to_read_total = total_blocks_needed;
    uint32_t current_block_total = 0;
    uint8_t* buffer_ptr_total = g_file_buffer;

     while (blocks_to_read_total > 0) {
        uint32_t chunk_size = (blocks_to_read_total > SDIO_MAX_BLOCKS) ? SDIO_MAX_BLOCKS : blocks_to_read_total;
        // watchdog_update();
        if (sd_read_blocks(current_block_total, buffer_ptr_total, chunk_size) != 0) {
            printf("ERROR: Failed to read blocks %lu-%lu.\n", current_block_total, current_block_total + chunk_size -1);
            // set_led_state(STATE_CRITICAL_ERROR);
            return NULL;
        }
        blocks_to_read_total -= chunk_size;
        current_block_total += chunk_size;
        buffer_ptr_total += chunk_size * SD_BLOCK_SIZE;
     }

    printf("File loaded successfully into buffer.\n");

    // Return pointer to the actual star data (after the header)
    return (const PackedStar_t*)(g_file_buffer + header_size_actual);
}

// ...

/* ----------------------------- Public Variables -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */

//------------------------------------------------------------------------------
// Main Startup Sequence (Called by main.c)
//------------------------------------------------------------------------------

bool run_startup_sequence(void) {
    // watchdog_enable(STARTUP_WATCHDOG_TIMEOUT_MS, 1);

    // set_led_state(STATE_BOOTING);
    // Initialize core peripherals...
    // DMA, Timers, Interrupts, etc.

    // set_led_state(STATE_LOADING_SD);
    const PackedStar_t* packed_star_data = load_star_data_from_sd();
    if (packed_star_data == NULL) {
        printf("CRITICAL ERROR: Failed to load star data from SD card.\n");
        // set_led_state(STATE_CRITICAL_ERROR);
        return false; // Error handled internally
    }

    // set_led_state(STATE_PROCESSING_STARS); // Add this state
    if (!_compile_star_database(packed_star_data, g_star_count)) {
        printf("ERROR: Failed to compile star database.\n");
        // set_led_state(STATE_CRITICAL_ERROR);
        return false;
    }

    // set_led_state(STATE_ACQUIRING_GPS);
    // ... rest of startup ...

    return true;
}
