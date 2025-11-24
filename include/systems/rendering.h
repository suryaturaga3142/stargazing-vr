/*******************************************************************************
 * @file        rendering.h
 * @brief       Declarations related to all 3D rendering of stars.
 * @details     Library for tasks such as spatial culling and pixel mapping.
 * 
 * @see         rendering.c for implementation details.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * @version     1.0
 ******************************************************************************/

#ifndef RENDERING_H
#define RENDERING_H

#include <stdbool.h>
#include "config.h"
#include "structs.h"

extern volatile bool g_is_rendering;              // Prevents render ISR overruns.

#endif /* RENDERING_H */