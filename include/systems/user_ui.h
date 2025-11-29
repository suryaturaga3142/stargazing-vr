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


extern StateDetails_t current_state;

extern volatile bool g_drift_correct_request;     // Set by button ISR, handled by main.
extern volatile bool g_location_toggle_request;   // Set by button ISR, handled by main.

bool user_ui_init(void);
bool user_ui_set_state(LEDState_e state);

#ifdef __cplusplus
}
#endif

#endif /* USER_UI_H */