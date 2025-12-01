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
#include "config.h"
#include "globals.h"
#include "minmea.h"

#include "pico/stdlib.h"
#include "pico/critical_section.h"
#include "pico/binary_info.h"
#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include <string.h> // For strcpy
#include <stdio.h>  // For printf
#include <stdlib.h>
// ...

//#define C3
#define CN


#ifdef C3

/* ---------------------------- Private Constants --------------------------- */
// Define which I2C port we are using for the GPS, as defined by the
// RP2350 datasheet for GPIO 38/39.
#define GPS_I2C_PORT i2c1

// The u-blox default 7-bit I2C address
#define GPS_I2C_ADDR 0x42

// Registers for reading data (from u-blox M10 datasheet)
#define REG_BYTES_AVAIL_HIGH 0xFD
#define REG_BYTES_AVAIL_LOW  0xFE
#define REG_DATA_STREAM      0xFF

// Buffer to store incoming NMEA sentences
#define LINE_BUFFER_LENGTH 256

/* ----------------------------- Private Variables -------------------------- */
// Persistent buffer to hold partial NMEA sentences as they are streamed
static char line_buffer[LINE_BUFFER_LENGTH];
static uint16_t line_buffer_index = 0;

/* ----------------------------- Private Functions -------------------------- */

/**
 * @brief Parses a complete NMEA sentence and updates the g_latest_gps_data global.
 * @param line The NMEA sentence string.
 */
static void parse_line(const char *line) {
    // --- PRINT RAW SENTENCE ---
    // This is the requested output for the sandbox test.
    printf("RAW: %s\n", line);
    // --------------------------

    // Update the global struct based on the sentence type
    switch (minmea_sentence_id(line, false)) {
        case MINMEA_SENTENCE_RMC: {
            struct minmea_sentence_rmc frame;
            if (minmea_parse_rmc(&frame, line)) {
                g_latest_gps_data.is_valid = frame.valid;
                if (frame.valid) {
                    g_latest_gps_data.latitude = minmea_tofloat(&frame.latitude);
                    g_latest_gps_data.longitude = minmea_tofloat(&frame.longitude);
                    g_latest_gps_data.time.year   = frame.date.year + 2000; // minmea returns 2-digit year
                    g_latest_gps_data.time.month  = frame.date.month;
                    g_latest_gps_data.time.day    = frame.date.day;
                    g_latest_gps_data.time.hour   = frame.time.hours;
                    g_latest_gps_data.time.minute = frame.time.minutes;
                    g_latest_gps_data.time.second = frame.time.seconds;
                }
            }
            break;
        }
        case MINMEA_SENTENCE_GGA: {
            struct minmea_sentence_gga frame;
            if (minmea_parse_gga(&frame, line)) {
                if (frame.fix_quality > 0) {
                    g_latest_gps_data.is_valid = true; 
                    g_latest_gps_data.latitude = minmea_tofloat(&frame.latitude);
                    g_latest_gps_data.longitude = minmea_tofloat(&frame.longitude);
                } else {
                    g_latest_gps_data.is_valid = false;
                }
            }
            break;
        }
        default:
            break; // Ignore other sentences (GSV, GSA, etc.)
    }
}

/**
 * @brief Processes a chunk of raw byte data read from the I2C bus.
 * @details This function is a state-machine that builds NMEA sentences
 * (which end in '\n' or '\r') from the stream of bytes.
 */
static void process_data_chunk(uint8_t* data, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        char ch = data[i];

        // A $ indicates a new sentence, reset the buffer
        if (ch == '$') {
            line_buffer_index = 0;
            line_buffer[line_buffer_index++] = ch;
        } 
        // A \n or \r indicates the end of a sentence
        else if (ch == '\n' || ch == '\r') {
            if (line_buffer_index > 0) {
                // We have a complete sentence
                line_buffer[line_buffer_index] = '\0';
                
                // Check checksum and, if valid, parse it
                if (minmea_check(line_buffer, false)) {
                    parse_line(line_buffer);
                }
                line_buffer_index = 0; // Reset for next line
            }
            // else: ignore empty newlines
        }
        // Otherwise, add the character to the buffer
        else if (line_buffer_index < (LINE_BUFFER_LENGTH - 1)) {
            // Only add if we have started buffering (seen a '$')
            if (line_buffer_index > 0) {
                line_buffer[line_buffer_index++] = ch;
            }
        }
        // else: buffer overflow, reset and wait for next '$'
        else {
            line_buffer_index = 0;
        }
    }
}

/* ----------------------------- Public Functions --------------------------- */

