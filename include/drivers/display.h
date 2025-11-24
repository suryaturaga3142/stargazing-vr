/*******************************************************************************
 * @file        display.h
 * @brief       Driver for LCD Displays
 * @details     A very high level C file for managing only the LCD and PIO
 *              initialization.
 * 
 * @see         display.c for implementation details.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * @version     1.0
 ******************************************************************************/

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include "structs.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

bool display_init(void);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H */