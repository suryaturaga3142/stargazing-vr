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
#include "interrupts.h"
#include "user_ui.h"
#include "monitor.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
//#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// ...

/* ---------------------------- Private Constants --------------------------- */
// ...

/* ----------------------------- Private Variables -------------------------- */
// ...

/* ----------------------------- Private Functions -------------------------- */

/**
 * @brief Copy of strsep bc this library doesn't have it
 * 
 * @param stringp Pointer to array
 * @param delim Delimiter string
 * @return start Beginning of next string or NULL if ended
 */
static char *my_strsep(char **stringp, const char *delim) {
    char *start = *stringp;
    char *p;
    if (start == NULL) return NULL;
    p = strpbrk(start, delim);
    if (p == NULL) {
        *stringp = NULL;
    } else {
        *p = '\0';
        *stringp = p + 1;
    }
    return start;
}

/**
 * @brief Converts the regular text into decimal accounting for negatives
 * 
 * @param nmea_val The raw value given
 * @param dir N/S/E/W
 * @return decimal Latitude/longitude as float
 */
static float nmea_to_decimal(float nmea_val, char dir) {
    int degrees = (int)(nmea_val / 100);
    float minutes = nmea_val - (degrees * 100);
    float decimal = degrees + (minutes / 60.0f);
    if (dir == 'S' || dir == 'W') {
        decimal *= -1.0f;
    }
    return decimal;
}

/**
 * @brief Parses an extracted sentence using standard tools with strsep
 * 
 * @param nmea-sentence Extracted $GNRMC sentence
 * @return true if g_latest_gps_data was successfully updated
 */
static bool gps_parse_rmc(char *nmea_sentence) {
    
    char buffer[100];
    strncpy(buffer, nmea_sentence, 100);
    buffer[sizeof(buffer) - 1] = '\0';

    char *current = buffer;
    char *token;
    int field_index = 0;

    char raw_time[16] = {0};
    char status = 'V';
    float raw_lat = 0.0f;
    char lat_dir = 'N';
    float raw_lon = 0.0f;
    char lon_dir = 'E';
    char raw_date[8] = {0};

    while ((token = my_strsep(&current, ",")) != NULL) {
        switch (field_index) {
            case 0: // Tag
                if (strcmp(token, "$GNRMC") != 0 && strcmp(token, "GNRMC") != 0) return false;
                break;
            case 1: // Time
                strncpy(raw_time, token, sizeof(raw_time) - 1);
                break;
            case 2: // Status
                if (strlen(token) > 0) status = token[0];
                break;
            case 3: // Latitude
                raw_lat = strtof(token, NULL);
                break;
            case 4: // N/S
                if (strlen(token) > 0) lat_dir = token[0];
                break;
            case 5: // Longitude
                raw_lon = strtof(token, NULL);
                break;
            case 6: // E/W
                if (strlen(token) > 0) lon_dir = token[0];
                break;
            case 9: // Date
                strncpy(raw_date, token, sizeof(raw_date) - 1);
                break;
        }
        field_index++;
    }
    
    if (status == 'A') {
        g_latest_gps_data.is_valid = true;
    } else {
        g_latest_gps_data.is_valid = false;
        return false;
    }

    g_latest_gps_data.latitude = nmea_to_decimal(raw_lat, lat_dir);
    g_latest_gps_data.longitude = nmea_to_decimal(raw_lon, lon_dir);

    if (strlen(raw_time) >= 6) {
        char tmp[3] = {0};
        tmp[0] = raw_time[0]; 
        tmp[1] = raw_time[1];
        g_latest_gps_data.time.hour = atoi(tmp);
        
        tmp[0] = raw_time[2]; 
        tmp[1] = raw_time[3];
        g_latest_gps_data.time.minute = atoi(tmp);
        
        tmp[0] = raw_time[4]; 
        tmp[1] = raw_time[5];
        g_latest_gps_data.time.second = atoi(tmp);
    }

    if (strlen(raw_date) == 6) {
        char tmp[3] = {0};
        tmp[0] = raw_date[0]; 
        tmp[1] = raw_date[1];
        g_latest_gps_data.time.day = atoi(tmp);
        
        tmp[0] = raw_date[2]; 
        tmp[1] = raw_date[3];
        g_latest_gps_data.time.month = atoi(tmp);
        
        tmp[0] = raw_date[4]; 
        tmp[1] = raw_date[5];
        g_latest_gps_data.time.year = 2000 + atoi(tmp);
    }

    return true;
}
// ...

