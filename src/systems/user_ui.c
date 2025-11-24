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

