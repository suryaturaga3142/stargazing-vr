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
#include <stdbool.h>

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

/**
 * @brief A group of Quaternions to hold the complete fix data.
 * @details total = loc * time
 */
typedef struct {
    Quaternion_t loc, time, total;
} Qfix_t;

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
 * @brief Julian Date for all astronomical calculations.
 * @details A continuous count of days since a standard epoch.
 * Linear format better for sidereal time calc. Struct makes this safer.
 */
typedef struct {
    double jd; // Use a 'double' for maximum precision in calculations.
} JulianDate_t;

/**
 * @brief Sidereal time in radians for quaternion calculations
 * @details High precision in radians to allow easier calcs. This is relative to 
 * stars so accounts for revolution.
 */
typedef struct {
    double st;
} SideReal_t;

/**
 * @brief A manual definition of the exact RTC format argument passed
 * @details RTC needs this format to set it. Linker isn't working so use this.
 */
typedef struct {
    int16_t year;    ///< 0..4095
    int8_t month;    ///< 1..12, 1 is January
    int8_t day;      ///< 1..28,29,30,31 depending on month
    int8_t dotw;     ///< 0..6, 0 is Sunday
    int8_t hour;     ///< 0..23
    int8_t min;      ///< 0..59
    int8_t sec;      ///< 0..59
} datetime_t;

/* --------------------------- Star Catalog Data Types ---------------------- */

/**
 * @brief Header for the stars.bin file on the SD card.
 * @details Provides metadata to validate the file and understand its contents.
 */
typedef struct __attribute__((packed)) {
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
typedef struct __attribute((packed))__ {
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
 * x, y, and z are cartesian coordinates. mag is standard range b/w -1.5 - 6.5 and lower is brighter.
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
 * @details Bundles the orientation and velocity from a single 200Hz IMU update
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


/* -------------------------- User UI ------------------------- */

/**
 * @brief A globally accessed enum to control the state
 * @details All modules can access to change RGB LED
 */
typedef enum {
    LED_STATE_BOOTING,          // White / Pulse
    LED_STATE_SD_LOADING,       // Blue / Pulse
    LED_STATE_GPS_SEARCHING,    // Yellow / Pulse
    LED_STATE_RUN,              // Green / Solid (GPS Mode)
    LED_STATE_RUN_J2000,        // Cyan / Solid (J2000 Mode)
    LED_STATE_TIMELAPSE,        // Purple / Pulse
    LED_STATE_RUN_NO_FIX,       // Red / Slow Blink
    LED_STATE_WARN_OVERHEAT,    // Orange / Pulse
    LED_STATE_ERR_CRITICAL,     // Red / Fast Blink
    LED_STATE_REBOOTED,         // Magenta / Blink (Watchdog Reset)
    LED_STATE_DRIFT_CONFIRM     // Cyan / Fast Blink (Feedback)
} LEDState_e;

/**
 * @brief Represents the current state of the RGB LED indicator.
 * @details Used by the UI manager to control the color and pattern of the
 * user-facing status LED based on the system state.
 */
typedef struct {
    LEDState_e state;
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
    } color;
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
} StateDetails_t;

#endif /* STRUCTS_H */