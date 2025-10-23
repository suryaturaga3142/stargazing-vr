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
#include "globals.h"
 
// typedef struct {
//     UTCTime_t time;
//     float     latitude;
//     float     longitude;
//     bool      is_valid;
// } GPSData_t;
// typedef struct {
//     uint16_t year;
//     uint8_t  month;
//     uint8_t  day;
//     uint8_t  hour;
//     uint8_t  minute;
//     uint8_t  second;
// } UTCTime_t;


void init_uart(uint baud_rate);
int uart_read_line(char *buffer, size_t max_len);

void gps_init(void);
int gps_read_data(GPSData_t *data);
int parse_gnrmc(const char *sentence, GPSData_t *data);
void process_gnrmc_from_uart(void);



#endif /* GPS_H */