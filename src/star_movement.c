/* 
 * Functions for modeling Earth's rotation and how it affects star location
*/

#include "star_movement.h"
#include "quaternions.h"

float earth_angular_velocity = (7.292115e-5 * 180 / 3.14159265358979323846); //degrees per second
float earth_axis[]; //TODO: Make quaternion instead so it's easier to update based on position relative to GPS location?

//Get the quaternion for the rotation of the earth based on the amount of time passed
//TODO: Is there a way to have a function as the quaternion angle? Or is this good?
quaternion get_earth_rotation(float seconds)
{
    float angle = (seconds * earth_angular_velocity); // % 360; //degrees
    quaternion earth_rotation = rotation_to_quaternion(angle, earth_axis);
    return earth_rotation;
}

//Update star positions based on Earth rotating for a set period of time
//TODO: Finish function
void rotate_earth(float seconds)
{
    quaternion earth_rotation = get_earth_rotation(seconds);
    quaternion new_star;
    // for each star in list of stars
    //      new_star = rotate_quaternion(star, earth_rotation); 
    //      replace star in list with new star value
}

//Convert Universal Time to Greenwich Mean Sidereal Time
//D is Julian day (days since J2000.0), H is hours since 0h UT 
float convert_UT_to_GMST(int D, float H)
{
    float GMST = (6.697374558 + (0.06570982441908 * D) + (1.00273790935 * H)); // % 24; //hours
    return GMST;
}

//Convert Greenwich Mean Sidereal Time to sidereal time for the current location
float convert_GMST_to_LST(float GMST, float longitude)
{
    float LST; //Local Standard Time
    float longitude_hours = longitude / 15.041; //Convert longitude to hours, 15.041 degrees per hour
    LST = (GMST + longitude_hours); // % 24; 
    return LST;
}

//Get user's starting location and time and initialize stars based on that info
//TODO: Finish function
void init_star_locations()
{
    //TODO: Convert all stars to quaternions from list of RA and Dec angles using star_coords_to_quaternion() fxn
    float longitude, latitude; //TODO: Call function to get location from GPS
    int days; //TODO: Call function to get date and time
    float hours; 
    float rotation_seconds = 3600 * convert_GMST_to_LST(convert_UT_to_GMST(days, hours), longitude); //Get time difference from 
    rotate_earth(rotation_seconds); //Rotate Earth the necessary amount of time to get star locations relative to user's longitude
    return;
}