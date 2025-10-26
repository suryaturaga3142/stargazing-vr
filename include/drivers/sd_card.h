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

#include <stdint.h>

/**
 * @brief Initializes the SD card in 4-bit SDIO mode.
 * @details Uses pin and PIO settings from config.h.
 * @return 0 on success, negative error code on failure.
 */
int sd_init(void);

/**
 * @brief Reads one or more contiguous blocks from the SD card.
 *
 * @param sector The starting block (LBA) to read from.
 * @param buffer Pointer to the destination buffer (must be 32-bit aligned).
 * @param num_sectors The number of blocks (512 bytes each) to read.
 * @return 0 on success, negative error code on failure.
 */
int sd_read_blocks(uint32_t sector, uint8_t *buffer, uint32_t num_sectors);

#endif /* SD_CARD_H */