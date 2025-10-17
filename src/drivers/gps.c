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
// ...

/* ----------------------------- Private Functions -------------------------- */
// ...

/* ----------------------------- Public Functions --------------------------- */
void init_uart(uint baud_rate)
{ 
    gpio_set_function(TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);
    uart_init(UART_ID, baud_rate);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
}

void uart_send(const uint8_t *data, size_t length) 
{
    uart_write_blocking(UART_ID, data, length);
}

int uart_read(char *buffer, size_t max_len)
{
    int i;
    for(i = 0; i < max_len-1; i++)
    {
        while(!uart_is_readable(UART_ID))
        {
            tight_loop_contents();
        }
        char c = uart_getc(UART_ID);
        buffer[i] = c;
        if(c == '\n')
        {
            break;
        }
    }
    buffer[i+1] = '\0';
    return i+1;
}