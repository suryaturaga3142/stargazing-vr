#ifndef STARMOVEMENT_H
#define STARMOVEMENT_H

#include <math.h>
#include <quaternions.h>

/* Function definitions are in star_movements.c */

//Get the quaternion for the rotation of the earth based on the amount of time passed
quaternion get_earth_rotation(float seconds);

//Update star positions based on Earth rotating for a set period of time
void rotate_earth(float seconds);

//Convert Universal Time to Greenwich Mean Sidereal Time
//D is Julian day (days since J2000.0), H is hours since 0h UT 
float convert_UT_to_GMST(int D, float H);

//Convert Greenwich Mean Sidereal Time to sidereal time for the current location
float convert_GMST_to_LST(float GMST, float longitude);

//Get user's starting location and time and initialize stars based on that info
void init_star_locations();

#endif