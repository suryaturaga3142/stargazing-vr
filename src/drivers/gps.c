/*******************************************************************************
 * @file        gps.c
 * @brief       Implements the functionality for the GPS module.
 * @details     Coding the driver for the NEO M10 GPS.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * 
 * @note        This module is designed to be driven by interrupts and is not
 *              intended to be called from a blocking main loop.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

/* ----------------------------- Private Includes --------------------------- */
#include "gps.h"
#include "string.h"

/* ---------------------------- Private Constants --------------------------- */
#define UART_ID uart0
#define TX_PIN 0
#define RX_PIN 1

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
    gpio_set_function(TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);
    uart_init(UART_ID, baud_rate);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
}

// uart_write_blocking(UART_ID, data, length); for sending commands
// uart_read_blocking(UART_ID, buffer, max_len);

void gps_init()
{
    //initialise uart 0
    init_uart(9600);

    //Calculate checksum for UBX config message
    calc_ubx_checksum(ubx_cfg_msgout_gnrmc_uart0, sizeof(ubx_cfg_msgout_gnrmc_uart0));

    //Send configuration message over UART
    uart_write_blocking(UART_ID, ubx_cfg_msgout_gnrmc_uart0, sizeof(ubx_cfg_msgout_gnrmc_uart0));

    //Small delay for GPS to apply settings
    sleep_ms(100);
}