;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; @file       ili9486_pio.s
; @brief      PIO assembly program to drive an ILI9486 display via a 16-bit 8080 parallel interface.
; @author     LED Chasers
; @date       2025-10-14
; @version    1.0
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; DETAILED DESCRIPTION:
; =====================
; This program implements a high-speed, 16-bit 8080 parallel bus master. It is
; specifically timed to meet the requirements of the ILI9486 display controller.
; The program is designed to be fed a continuous stream of 32-bit words from
; the DMA controller, offloading all data transfer from the CPU.
;
;
; HEADSET INTERACTION:
; ====================
; This file is the lowest-level hardware driver for the VR headset's dual LCDs.
; It directly generates the electrical signals that send commands and pixel
; data to the screens.
;
;
; DEPENDENCIES:
; =============
;   - DEPENDS ON:
;     - The DMA controller to continuously feed its TX FIFO.
;     - The C code in `display.c` to correctly initialize the PIO state machine,
;       configure the pin mappings, and set the clock speed.
;
;   - REQUIRED BY:
;     - The `pioasm` build tool, which compiles this file into `ili9486_pio.pio.h`.
;     - `display.c`, which includes the auto-generated header to load this program.
;
;
; PIO PROGRAM STRUCTURE:
; ======================
;   - PINS:
;     - `out` pins (16): Mapped to the D0-D15 data bus.
;     - `side_set` pins (2): Mapped to the D/C and /WR control lines.
;
;   - DATA FORMAT:
;     - Expects 32-bit words from the TX FIFO.
;     - Bit 16 is used for the D/C side-set pin (1=Data, 0=Command).
;     - Bits 15-0 are the 16-bit data/command payload for the `out` instruction.
;
;   - TIMING:
;     - The loop is timed to respect the ILI9486's minimum write cycle (~66ns).
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.program ili9486_pio
