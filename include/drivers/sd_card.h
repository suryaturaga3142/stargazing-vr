/*******************************************************************************
 * @file        sd_card.h
 * @brief       Driver library header for SD Card reading.
 * @details     This library is meant to declare functions and locals for SD 
 *              Card usage through SDIO interface.
 * 
 * @see         sd_card.c for implementation details.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * @version     1.0
 ******************************************************************************/

#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdbool.h>
#include "structs.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif
    
bool sd_check(void);
bool sd_init(void);
bool sd_load_data(void);
bool sd_buf_sort(void);
bool sd_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* SD_CARD_H */