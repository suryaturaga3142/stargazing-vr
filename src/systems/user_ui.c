/*******************************************************************************
 * @file        user_ui.c
 * @brief       Implements the functionality for the User UI.
 * @details     Functions to handle PBs and RGB LEDs.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * 
 * @note        This module is designed to be highly low priority. Execute most
 *              UI related stuff in main since it can potentially and 
 *              unneccessarily block stuff.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "user_ui.h"
#include "config.h"
#include "interrupts.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/time.h"
// ...

/* ---------------------------- Private Constants --------------------------- */

// ...

/* ----------------------------- Private Variables -------------------------- */

StateDetails_t current_state = {
    .state   = LED_STATE_BOOTING,
    .color   = LED_COLOR_OFF,
    .pattern = LED_SOLID,
    .speed   = LED_SPEED_MEDIUM
};

// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Variables --------------------------- */

volatile bool g_drift_correct_request   = false;
volatile bool g_location_toggle_request = true;

/* ----------------------------- Public Functions --------------------------- */

/**
 * @brief Initializes GPIO peripherals and RGB LED stuff
 * 
 * @return true if everything was successful
 */
bool user_ui_init(void)
{
    // Initialize pushbutton GPIOs with interrupts
    gpio_init(PIN_BTN_DRIFT_CORRECT);
    gpio_set_dir(PIN_BTN_DRIFT_CORRECT, GPIO_IN);
    gpio_init(PIN_BTN_LOCATION_TOGGLE);
    gpio_set_dir(PIN_BTN_LOCATION_TOGGLE, GPIO_IN);

    gpio_set_irq_enabled_with_callback(PIN_BTN_DRIFT_CORRECT, GPIO_IRQ_EDGE_RISE, true, irq_gpio_handler);
    gpio_set_irq_enabled(PIN_BTN_LOCATION_TOGGLE, GPIO_IRQ_EDGE_RISE, true);

    // Initialize LED GPIOs just because
    gpio_init(PIN_LED_1);
    gpio_init(PIN_LED_2);
    gpio_init(PIN_LED_3);
    gpio_init(PIN_LED_4);
    gpio_set_dir(PIN_LED_1, GPIO_OUT);
    gpio_set_dir(PIN_LED_2, GPIO_OUT);
    gpio_set_dir(PIN_LED_3, GPIO_OUT);
    gpio_set_dir(PIN_LED_4, GPIO_OUT);

    // Initialize PWM for RGB LED control
    gpio_init(PIN_LED_R);
    gpio_init(PIN_LED_G);
    gpio_init(PIN_LED_B);
    gpio_set_function(PIN_LED_R, GPIO_FUNC_PWM);
    gpio_set_function(PIN_LED_G, GPIO_FUNC_PWM);
    gpio_set_function(PIN_LED_B, GPIO_FUNC_PWM);

    pwm_hw->slice[ pwm_gpio_to_slice_num(PIN_LED_R) ].div = LED_PWM_DIV << PWM_CH0_DIV_INT_LSB;
    pwm_hw->slice[ pwm_gpio_to_slice_num(PIN_LED_G) ].div = LED_PWM_DIV << PWM_CH0_DIV_INT_LSB;
    pwm_hw->slice[ pwm_gpio_to_slice_num(PIN_LED_B) ].div = LED_PWM_DIV << PWM_CH0_DIV_INT_LSB;

    pwm_hw->slice[ pwm_gpio_to_slice_num(PIN_LED_R) ].top = LED_PWM_TOP - 1;
    pwm_hw->slice[ pwm_gpio_to_slice_num(PIN_LED_G) ].top = LED_PWM_TOP - 1;
    pwm_hw->slice[ pwm_gpio_to_slice_num(PIN_LED_B) ].top = LED_PWM_TOP - 1;

    pwm_hw->inte = GP(pwm_gpio_to_slice_num(PIN_LED_R)) |
                   GP(pwm_gpio_to_slice_num(PIN_LED_G)) |
                   GP(pwm_gpio_to_slice_num(PIN_LED_B));

    irq_set_exclusive_handler(PWM_IRQ_WRAP_0, irq_on_pwm_wrap);
    irq_set_enabled(PWM_IRQ_WRAP_0, true);

    pwm_hw->slice[ pwm_gpio_to_slice_num(PIN_LED_R) ].csr |= PWM_CH0_CSR_EN_BITS;
    pwm_hw->slice[ pwm_gpio_to_slice_num(PIN_LED_G) ].csr |= PWM_CH0_CSR_EN_BITS;
    pwm_hw->slice[ pwm_gpio_to_slice_num(PIN_LED_B) ].csr |= PWM_CH0_CSR_EN_BITS;

    user_ui_set_state(LED_STATE_BOOTING);

    return true;
}

