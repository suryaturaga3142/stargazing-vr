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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief A globally accessed enum to control the state
 * @details All modules can access to change RGB LED
 */
typedef enum {
    LED_STATE_BOOTING,
    LED_STATE_REBOOTED,
    LED_STATE_SD_LOADING,
    LED_STATE_GPS_SEARCHING,
    LED_STATE_ERR_CRITICAL,
    LED_STATE_ERR_GENERIC,
    LED_STATE_RUN_NO_FIX,
    LED_STATE_RUN
} LEDState_e;

extern volatile bool g_drift_correct_request;     // Set by button ISR, handled by main.
extern volatile bool g_use_actual_gps;            // Set by button ISR, handled by main.

bool user_ui_init(void);
bool user_ui_set_state(LEDState_e state);

#ifdef __cplusplus
}
#endif

#endif /* USER_UI_H */