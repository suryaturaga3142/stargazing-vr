/*******************************************************************************
 * @file        structs.h
 * @brief       Header file for all struct definitions.
 * @details     Defines all structs meant for tasks such as stars, raw stars, 
 *              sphere sectors, etc.
 * 
 # @see         globals.c for use details.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * @version     1.0
 ******************************************************************************/

#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>
#include "hardware/pio.h"
#include "config.h"

/* -------------------------- Core Math Data Types -------------------------- */

/**
 * @brief A standard 4-element single-precision floating-point quaternion.
 * @details Represents a rotation in 3D space, typically in (w, x, y, z) order.
 */
typedef struct {
    float w, x, y, z;
} Quaternion_t;

/**
 * @brief A standard 3-element single-precision floating-point vector.
 * @details Used for positions, directions, and velocities in 3D space.
 */
typedef struct {
    float x, y, z;
} Vector3f_t;

/* -------------------------- Timekeeping Data Types ------------------------ */

/**
 * @brief A "human-readable" calendar time structure (UTC).
 * @details This format is used for interfacing with peripherals like the GPS
 * (parsing NMEA) and the hardware RTC (setting registers). It is not
 * used for high-performance mathematical calculations.
 */
typedef struct {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
} UTCTime_t;

/**
 * @brief A high-precision Julian Date for all astronomical calculations.
 * @details A Julian Date is a continuous count of days since a standard epoch.
 * This linear format is ideal for all standard celestial mechanics
 * algorithms (e.g., for sidereal time). Struct makes this safer.
 */
typedef struct {
    double jd; // Use a 'double' for maximum precision in calculations.
} JulianDate_t;


/* --------------------------- Star Catalog Data Types ---------------------- */

/**
 * @brief Header for the stars.bin file on the SD card.
 * @details Provides metadata to validate the file and understand its contents.
 */
typedef struct {
    uint32_t magic_number;    // Should be "STAR" (0x53544152) to make sure bin is right
    uint16_t version;         // File format version
    uint16_t header_size;     // Size of this header in bytes
    uint32_t star_count;      // Total number of stars in the file
} StarFileHeader_t;

/**
 * @brief Memory-efficient structure for a single star as stored on the SD card.
 * @details Uses scaled integers to minimize storage footprint. This is the
 * "on-disk" format that is unpacked at startup.
 */
typedef struct __attribute__((packed)) {
    int32_t  ra_scaled;
    int32_t  dec_scaled;
    int16_t  pmra_scaled;
    int16_t  pmdec_scaled;
    int16_t  mag_scaled;
} PackedStar_t;

/**
 * @brief Render-ready structure for a single star in RAM.
 * @details Stores the pre-calculated Cartesian coordinates on a unit sphere,
 * optimized for the real-time rendering loop. Single element in processed buffer.
 */
typedef struct {
    float x, y, z;
    float mag;
} Star_t;

/**
 * @brief Defines a single sector of the sky for spatial culling.
 * @details Acts as an index into the global `all_stars` array, describing a
 * contiguous "slice" of stars belonging to this patch. The use of a 2D array
 * makes this work by design with O(1) lookup time for a patch.
 */
typedef struct {
    uint32_t start_index;
    uint16_t star_count;
} SkyPatch_t;

/* -------------------------- Peripheral Data Types ------------------------- */

/**
 * @brief A complete, timestamped measurement snapshot from the IMU.
 * @details Bundles the orientation and velocity from a single 100Hz IMU update
 * to ensure they are always synchronized.
 */
typedef struct {
    Quaternion_t orientation; // The fused rotation vector (q)
    Vector3f_t   velocity;    // The calibrated angular velocity (ω)
    uint64_t     timestamp_us;// High-resolution timestamp of the measurement
} IMUData_t;

/**
 * @brief A complete snapshot of parsed data from a GPS fix.
 * @details Encapsulates all useful information from a set of NMEA sentences.
 */
typedef struct {
    UTCTime_t time;
    float     latitude;
    float     longitude;
    bool      is_valid;
} GPSData_t;

/* ------------------- SD Card Driver Internal Structures ------------------- */

/**
 * @brief Internal status codes for the SDIO driver.
 */
typedef enum {
    SDIO_OK = 0,
    SDIO_BUSY = 1,
    SDIO_ERR_RESPONSE_TIMEOUT = 2,
    SDIO_ERR_RESPONSE_CRC = 3,
    SDIO_ERR_RESPONSE_CODE = 4,
    SDIO_ERR_DATA_TIMEOUT = 5,
    SDIO_ERR_DATA_CRC = 6,
} sdio_status_t;

/**
 * @brief Internal state for the SD card driver.
 * @details This is defined in globals.c and should not be touched
 * externally except by the sd_card.c driver.
 */
typedef struct {
    // Pin configuration
    uint clk_gpio;
    uint cmd_gpio;
    uint d0_gpio;
    uint d1_gpio;
    uint d2_gpio;
    uint d3_gpio;

    // Card state
    uint32_t ocr; // Operating condition register
    uint32_t rca; // Relative card address
    sdio_status_t last_error;

    // PIO/DMA resources
    int dma_ch_a;
    int dma_ch_b;
    int cmd_sm;
    int data_sm;
    uint32_t pio_cmd_clk_offset;
    uint32_t pio_data_rx_offset;
    pio_sm_config pio_cfg_data_rx;

    // DMA block descriptors for read
    struct {
        void *write_addr;
        uint32_t transfer_count;
    } dma_blocks[SDIO_MAX_BLOCKS * 2 + 1]; // +1 for null terminator

    struct {
        uint32_t top;
        uint32_t bottom;
    } received_checksums[SDIO_MAX_BLOCKS];

    // Read transfer state
    uint32_t *data_buf;
    uint32_t blocks_done;
    uint32_t total_blocks;
    uint32_t blocks_checksumed;
    uint32_t checksum_errors;
    uint32_t transfer_start_time;

} sd_driver_state_t;

/* ---------------------------- System State Types -------------------------- */

/**
 * @brief Represents the current state of the RGB LED indicator.
 * @details Used by the UI manager to control the color and pattern of the
 * user-facing status LED.
 */
typedef struct {
    enum {
        LED_COLOR_OFF,
        LED_COLOR_WHITE,
        LED_COLOR_BLUE,
        LED_COLOR_YELLOW,
        LED_COLOR_GREEN,
        LED_COLOR_CYAN,
        LED_COLOR_RED,
        LED_COLOR_ORANGE,
        LED_COLOR_MAGENTA,
        LED_COLOR_PURPLE
    } LED_Color_t;
    enum {
        LED_SOLID,
        LED_BLINK,
        LED_PULSE
    } pattern;
    enum {
        LED_SPEED_SLOW,
        LED_SPEED_MEDIUM,
        LED_SPEED_FAST
    } speed;
} LEDState_t;


#endif /* STRUCTS_H */