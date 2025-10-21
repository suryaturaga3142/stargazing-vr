/*******************************************************************************
 * @file        gps.h
 * @brief       Header file for GPS driver library
 * @details     Function and locals declaration for GPS module.
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
int uart_read_line(char *buffer, size_t max_len);

void gps_init(void);
int gps_read_data(gps_data_t *data);
int parse_gnrmc(const char *sentence, gps_data_t *data);
void process_gnrmc_from_uart(void);



#endif /* GPS_H */