/* ----------------------------- Public Variables --------------------------- */
GPSData_t g_latest_gps_data = {0};

volatile bool g_gps_correction_needed = false;

/* ----------------------------- Public Functions --------------------------- */

/**
 * @brief Initialize the GPS module and enable the location and date/time
 * 
 * @return true if initialization was successful
 */
bool gps_init(void) {
    // Setup the UART peripheral for GPS module.
    gpio_init(PIN_GPS_TX);
    gpio_init(PIN_GPS_RX);
    gpio_set_function(PIN_GPS_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_GPS_RX, GPIO_FUNC_UART);
    uart_init(UART_PORT, GPS_UART_BAUD);
    uart_set_hw_flow(UART_PORT, false, false);
    uart_set_format(UART_PORT, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_PORT, true);

    // Setup a timer with interrupt to poll GPS data periodically
    timer0_hw->inte |= TIMER_INTE_ALARM_1_BITS;
    irq_set_exclusive_handler(TIMER0_IRQ_1, irq_timer_gps_callback);
    irq_set_enabled(TIMER0_IRQ_1, true);
    timer0_hw->alarm[1] = timer_hw->timerawl + (GPS_CORRECTION_INTERVAL_MIN * 60 * 1000);

    // Attempt a single parse
    if (g_use_gps_location) {

        user_ui_set_state(LED_STATE_GPS_SEARCHING);
        
        absolute_time_t deadline = make_timeout_time_ms(GPS_INIT_TIMEOUT_MS);

        while(!time_reached(deadline)) {
            watchdog_update();

            if (uart_is_readable(UART_PORT) && (uart_getc(UART_PORT) == '$')) {

                char tag[6];
                uart_read_blocking(UART_PORT, tag, 5); 
                tag[5] = '\0';

                if (strcmp(tag, "GNRMC") == 0) {
                    char sentence[100];
                    strcpy(sentence, "$GNRMC");
                    int idx = 6; 

                    while (idx < 99) {
                        if (uart_is_readable_within_us(UART_PORT, 100000)) {
                            char data = uart_getc(UART_PORT);
                            sentence[idx++] = data;
                            
                            if (data == '\n') {
                                sentence[idx] = '\0';
                                break;
                            }
                        }
                        else return false;
                    }

                    return gps_parse_rmc(sentence);
                }
            }
        }
    }

    // Ok to do since g_latest_gps_data.is_valid = false
    return true;
}

/**
 * @brief Checks the global flag for needing new GPS data and reads if needed. Called by main only.
 * 
 * @return true if data was updated successfully.
 */
bool gps_check_and_read(void) {

    if (!g_gps_correction_needed) {
        return false;
    }

    g_gps_correction_needed = false; // This is ok bc we can skip updates every now and then

    user_ui_set_state(LED_STATE_GPS_SEARCHING);

    absolute_time_t deadline = make_timeout_time_ms(GPS_TIMEOUT_MS);

    while(!time_reached(deadline)) {

        if (uart_is_readable(UART_PORT) && (uart_getc(UART_PORT) == '$')) {

            char tag[6];
            uart_read_blocking(UART_PORT, tag, 5); 
            tag[5] = '\0';

            if (strcmp(tag, "GNRMC") == 0) {

                //printf("Tag found\r\n");

                char sentence[100];
                strcpy(sentence, "$GNRMC");
                int idx = 6; 

                while (idx < 99) {
                    if (uart_is_readable_within_us(UART_PORT, 100000)) {
                        char data = uart_getc(UART_PORT);
                        sentence[idx++] = data;
                        
                        if (data == '\n') {
                            sentence[idx] = '\0';
                            break;
                        }
                    }
                    else return false;
                }

                //printf("Sentence: %s\r\n", sentence);

                return gps_parse_rmc(sentence);
            }
        }
    }

    
    return false;
}