/**
 * @brief Initializes the I2C bus (i2c1) for communication with the GPS.
 */
void gps_init(void) {
    // Initialize i2c1 at 100kHz (standard I2C speed)
    i2c_init(GPS_I2C_PORT, 100 * 1000);
    
    // Use the I2C pins defined in config.h
    // PIN_GPS_TX (38) is SDA
    // PIN_GPS_RX (39) is SCL
    gpio_set_function(PIN_GPS_TX, GPIO_FUNC_I2C); // Pin 38
    gpio_set_function(PIN_GPS_RX, GPIO_FUNC_I2C); // Pin 39
    
    // Enable pull-ups (standard I2C practice)
    gpio_pull_up(PIN_GPS_TX);
    gpio_pull_up(PIN_GPS_RX);

    // Add binary info for the debugger
    bi_decl(bi_2pins_with_func(PIN_GPS_TX, PIN_GPS_RX, GPIO_FUNC_I2C));

    // NOTE: The NEO-M10 defaults to NMEA output over I2C.
    // No UBX configuration messages are required for this basic test.
    line_buffer_index = 0;
    memset(line_buffer, 0, LINE_BUFFER_LENGTH);
}

/**
 * @brief Polls the GPS module over I2C for new NMEA sentences.
 */
void gps_update(void) {
    uint8_t reg_addr;
    uint8_t avail_bytes_buf[2];
    uint16_t bytes_available = 0;

    // 1. Check how many bytes are available to read.
    // We read from register 0xFD to get this info.
    reg_addr = REG_BYTES_AVAIL_HIGH;
    int ret = i2c_write_blocking(GPS_I2C_PORT, GPS_I2C_ADDR, &reg_addr, 1, true); // true = "no stop"
    if (ret < 0) {
        // This is the most common error: wiring is wrong or module is not found.
        printf("I2C Read Error: No device found at 0x%02X on i2c1.\n", GPS_I2C_ADDR);
        sleep_ms(1000);
        return; // No device responded
    }
    
    // Read the two bytes (high and low) that tell us the number of bytes available
    ret = i2c_read_blocking(GPS_I2C_PORT, GPS_I2C_ADDR, avail_bytes_buf, 2, false); // false = "stop"
    if (ret < 0) {
        printf("I2C Read Error: Failed to read byte count.\n");
        return; // Read error
    }

    // Combine the two 8-bit values into one 16-bit value
    bytes_available = ((uint16_t)avail_bytes_buf[0] << 8) | avail_bytes_buf[1];

    if (bytes_available > 0) {
        // 2. Read the available data from the 0xFF stream register
        
        // Limit read to a manageable chunk size to avoid large stack allocation
        if (bytes_available > 255) {
            bytes_available = 255;
        }
        
        uint8_t data_buf[bytes_available];
        
        reg_addr = REG_DATA_STREAM;
        i2c_write_blocking(GPS_I2C_PORT, GPS_I2C_ADDR, &reg_addr, 1, true); // true = "no stop"
        ret = i2c_read_blocking(GPS_I2C_PORT, GPS_I2C_ADDR, data_buf, bytes_available, false);
        
        if (ret > 0) {
            // 3. Process the chunk of data we just read
            process_data_chunk(data_buf, ret);
        }
    }
    // else: no data to read this cycle
}


#endif

#ifdef CN
/* ---------------------------- Private Constants --------------------------- */
// Buffer to store incoming NMEA sentences from the ISR
#define LINE_BUFFER_LENGTH 256
// ...

/* ----------------------------- Private Variables -------------------------- */
static char line_buffer[LINE_BUFFER_LENGTH];
static volatile uint16_t line_buffer_index = 0;
static volatile bool line_ready = false;
static volatile bool buffering_active = false; // For robust ISR
// ...

/* ----------------------------- Private Functions -------------------------- */
/**
 * @brief UART RX interrupt handler. (Robust version)
 *
 * This ISR waits for a '$' to begin buffering a sentence.
 * It ignores all other characters, preventing garbage/empty lines
 * from being processed.
 */
