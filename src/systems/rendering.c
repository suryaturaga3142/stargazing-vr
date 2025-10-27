/*******************************************************************************
 * @file        rendering.c
 * @brief       Implements the functionality for the rendering module.
 * @details     Works in tandem with results of mechanics.h functions. The 
 *              rotations in mechanics allows for a single quaternion 
 *              calculation. This will be used here to remap the relevant
 *              stars through spatial culling, map them onto the screen,
 *              and formulating the command buffer for display.c to access.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * 
 * @note        This module is designed to give help with pixel mapping. Call
 *              only during pixel projection in interrupts.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "globals.h"
#include "rendering.h"
#include <stdint.h>
// ...

/* ---------------------------- Private Constants --------------------------- */
#define ILI9486_CMD_SLEEP_OUT  0x11
#define ILI9486_CMD_DISPLAY_ON 0x29
#define ILI9486_CMD_CASET      0x2A
#define ILI9486_CMD_PASET      0x2B
#define ILI9486_CMD_MEMWRITE   0x2C
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */

/**
 * @brief Formats a 16-bit payload and D/C state into a 32-bit word for the PIO FIFO.
 * @param dc_state 0 for Command, 1 for Data.
 * @param payload The 16-bit command or pixel data.
 * @return The formatted 32-bit word ready for DMA transfer.
 */
static inline uint32_t _format_pio_word(uint8_t dc_state, uint16_t payload) {
    return (((uint32_t)dc_state & 0x01) << 16) | payload;
}

/**
 * @brief Appends the PIO words for a CASET command to the draw list.
 * @param buffer Pointer to the current position in the draw_list_buffer.
 * @param start_col Starting column.
 * @param end_col Ending column.
 * @return Updated pointer after adding words.
 */
static uint32_t* _append_caset_command(uint32_t* buffer, uint16_t start_col, uint16_t end_col) {
    *buffer++ = _format_pio_word(0, 0x002A); // CASET Command
    *buffer++ = _format_pio_word(1, (start_col >> 8) & 0xFF);
    *buffer++ = _format_pio_word(1, start_col & 0xFF);
    *buffer++ = _format_pio_word(1, (end_col >> 8) & 0xFF);
    *buffer++ = _format_pio_word(1, end_col & 0xFF);
    return buffer;
}

// ... similar helper functions for PASET, MEMWRITE, and sending pixel data ...

// ...

/* ----------------------------- Public Functions --------------------------- */

