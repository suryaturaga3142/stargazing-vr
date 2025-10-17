/*******************************************************************************
 * @file        gps.h
 * @brief       Header file for GPS driver library
 * @details     Function and locals decleration for GPS module.
 * 
 * @see         gps.c for implementation details.
 * 
 * @author      LED Chasers
 * @date        2025-10-14
 * @version     1.0
 ******************************************************************************/

#ifndef GPS_H
#define GPS_H

#include "pico/stdlib.h"
#include "hardware/uart.h"

typedef struct {
    char time[11];
    char status;
    double latitude;
    double longitude;
    char date[7];
} gps_data_t; 

void init_uart(uint baud_rate);
void uart_send(const uint8_t *data, size_t length);
int uart_read(char *buffer, size_t max_len);

void gps_init(void);
int gps_read_data(gps_data_t *data);


#endif /* GPS_H */