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
#include "monitor.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "pico/time.h"

#include "imu.h"
#include "user_ui.h"
#include "rendering.h"

// ...

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
static StateDetails_t led_cache = {-1, -1, -1, -1};
static uint32_t counter_ms = 0;
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
        g_imu_data_ready = true; // Set flag for main to read IMU data
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
 * 
 */
void irq_on_pwm_wrap(void) {
    // Acknowledge the interrupt
    pwm_clear_irq(pwm_gpio_to_slice_num(PIN_LED_R));
    pwm_clear_irq(pwm_gpio_to_slice_num(PIN_LED_G));
    pwm_clear_irq(pwm_gpio_to_slice_num(PIN_LED_B));

    // 2. Detect Configuration Change
    // We must check if Color, Pattern, OR Speed changed to reset the counter.
    // We copy the volatile global to a local var for safe reading.
    StateDetails_t local_target = current_state; 

    bool config_changed = (local_target.state != led_cache.state) ||
                          (local_target.pattern != led_cache.pattern) ||
                          (local_target.speed != led_cache.speed) ||
                          (local_target.color != led_cache.color);

    if (config_changed) {
        // Update cache
        led_cache.state = local_target.state;
        led_cache.pattern = local_target.pattern;
        led_cache.speed = local_target.speed;
        led_cache.color = local_target.color;
        
        // RESET counter so the new pattern starts from the beginning (t=0)
        counter_ms = 0; 
    }

    // 3. Determine Period (Total duration of one cycle)
    uint32_t period = LED_PERIOD_MEDIUM_MS;
    switch(local_target.speed) {
        case LED_SPEED_SLOW:   period = LED_PERIOD_SLOW_MS;   break;
        case LED_SPEED_FAST:   period = LED_PERIOD_FAST_MS;   break;
        case LED_SPEED_MEDIUM: default: period = LED_PERIOD_MEDIUM_MS; break;
    }

    // 4. Calculate Brightness Scalar (0.0 to 1.0)
    // This abstract number represents "How bright should we be at this millisecond?"
    float scalar = 1.0f;
    
    // Increment Time
    counter_ms++;
    if (counter_ms >= period) {
        counter_ms = 0;
    }

    if (local_target.pattern == LED_SOLID) {
        scalar = 1.0f;
    }
    else if (local_target.pattern == LED_BLINK) {
        // Square Wave: ON for first half, OFF for second half
        if (counter_ms < (period / 2)) {
            scalar = 1.0f;
        } else {
            scalar = 0.0f;
        }
    }
    else if (local_target.pattern == LED_PULSE) {
        // Triangle Wave: Linear Up, Linear Down
        // We use float math here for clarity. 
        float position = (float)counter_ms;
        float half_period = (float)period / 2.0f;

        if (position < half_period) {
            // Ramping Up (0.0 -> 1.0)
            scalar = position / half_period;
        } else {
            // Ramping Down (1.0 -> 0.0)
            // Calculate how far into the second half we are
            float offset = position - half_period;
            scalar = 1.0f - (offset / half_period);
        }
    }

    // 5. Apply Color with Scalar
    // Get the max brightness values for the requested color
    uint32_t target_hex = LED_HEX_OFF;
    switch (local_target.color) {
        case LED_COLOR_WHITE:   target_hex = LED_HEX_WHITE; break;
        case LED_COLOR_BLUE:    target_hex = LED_HEX_BLUE; break;
        case LED_COLOR_GREEN:   target_hex = LED_HEX_GREEN; break;
        case LED_COLOR_RED:     target_hex = LED_HEX_RED; break;
        case LED_COLOR_YELLOW:  target_hex = LED_HEX_YELLOW; break;
        case LED_COLOR_CYAN:    target_hex = LED_HEX_CYAN; break;
        case LED_COLOR_MAGENTA: target_hex = LED_HEX_MAGENTA; break;
        case LED_COLOR_PURPLE:  target_hex = LED_HEX_PURPLE; break;
        case LED_COLOR_ORANGE:  target_hex = LED_HEX_ORANGE; break;
        default:                target_hex = LED_HEX_OFF; break;
    }

    // Apply the scaler to the raw PWM values
    uint16_t final_r = (uint16_t)(LED_VAL_R(target_hex) * scalar);
    uint16_t final_g = (uint16_t)(LED_VAL_G(target_hex) * scalar);
    uint16_t final_b = (uint16_t)(LED_VAL_B(target_hex) * scalar);

    // 6. Write to Hardware (Note the inversion)
    pwm_set_gpio_level(PIN_LED_R, LED_PWM_TOP - final_r);
    pwm_set_gpio_level(PIN_LED_G, LED_PWM_TOP - final_g);
    pwm_set_gpio_level(PIN_LED_B, LED_PWM_TOP - final_b);

    return;
}

/**
 * @brief IRQ for a repeating timer to check in health of all systems
 * 
 */
void irq_timer_monitor_callback(void) {
    timer0_hw->intr &= TIMER_INTR_ALARM_0_BITS;
    monitor_update();
    timer0_hw->alarm[0] = timer_hw->timerawl + (WATCHDOG_SUPERVISOR_INTERVAL_MS * 1000);
    return;
}
