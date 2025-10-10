; PIO Assembly Program: led_chaser
;
; Goal: Control 4 LEDs connected to consecutive GPIO pins.
;
; How it works:
; 1. Waits for a 32-bit word to be pushed from the CPU to the TX FIFO.
; 2. Pulls this word into its Output Shift Register (OSR).
; 3. Outputs the lower 4 bits of the OSR to the 'out' pins in parallel.
; 4. Delays for a period to make the LED state visible.
; 5. Wraps around to wait for the next value.

.program led_chaser

.wrap_target
    pull block          ; Wait for a new 32-bit value from the TX FIFO and load into OSR.
    out pins, 4         ; Output the lower 4 bits from OSR to the 'out' pins.
    nop [31]            ; Delay for 32 cycles to make the change visible.
                        ; The number of cycles can be adjusted here or by the clock divider in C.
.wrap
