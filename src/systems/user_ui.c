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
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/time.h"
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Variables --------------------------- */
volatile bool g_drift_correct_request = false;
volatile bool g_toggle_mode_request = false;

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

    gpio_set_irq_enabled_with_callback(PIN_BTN_DRIFT_CORRECT, GPIO_IRQ_EDGE_RISE, true, NULL);
    gpio_set_irq_enabled_with_callback(PIN_BTN_LOCATION_TOGGLE, GPIO_IRQ_EDGE_RISE, true, NULL);

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
    // Set initial LED state (e.g., off)
    return true;
}
