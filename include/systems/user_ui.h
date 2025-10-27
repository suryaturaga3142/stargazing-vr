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

#include "config.h"

// -- LED RGB Isolation Macros --
#define LED_VAL_R(hex)          (hex >> 16) & 0xFF
#define LED_VAL_G(hex)          (hex >>  8) & 0xFF
#define LED_VAL_B(hex)          (hex >>  0) & 0xFF

#endif /* USER_UI_H */