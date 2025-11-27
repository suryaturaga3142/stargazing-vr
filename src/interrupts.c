/*******************************************************************************
 * @file        interrupts.c
 * @brief       Implements the functionality for the interrrupts.
 * @details     All irq general handlers.
 * 
 * @author      LED Chasers
 * @date        2025-11-24
 * 
 * @note        This module is designed to be driven by interrupts and is not
 *              intended to be called from a blocking main loop.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "config.h"
#include "interrupts.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#include "user_ui.h"
#include "rendering.h"

// ...

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
static volatile LEDState_e led_running_state = LED_STATE_BOOTING;
static volatile uint32_t led_pattern_counter = 0;
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Variables -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */

/**
 * @brief Generic GPIO IRQ Handler function.
 * 
 * @param gpio Pin that caused the interrupt.
 * @param events Event mask that triggered the interrupt.
 */
void irq_gpio_handler(uint gpio, uint32_t events)
{
    if (gpio == PIN_IMU_INT) {
        gpio_acknowledge_irq(PIN_IMU_INT, events);
        run_main_render();
        return;
    }
    if (gpio == PIN_BTN_DRIFT_CORRECT) {
        gpio_acknowledge_irq(PIN_BTN_DRIFT_CORRECT, events);
        g_drift_correct_request = true;
    } else if (gpio == PIN_BTN_LOCATION_TOGGLE) {
        gpio_acknowledge_irq(PIN_BTN_LOCATION_TOGGLE, events);
        g_use_actual_gps = !g_use_actual_gps;
    }
    return;
}

/**
 * @brief Handler for User UI RGB LED state handler on wrap
 */
void irq_on_pwm_wrap(void) {
    // Acknowledge the interrupt
    pwm_clear_irq(pwm_gpio_to_slice_num(PIN_LED_R));
    pwm_clear_irq(pwm_gpio_to_slice_num(PIN_LED_G));
    pwm_clear_irq(pwm_gpio_to_slice_num(PIN_LED_B));

    if (led_running_state != current_state.state) {
        led_running_state = current_state.state;

        uint16_t cc_r = 0;
        uint16_t cc_g = 0;
        uint16_t cc_b = 0;

        // Set color based on current_state.color
        // Dont use patter and speed for now
        switch (current_state.color)
        {
        case LED_COLOR_OFF:
            break;
        case LED_COLOR_WHITE:
            cc_r = LED_VAL_R(LED_HEX_WHITE);
            cc_g = LED_VAL_G(LED_HEX_WHITE);
            cc_b = LED_VAL_B(LED_HEX_WHITE);
            break;
        case LED_COLOR_BLUE:
            cc_r = LED_VAL_R(LED_HEX_BLUE);
            cc_g = LED_VAL_G(LED_HEX_BLUE);
            cc_b = LED_VAL_B(LED_HEX_BLUE);
            break;
        case LED_COLOR_YELLOW:
            cc_r = LED_VAL_R(LED_HEX_YELLOW);
            cc_g = LED_VAL_G(LED_HEX_YELLOW);
            cc_b = LED_VAL_B(LED_HEX_YELLOW);   
            break;
        case LED_COLOR_GREEN:
            cc_r = LED_VAL_R(LED_HEX_GREEN);
            cc_g = LED_VAL_G(LED_HEX_GREEN);
            cc_b = LED_VAL_B(LED_HEX_GREEN);
            break;
        case LED_COLOR_CYAN:
            cc_r = LED_VAL_R(LED_HEX_CYAN);
            cc_g = LED_VAL_G(LED_HEX_CYAN);
            cc_b = LED_VAL_B(LED_HEX_CYAN);
            break;
        case LED_COLOR_RED:
            cc_r = LED_VAL_R(LED_HEX_RED);
            cc_g = LED_VAL_G(LED_HEX_RED);
            cc_b = LED_VAL_B(LED_HEX_RED);
            break;
        case LED_COLOR_ORANGE:
            cc_r = LED_VAL_R(LED_HEX_ORANGE);
            cc_g = LED_VAL_G(LED_HEX_ORANGE);
            cc_b = LED_VAL_B(LED_HEX_ORANGE);
            break;
        case LED_COLOR_MAGENTA:
            cc_r = LED_VAL_R(LED_HEX_MAGENTA);
            cc_g = LED_VAL_G(LED_HEX_MAGENTA);
            cc_b = LED_VAL_B(LED_HEX_MAGENTA);
            break;
        case LED_COLOR_PURPLE:
            cc_r = LED_VAL_R(LED_HEX_PURPLE);
            cc_g = LED_VAL_G(LED_HEX_PURPLE);
            cc_b = LED_VAL_B(LED_HEX_PURPLE);
            break;
        default:
            break;
        }

        pwm_set_gpio_level(PIN_LED_R, cc_r);
        pwm_set_gpio_level(PIN_LED_G, cc_g);
        pwm_set_gpio_level(PIN_LED_B, cc_b);
    }
        
}