/**
 * @brief Function for modules to set the system state
 * 
 * @param state The desired LED state to set
 * @return true if it was set correctly.
 */
bool user_ui_set_state(LEDState_e state)
{
    current_state.state = state;

    switch (state) {
        // --- Normal Operations ---
        case LED_STATE_BOOTING:
            current_state.color   = LED_COLOR_WHITE;
            current_state.pattern = LED_PULSE;
            current_state.speed   = LED_SPEED_MEDIUM;
            break;

        case LED_STATE_SD_LOADING:
            current_state.color   = LED_COLOR_BLUE;
            current_state.pattern = LED_PULSE;
            current_state.speed   = LED_SPEED_MEDIUM;
            break;

        case LED_STATE_GPS_SEARCHING:
            current_state.color   = LED_COLOR_YELLOW;
            current_state.pattern = LED_PULSE;
            current_state.speed   = LED_SPEED_MEDIUM;
            break;

        case LED_STATE_RUN: // GPS Mode
            current_state.color   = LED_COLOR_GREEN;
            current_state.pattern = LED_SOLID;
            current_state.speed   = LED_SPEED_MEDIUM;
            break;

        case LED_STATE_RUN_J2000: // J2000 Mode
            current_state.color   = LED_COLOR_CYAN;
            current_state.pattern = LED_SOLID;
            current_state.speed   = LED_SPEED_MEDIUM;
            break;

        case LED_STATE_TIMELAPSE:
            current_state.color   = LED_COLOR_PURPLE;
            current_state.pattern = LED_PULSE;
            current_state.speed   = LED_SPEED_MEDIUM;
            break;

        // --- Warnings & Non-Critical ---
        case LED_STATE_RUN_NO_FIX: // Warning: No GPS
            current_state.color   = LED_COLOR_RED;
            current_state.pattern = LED_BLINK;
            current_state.speed   = LED_SPEED_SLOW;
            break;

        case LED_STATE_WARN_OVERHEAT:
            current_state.color   = LED_COLOR_ORANGE;
            current_state.pattern = LED_PULSE;
            current_state.speed   = LED_SPEED_MEDIUM;
            break;

        // --- Critical Errors & System Events ---
        case LED_STATE_ERR_CRITICAL:
            current_state.color   = LED_COLOR_RED;
            current_state.pattern = LED_BLINK;
            current_state.speed   = LED_SPEED_FAST;
            break;

        case LED_STATE_REBOOTED: // Watchdog Reset
            // "Single Flash" simulated by Fast Blink. 
            // Application should switch out of this state after a short delay.
            current_state.color   = LED_COLOR_MAGENTA;
            current_state.pattern = LED_BLINK;
            current_state.speed   = LED_SPEED_FAST;
            break;

        case LED_STATE_DRIFT_CONFIRM:
            // "Single Flash" simulated by Fast Blink.
            current_state.color   = LED_COLOR_CYAN;
            current_state.pattern = LED_BLINK;
            current_state.speed   = LED_SPEED_FAST;
            break;

        default:
            // Fallback for undefined behavior: Solid Red
            current_state.color   = LED_COLOR_RED;
            current_state.pattern = LED_SOLID;
            current_state.speed   = LED_SPEED_MEDIUM;
            return false;
    }

    return true;
}