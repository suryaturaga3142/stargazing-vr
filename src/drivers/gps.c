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
#include "string.h"
#include <stdio.h>

/* ---------------------------- Private Constants --------------------------- */
/* ----------------------------- Private Variables -------------------------- */
// config message 
static uint8_t ubx_cfg_msgout_gnrmc_uart0[] = {
    0xB5, 0x62,       // UBX header sync chars
    0x06, 0x01,       // Class = CFG (0x06), ID = CFG-MSG (0x01)
    0x08, 0x00,       // Payload length = 8 bytes
    0xF0, 0x06,       // Payload: message class (0xF0) and ID (0x06)
    0x01,             // UART0 rate = 1 → enable message once per fix
    0x00,             // UART1 rate = 0
    0x00,             // USB rate = 0
    0x00,             // SPI rate = 0
    0x00,             // I2C rate = 0
    0x00, 0x00        // Checksum placeholder
};


/* ----------------------------- Private Functions -------------------------- */
//checksum for config command
static void calc_ubx_checksum(uint8_t *msg, int length) 
{
    uint8_t ck_a = 0, ck_b = 0;
    for (int i = 2; i < length - 2; i++) 
    {
        ck_a += msg[i];
        ck_b += ck_a;
    }
    msg[length - 2] = ck_a;
    msg[length - 1] = ck_b;
}
//latitude/longitude conversion
double nmea_to_decimal(const char *coord, char dir) {
    if (!coord || !*coord) return 0.0;
    double val = atof(coord);
    int deg = (int)(val / 100);
    double min = val - deg * 100;
    double dec = deg + min / 60.0;
    if (dir == 'S' || dir == 'W') dec = -dec;
    return dec;
}


/* ----------------------------- Public Functions --------------------------- */
void init_uart(uint baud_rate)
{ 
    gpio_set_function(PIN_GPS_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_GPS_RX, GPIO_FUNC_UART);
    uart_init(UART_PORT, baud_rate);
    uart_set_format(UART_PORT, 8, 1, UART_PARITY_NONE);
}

int uart_read_line(char *buffer, size_t max_len) 
{
    size_t i = 0;
    while (i < max_len - 1) {
        uart_read_blocking(UART_PORT, &buffer[i], 1); // Read one byte at a time
        if (buffer[i] == '\n') {
            i++;
            break;
        }
        i++;
    }
    buffer[i] = '\0';
    return i;
}

void gps_init()
{
    //initialise uart 0
    init_uart(9600);

    //Calculate checksum for UBX config message
    calc_ubx_checksum(ubx_cfg_msgout_gnrmc_uart0, sizeof(ubx_cfg_msgout_gnrmc_uart0));

    //Send configuration message over UART
    uart_write_blocking(UART_PORT, ubx_cfg_msgout_gnrmc_uart0, sizeof(ubx_cfg_msgout_gnrmc_uart0));

    //Small delay for GPS to apply settings
    sleep_ms(100);
}


void process_gnrmc_from_uart() 
{
    char nmea_buf[128];
    GPSData_t gps;

    while (1) {
        int len = uart_read_line(nmea_buf, 128);
        if (len > 0 && strncmp(nmea_buf, "$GNRMC", 6) == 0) {
            if (parse_gnrmc(nmea_buf, &gps)) 
            {
                printf("UTC: %02d-%02d-%02d %02d:%02d:%02d\n",
                    gps.time.year, gps.time.month, gps.time.day,
                    gps.time.hour, gps.time.minute, gps.time.second);
                printf("Lat: %.6f, Lon: %.6f, Valid: %d\n",
                    gps.latitude, gps.longitude, gps.is_valid);
            }
            else {
                printf("parse failed\n");
            }
        }
    }
}

int parse_gnrmc(const char *sentence, GPSData_t *data) {
    if (!sentence || !data) return 0;
    if (strncmp(sentence, "$GNRMC", 6) != 0) return 0;

    // Copy to a buffer for strtok
    char buf[128];
    strncpy(buf, sentence, sizeof(buf));
    buf[sizeof(buf)-1] = '\0';

    char *token;
    int field = 0;

    token = strtok(buf, ",");
    while (token) 
    {
        switch (field) 
        {
            case 1: // UTC Time
                if (strlen(token) >= 6) 
                {
                    data->time.hour   = (uint8_t)((token[0]-'0')*10 + (token[1]-'0'));
                    data->time.minute = (uint8_t)((token[2]-'0')*10 + (token[3]-'0'));
                    data->time.second = (uint8_t)((token[4]-'0')*10 + (token[5]-'0'));
                }
                break;

            case 2: // Status (A = valid, V = invalid)
                data->is_valid = (token[0] == 'A');
                break;

            case 3: { // Latitude
                char lat_str[16] = {0};
                strncpy(lat_str, token, 15);
                token = strtok(NULL, ","); field++;
                char ns = token ? token[0] : 'N';
                data->latitude = nmea_to_decimal(lat_str, ns);
                break;
            }

            case 5: { // Longitude
                char lon_str[16] = {0};
                strncpy(lon_str, token, 15);
                token = strtok(NULL, ","); field++;
                char ew = token ? token[0] : 'E';
                data->longitude = nmea_to_decimal(lon_str, ew);
                break;
            }

            case 9: // Date (ddmmyy)
                if (strlen(token) >= 6) {
                    data->time.day   = (uint8_t)((token[0]-'0')*10 + (token[1]-'0'));
                    data->time.month = (uint8_t)((token[2]-'0')*10 + (token[3]-'0'));
                    data->time.year  = (uint16_t)(2000 + (token[4]-'0')*10 + (token[5]-'0'));
                }
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }
    return data->is_valid ? 1 : 0;
}