static void on_uart_rx() {
    while (uart_is_readable(UART_PORT)) {
        char ch = uart_getc(UART_PORT);
        
        if (ch == '$') {
            // Start of a new NMEA sentence
            line_buffer_index = 0;
            line_buffer[line_buffer_index++] = ch;
            buffering_active = true;
            line_ready = false; // Discard any previous partial line
        } 
        else if (buffering_active) 
        {
            // We are inside a sentence, keep buffering
            if (line_ready) {
                // Main loop hasn't processed last line yet.
                // A new '$' will reset this, but until then, drop chars.
                continue;
            }
            
            if (ch == '\n' || ch == '\r') {
                // End of line, mark for processing
                if (line_buffer_index > 0) { // Should always be > 0 since we started with '$'
                    line_buffer[line_buffer_index] = '\0';
                    line_ready = true;
                    buffering_active = false;
                }
                // else: buffer was reset by '$' (e.g., "$...$"), ignore this newline
            } 
            else if (line_buffer_index < (LINE_BUFFER_LENGTH - 1)) {
                // Add character to buffer
                line_buffer[line_buffer_index++] = ch;
            } 
            else {
                // Buffer overflow, sentence is too long.
                // Discard this line and wait for a new '$'.
                buffering_active = false;
                line_buffer_index = 0;
            }
        }
        // else: (buffering_active is false and ch != '$')
        //       We are waiting for a '$', so ignore this character.
    }
}

/**
 * @brief Parses a complete NMEA sentence and updates the g_latest_gps_data global.
 * @param line The NMEA sentence string.
 */
static void parse_line(const char *line) 
{
    if (!line || line[0] != '$') 
        return;

    // --- Step 1: Compute and optionally check checksum ---
    unsigned char cs = 0;
    const char *p = line + 1; // skip '$'
    while (*p && *p != '*') {
        cs ^= (unsigned char)(*p++);
    }

    const char *asterisk = strchr(line, '*');
    if (asterisk && strlen(asterisk) >= 3) {
        unsigned int provided_cs;
        if (sscanf(asterisk + 1, "%2x", &provided_cs) == 1) {
            if (cs != (unsigned char)provided_cs) {
                printf("Checksum mismatch: computed %02X vs provided %02X\n", cs, provided_cs);
                return;
            }
        }
    }

    // --- Step 2: Only parse RMC sentences ---
    if (minmea_sentence_id(line, true) != MINMEA_SENTENCE_RMC)
        return;

    struct minmea_sentence_rmc frame;

    // --- Step 3: Try parsing with minmea first ---
    if (minmea_parse_rmc(&frame, line)) 
    {
        bool parsed_valid = frame.valid;

        // --- Step 4: Also check raw status field in case of extra fields ---
        const char *p = strchr(line, ','); // after $GxRMC
        if (p) {
            p++; // skip time field
            const char *q = strchr(p, ','); // end of time
            if (q) {
                const char *status_field = q + 1;
                if (status_field && (*status_field == 'A' || *status_field == 'V')) {
                    parsed_valid = (*status_field == 'A');
                }
            }
        }

        // --- Step 5: Update global GPS structure ---
        g_latest_gps_data.is_valid = parsed_valid;
        if (parsed_valid) {
            g_latest_gps_data.latitude  = minmea_tofloat(&frame.latitude);
            g_latest_gps_data.longitude = minmea_tofloat(&frame.longitude);

            g_latest_gps_data.time.year   = frame.date.year + 2000;
            g_latest_gps_data.time.month  = frame.date.month;
            g_latest_gps_data.time.day    = frame.date.day;
            g_latest_gps_data.time.hour   = frame.time.hours;
            g_latest_gps_data.time.minute = frame.time.minutes;
            g_latest_gps_data.time.second = frame.time.seconds;
        }
    } 
    else
    {
        // --- Step 6: Fallback manual parsing for non-standard extra fields ---
        // Split sentence manually and read first 10-12 fields
        char copy[128];
        strncpy(copy, line, sizeof(copy));
        copy[sizeof(copy)-1] = '\0';

        char *tokens[16]; // enough for RMC + extra fields
        int i = 0;
        char *tok = strtok(copy, ",");
        while (tok && i < 16) {
            tokens[i++] = tok;
            tok = strtok(NULL, ",");
        }


        // --- Step 6: Fallback manual parsing for non-standard extra fields ---
        if (i >= 10) // enough fields for time, status, lat, lon, date
        {
            // Status: A = valid, V = invalid
            g_latest_gps_data.is_valid = (tokens[2][0] == 'A');

            double lat_raw = atof(tokens[3]);
            int lat_deg = (int)(lat_raw / 100);
            double lat_min = lat_raw - (lat_deg * 100);
            double latitude = lat_deg + (lat_min / 60.0);
            if (tokens[4][0] == 'S') latitude = -latitude;
            g_latest_gps_data.latitude = latitude;

            double lon_raw = atof(tokens[5]);
            int lon_deg = (int)(lon_raw / 100);
            double lon_min = lon_raw - (lon_deg * 100);
            double longitude = lon_deg + (lon_min / 60.0);
            if (tokens[6][0] == 'W') longitude = -longitude;
            g_latest_gps_data.longitude = longitude;

            // Parse time (hhmmss.ss) and date (ddmmyy)
            int raw_time = atoi(tokens[1]);
            g_latest_gps_data.time.hour   = raw_time / 10000;
            g_latest_gps_data.time.minute = (raw_time / 100) % 100;
            g_latest_gps_data.time.second = raw_time % 100;

            int raw_date = atoi(tokens[9]);
            g_latest_gps_data.time.day    = raw_date / 10000;
            g_latest_gps_data.time.month  = (raw_date / 100) % 100;
            g_latest_gps_data.time.year   = (raw_date % 100) + 2000;
        }


    }
}


