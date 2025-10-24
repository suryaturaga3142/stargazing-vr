/*******************************************************************************
 * @file        config.h
 * @brief       Set of project configuration variables and definitions.
 * @details     This header file will define all variables related to 
 *              controlling the software before flashing.
 * 
 * @see         config.c for implementation details.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * @version     1.0
 ******************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

//------------------------------------------------------------------------------
// SYSTEM TIMING CONSTANTS
//------------------------------------------------------------------------------

#define TARGET_REFRESH_RATE_HZ          150
#define IMU_SAMPLE_RATE_HZ              100
#define GPS_CORRECTION_INTERVAL_MIN     10
#define WATCHDOG_SUPERVISOR_INTERVAL_MS 100
#define WATCHDOG_TIMEOUT_MS             200

//------------------------------------------------------------------------------
// STAR CATALOG & CULLING CONSTANTS
//------------------------------------------------------------------------------

#define STAR_CATALOG_SIZE_MAX           15000
#define SKY_PATCH_RA_DIVISIONS          24
#define SKY_PATCH_DEC_DIVISIONS         12

//------------------------------------------------------------------------------
// PERIPHERAL CONFIGURATION
//------------------------------------------------------------------------------

// -- ILI9486 Displays (GPIO Bank 0-20) --
// The 16-bit data bus is implemented with PIO.
#define PIN_LCD_D0                      0   // D0-D15 MUST be consecutive. Uses GPIO 0-15.
#define PIN_LCD_RESET                   16
#define PIN_LCD_CS                      17
#define PIN_LCD_DC                      18  // Data/Command
#define PIN_LCD_WR                      19  // Write Strobe
#define PIN_LCD_RD                      20  // Read Strobe is tied to 3.3V, but we reserve the pin.

// -- FORBIDDEN ZONE (GPIO 21-26) --
// Connected to Proton board PBs and LEDs.

// -- BNO085 IMU (GPIO Bank 27-30) --
// Grouped together for clean I2C routing.
#define I2C_PORT                        i2c0
#define IMU_I2C_ADDR                    0x4A
#define PIN_IMU_RST                     27  // Reset pin
#define PIN_IMU_SDA                     28
#define PIN_IMU_SCL                     29
#define PIN_IMU_INT                     30  // Interrupt pin

// -- NEO-M10 GPS (GPIO Bank 38-39) --
#define UART_PORT                       uart1
#define GPS_UART_BAUD                   9600
#define PIN_GPS_TX                      38 // RP2350 TX -> GPS RX (For UBX commands)
#define PIN_GPS_RX                      39 // RP2350 RX <- GPS TX (For NMEA data)

// -- SD Card (via SDIO on a high bank) --
// Uses a second PIO for implementing SDIO.
#define PIN_SDIO_CLK                    31
#define PIN_SDIO_CMD                    32
#define PIN_SDIO_D0                     33  // D0-D3 MUST be consecutive. Uses GPIO 33-36

// -- User Interface (GPIO Bank 40-44) --
// Grouped together for clean User UI routing.
#define PIN_LED_R                       40
#define PIN_LED_G                       41
#define PIN_LED_B                       42
#define PIN_BTN_DRIFT_CORRECT           43
#define PIN_BTN_LOCATION_TOGGLE         44

// -- LED Related Definitions --
// These define the actual periods in milliseconds for the LED patterns.
#define LED_PERIOD_SLOW_MS      1000
#define LED_PERIOD_MEDIUM_MS    500
#define LED_PERIOD_FAST_MS      250
// -- LED Color Definitions (24-bit RGB Hex: 0x00RRGGBB) --
#define LED_HEX_OFF             0x00000000
#define LED_HEX_WHITE           0x00FFFFFF
#define LED_HEX_BLUE            0x000000FF
#define LED_HEX_YELLOW          0x00FFFF00
#define LED_HEX_GREEN           0x0000FF00
#define LED_HEX_CYAN            0x0000FFFF
#define LED_HEX_RED             0x00FF0000
#define LED_HEX_ORANGE          0x00FFA500
#define LED_HEX_MAGENTA         0x00FF00FF
#define LED_HEX_PURPLE          0x00800080
// -- LED RGB Isolation Macros --
#define LED_VAL_R(hex)          (hex >> 16) & 0xFF
#define LED_VAL_G(hex)          (hex >>  8) & 0xFF
#define LED_VAL_B(hex)          (hex >>  0) & 0xFF

#endif /* CONFIG_H */