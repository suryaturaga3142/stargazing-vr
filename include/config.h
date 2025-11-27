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

#define TARGET_REFRESH_RATE_HZ          200
#define GPS_CORRECTION_INTERVAL_MIN     1
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
#define PIN_LCD_DC                      2  // Data/Command
#define PIN_LCD_WR                      3  // Write Strobe
#define PIN_LCD_RESET                   4
#define PIN_LCD_D0                      5  // D0-D15 MUST be consecutive. Uses GPIO 5-20.
#define PIN_LCD_BL                      27

// -- In Built Zone (GPIO 21-26) --
// Connected to Proton board PBs and LEDs.
#define PIN_PB_1  21
#define PIN_PB_2  26
#define PIN_LED_1 22
#define PIN_LED_2 23
#define PIN_LED_3 24
#define PIN_LED_4 25

// -- BNO085 IMU (GPIO Bank 27-30) --
// Grouped together for clean I2C routing.
#define I2C_PORT                        i2c0
#define IMU_I2C_ADDR                    0x4A
#define PIN_IMU_SDA                     28
#define PIN_IMU_SCL                     29
#define PIN_IMU_INT                     30  // Interrupt pin
#define PIN_IMU_RST                     31  // Reset pin

// -- SD Card (via SDIO) --
// Uses a second PIO for implementing SDIO.
#define PIN_SDIO_CLK                    32
#define PIN_SDIO_CMD                    33
#define PIN_SDIO_D0                     34  // D0-D3 MUST be consecutive. Uses GPIO 34-37
#define PIN_SDIO_DET                    38

// -- NEO-M10 GPS (GPIO Bank 40-41) --
#define UART_PORT                       uart1
#define GPS_UART_BAUD                   38400
#define PIN_GPS_TX                      40 // RP2350 TX -> GPS RX (For UBX commands)
#define PIN_GPS_RX                      41 // RP2350 RX <- GPS TX (For NMEA data)

// -- User Interface (GPIO Bank 42-44) --
// Grouped together for clean User UI routing.
#define PIN_LED_B                       42
#define PIN_LED_G                       43
#define PIN_LED_R                       44
#define PIN_BTN_DRIFT_CORRECT           PIN_PB_1
#define PIN_BTN_LOCATION_TOGGLE         PIN_PB_2


//------------------------------------------------------------------------------
// USER UI
//------------------------------------------------------------------------------

// -- LED Related Definitions --
// Defines the TOP and PSC values
#define LED_PWM_DIV             6
#define LED_PWM_TOP             25500
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
#define LED_VAL_R(hex)          (LED_PWM_TOP / 255 * ((hex >> 16) & 0xFF))
#define LED_VAL_G(hex)          (LED_PWM_TOP / 255 * ((hex >>  8) & 0xFF))
#define LED_VAL_B(hex)          (LED_PWM_TOP / 255 * ((hex >>  0) & 0xFF))


//------------------------------------------------------------------------------
// RANDOM STUFF
//------------------------------------------------------------------------------

#define GP(x) (1 << x)


#endif /* CONFIG_H */