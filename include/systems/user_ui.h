/*******************************************************************************
 * @file        user_ui.h
 * @brief       Library for all UI experience with user of headset.
 * @details     Handles button presses, RGB LED indication, etc.
 * 
 * @see         user_ui.c for implementation details.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * @version     1.0
 ******************************************************************************/

#ifndef USER_UI_H
#define USER_UI_H

#include <stdbool.h>
#include "structs.h"

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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief A globally accessed enum to control the state
 * @details All modules can access to change RGB LED
 */
typedef enum {
    LED_STATE_READY
} LEDState_e;

extern volatile bool g_drift_correct_request;     // Set by button ISR, handled by main.
extern volatile bool g_toggle_mode_request;       // Set by button ISR, handled by main.

bool user_ui_init(void);
bool user_ui_set_state(LEDState_e state);

#ifdef __cplusplus
}
#endif

#endif /* USER_UI_H */