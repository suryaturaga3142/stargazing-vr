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

#include "user_ui.h"
#include "rendering.h"

// ...

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
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

}