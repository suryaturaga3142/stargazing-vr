/*******************************************************************************
 * @file        interrupts.h
 * @brief       Definitions for all interrupt handlers
 * @details     All the gpio and timer interrupts go in this file. These are 
 *              just the handlers! It's recommended they call their own functions 
 *              living in other files after determining interrupt source.
 * 
 * @see         interrupts.c for implementation details.
 * 
 * @author      LED Chasers
 * @date        2025-11-24
 * @version     1.0
 ******************************************************************************/

#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif





#ifdef __cplusplus
}
#endif


#endif /* INTERRUPTS_H */