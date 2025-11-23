/*******************************************************************************
 * @file        startup.h
 * @brief       Header file for the startup routines.
 * @details     Contains all the function definitions.
 * 
 * @see         startup.c for implementation details.
 * 
 * @author      LED Chasers
 * @date        2025-11-23
 * @version     1.0
 ******************************************************************************/

#ifndef STARTUP_H
#define STARTUP_H

void startup_peripherals(void);
void startup_ui(void);
void startup_check_sd(void);
void startup_imu(void);
void startup_lcd(void);
void startup_sd(void);
void startup_gps(void);

#endif /* STARTUP_H */
