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

// --- Star Catalog Data Structures ---
// These large arrays hold the pre-processed star data.
extern Star_t all_stars[STAR_CATALOG_SIZE_MAX];
extern SkyPatch_t sky_database[SKY_PATCH_RA_DIVISIONS][SKY_PATCH_DEC_DIVISIONS];


extern Qfix_t Qfix_last;

#ifdef __cplusplus
extern "C" {
#endif

bool run_main_render(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_H */