void disable_unwanted_nmea_msgs(void) {
    // List of PUBX disable commands for unwanted NMEA messages (as ASCII strings)
    const char *disable_cmds[] = {
        "$PUBX,40,GGA,0,0,0,0*5A\r\n",
        "$PUBX,40,GLL,0,0,0,0*5C\r\n",
        "$PUBX,40,GSA,0,0,0,0*4E\r\n",
        "$PUBX,40,GSV,0,0,0,0*59\r\n",
        "$PUBX,40,VTG,0,0,0,0*48\r\n"
    };
    int num_cmds = sizeof(disable_cmds) / sizeof(disable_cmds[0]);

    for (int i = 0; i < num_cmds; i++) {
        uart_write_blocking(UART_PORT, (const uint8_t *)disable_cmds[i], strlen(disable_cmds[i]));
        sleep_ms(150);  // small delay to allow GPS to process
    }
}


void gps_init(void) {
    // --- Pin setup ---
    #define TEST_PIN_GPS_TX 40
    #define TEST_PIN_GPS_RX 41

    gpio_set_function(TEST_PIN_GPS_TX, GPIO_FUNC_UART);
    gpio_set_function(TEST_PIN_GPS_RX, GPIO_FUNC_UART);

    // --- Initialize UART at GPS current baud rate ---
    uart_init(UART_PORT, 38400);
    sleep_ms(10);
    // --- Disable other common NMEA messages on UART1 ---

    uart_write_blocking(UART_PORT, (const uint8_t *)"$PUBX,40,RMC,0,1,0,0*46\r\n", 19);
    sleep_ms(150);

    uart_write_blocking(UART_PORT, (const uint8_t *)"$PUBX,40,VTG,0,0,0,0*48\r\n", 19);
    sleep_ms(150);

    disable_unwanted_nmea_msgs();

    // --- Enable UART interrupts and ISR ---
    irq_set_exclusive_handler(UART1_IRQ, on_uart_rx);
    irq_set_enabled(UART1_IRQ, true);
    uart_set_irq_enables(UART_PORT, true, false);
}



//
// gps_update() function (from your code)
//
void gps_update(void) {
    if (!line_ready) {
        return; // No new line to process
    }

    char line_to_process[LINE_BUFFER_LENGTH];
    
    uint32_t irq_status = save_and_disable_interrupts();
    strcpy(line_to_process, line_buffer);
    line_ready = false;
    restore_interrupts(irq_status);

    printf("RAW: %s\n", line_to_process);

    if (minmea_check(line_to_process, false)) {
        parse_line(line_to_process);

        if (g_latest_gps_data.is_valid) {
        printf("GPS FIX: %02d-%02d-%04d %02d:%02d:%02d | Lat: %.6f | Lon: %.6f\n",
            g_latest_gps_data.time.day,
            g_latest_gps_data.time.month,
            g_latest_gps_data.time.year,
            g_latest_gps_data.time.hour,
            g_latest_gps_data.time.minute,
            g_latest_gps_data.time.second,
            g_latest_gps_data.latitude,
            g_latest_gps_data.longitude
        );
        } else {
            printf("No valid GPS fix yet.\n");
        }
    }
}



#endif


#ifdef C1
/* ----------------------------- Public Functions --------------------------- */

void gps_init(void) {
    // Set the TX and RX pins (from config.h)
    gpio_set_function(PIN_GPS_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_GPS_RX, GPIO_FUNC_UART);

    // Initialize our UART at the module's *factory default* baud rate.
    uart_init(UART_PORT, 38400);
    
    // Set up and enable the UART RX interrupt.
    int UART_IRQ = (UART_PORT == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx_debug);
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(UART_PORT, true, false); // Enable RX interrupt
}

// We don't need a gps_update() function for this simple test.
void gps_update(void) {
    // Do nothing. The ISR does all the work.
}

#endif