/*******************************************************************************
 * @file        gps.c
 * @brief       Implements the complete functionality for the GPS module.
 * @details     Coding the driver for the NEO M10 GPS. Uses UART blocking for 
 *              sending UBX commands and receiving NMEA sentences. 
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * 
 * @note        This module is designed to be taken care of by interrupts or
 *              main. Use main for long blocking functions.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "gps.h"
#include "minmea.h" // Assumes minmea.h is in the src/ dir or include path

// We get all hardware defs from your config.h
#include "config.h" 

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include <string.h> // For strcpy
// ...

/* ---------------------------- Private Constants --------------------------- */
// Buffer to store incoming NMEA sentences
#define LINE_BUFFER_LENGTH 256
// ...

/* ----------------------------- Private Variables -------------------------- */
static char line_buffer[LINE_BUFFER_LENGTH];
static volatile uint16_t line_buffer_index = 0;
static volatile bool line_ready = false;

// Internal struct to hold the latest data.
// Initialized to zero/invalid.
static gps_data_t current_gps_data = {0};
// ...

/* ----------------------------- Private Functions -------------------------- */
/**
 * @brief UART RX interrupt handler.
 * Reads characters into the line_buffer and sets line_ready flag
 * when a complete line ('\n' or '\r') is received.
 */
static void on_uart_rx() {
    while (uart_is_readable(UART_PORT)) {
        char ch = uart_getc(UART_PORT);

        if (line_ready) {
            // Main loop hasn't processed last line yet.
            // Drop this character to prevent buffer overwrite.
            continue; 
        }

        if (ch == '\n' || ch == '\r') {
            if (line_buffer_index > 0) {
                // End of line, mark for processing
                line_buffer[line_buffer_index] = '\0';
                line_ready = true;
                line_buffer_index = 0; // Reset for next line
            }
            // else: ignore empty lines
        } else if (line_buffer_index < (LINE_BUFFER_LENGTH - 1)) {
            // Add character to buffer
            line_buffer[line_buffer_index++] = ch;
        }
        // else: buffer overflow, character is dropped
    }
}

/**
 * @brief Parses a complete NMEA sentence and updates the internal data struct.
 * @param line The NMEA sentence string.
 * @return true if a useful (RMC or GGA) frame was parsed, false otherwise.
 */
static bool parse_line(const char *line) {
    bool new_data_parsed = false;
    switch (minmea_sentence_id(line, false)) {
        case MINMEA_SENTENCE_RMC: {
            struct minmea_sentence_rmc frame;
            if (minmea_parse_rmc(&frame, line)) {
                current_gps_data.fix_valid = frame.valid;
                if (frame.valid) {
                    current_gps_data.latitude = minmea_tocoord(&frame.latitude);
                    current_gps_data.longitude = minmea_tocoord(&frame.longitude);
                    current_gps_data.speed = minmea_tofloat(&frame.speed);
                    new_data_parsed = true;
                }
            }
            break;
        }

        case MINMEA_SENTENCE_GGA: {
            struct minmea_sentence_gga frame;
            if (minmea_parse_gga(&frame, line)) {
                if (frame.fix_quality > 0) {
                    // This frame has a valid fix
                    current_gps_data.fix_valid = true; 
                    current_gps_data.latitude = minmea_tocoord(&frame.latitude);
                    current_gps_data.longitude = minmea_tocoord(&frame.longitude);
                    current_gps_data.satellites_tracked = frame.satellites_tracked;
                    current_gps_data.altitude = minmea_tofloat(&frame.altitude);
                    new_data_parsed = true;
                } else {
                    // This frame reports no fix
                    current_gps_data.fix_valid = false;
                }
            }
            break;
        }
        default:
            break; // Ignore other sentences
    }
    return new_data_parsed;
}
// ...

/* ----------------------------- Public Functions --------------------------- */
void gps_init(void) {
    // Initialize UART using defines from config.h
    uart_init(UART_PORT, GPS_UART_BAUD);

    // Set the TX and RX pins
    gpio_set_function(PIN_GPS_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_GPS_RX, GPIO_FUNC_UART);

    // Set up and enable the UART RX interrupt.
    int UART_IRQ = (UART_PORT == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(UART_PORT, true, false); // Enable RX interrupt, disable TX
}

bool gps_update(void) {
    if (!line_ready) {
        return false; // No new line to process
    }

    // Create a local copy of the line to process.
    // This minimizes time with interrupts disabled.
    char line_to_process[LINE_BUFFER_LENGTH];
    
    // Disable interrupts briefly to safely copy and clear flag
    uint32_t irq_status = save_and_disable_interrupts();
    strcpy(line_to_process, line_buffer);
    line_ready = false;
    restore_interrupts(irq_status);

    // Process the line outside the critical section
    if (minmea_check(line_to_process, false)) {
        return parse_line(line_to_process);
    }
    
    return false;
}

gps_data_t gps_get_data(void) {
    // Return a copy of the internal data.
    // For a safety-critical system, you might wrap this in an
    // interrupt disable/enable block, but for printing it's fine.
    return current_gps_data;
}
