/* 
 * Functions for modeling Earth's rotation and how it affects star location
*/

#include <math.h>

struct quaternion {
    float real;
    float i;
    float j;
    float k;
};

static float earth_angular_velocity = (7.292115e-5 * 180 / math.pi); //degrees per second
float[] earth_axis; //TODO: Make quaternion instead so it's easier to update based on position relative to GPS location?

//Take in RA and Dec angles in degrees for a star and output that star's quaternion location
struct quaternion star_coords_to_quaternion(float angle_ra_deg, float angle_dec_deg)
{
    struct quaternion q;
    q.real = 0;
    q.i = math.cos(angle_dec_deg) * math.cos(angle_ra_deg);
    q.j = math.cos(angle_dec_deg) * math.sin(angle_ra_deg);
    q.k = math.sin(angle_dec_deg);
    return q;
}

//Create unit quaternion to represent rotation given a rotation axis and the amount to rotate by
//TODO: Is an array the best way to pass in the axis? Also should it be checked to verify it's a unit vector
struct quaternion rotation_to_quaternion(float angle, float[] axis)
{
    struct quaternion q_rotation;
    q_rotation.real = math.cos(angle / 2);
    q_rotation.i = math.sin(angle / 2) * axis[0];
    q_rotation.j = math.sin(angle / 2) * axis[1];
    q_rotation.k = math.sin(angle / 2) * axis[2];
    return q_rotation;
}

//Get the quaternion for the rotation of the earth based on the amount of time passed
//TODO: Is there a way to have a function as the quaternion angle? Or is this good?
struct quaternion get_earth_rotation(float seconds)
{
    float angle = (seconds * earth_angular_velocity) % 360;
    struct quaternion earth_rotation = rotation_to_quaternion(angle, earth_axis);
    return earth_rotation;
}

//Update star positions based on Earth rotating for a set period of time
//TODO: Finish function
void rotate_earth(float seconds)
{
    struct quaternion earth_rotation = get_earth_rotation(seconds);
    struct quaternion new_star;
    // for each star in list of stars
    //      new_star = rotate_quaternion(star, earth_rotation); 
    //      replace star in list with new star value
}

//Take the conjugate of a quaternion
struct quaternion q_conjugate(struct quaternion q)
{
    struct quaternion q_c = {q.real, -1 * q.i, -1 * q.j, -1 * q.k};
    return q_c;
}

//Get new position of a star after it is rotated by the rotation quaternion
struct quaternion rotate_quaternion(struct quaternion q_star, struct quaternion q_rotation)
{
    struct quaternion q_star_new; //New position of star
    struct quaternion q_rot_inverse = q_conjugate(q_rotation); //Get inverse of quaternion (conjugate for unit quaternion)
    //TODO: Verify this doesn't need to be (Qr^-1 * Qs) * Qr because Earth is rotating instead of stars
    q_star_new = q_product(q_product(q_rotation, q_star), q_rot_inverse); //Calculate (Qr * Qs) * Qr^-1
    return q_star_new;
}

//Take the product of two quaternions q1 * q2
struct quaternion q_product(struct quaternion q1, struct quaternion q2)
{
    struct quaternion q3;
    q3.real = (q1.real * q2.real) - (q1.i * q2.i) - (q1.j * q2.j) - (q1.k * q2.k); //Calculate real component
    q3.i = (q1.real * q2.i) + (q1.i * q2.real) + (q1.j * q2.k) - (q1.k * q2.j); //Calculate i component
    q3.j = (q1.real * q2.j) - (q1.i * q2.k) + (q1.j * q2.real) + (q1.k * q2.i); //Calculate j component
    q3.k = (q1.real * q2.k) + (q1.i * q2.j) - (q1.j * q2.i) + (q1.k * q2.real); //Calculate k component
    return q3;
}

//Convert Universal Time to Greenwich Mean Sidereal Time
//D is Julian day (days since J2000.0), H is hours since 0h UT 
float convert_UT_to_GMST(int D, float H)
{
    float GMST = (6.697374558 + (0.06570982441908 * D) + (1.00273790935 * H)) % 24; //hours
    return GMST;
}

//Convert Greenwich Mean Sidereal Time to sidereal time for the current location
float convert_GMST_to_LST(float GMST, float longitude)
{
    float LST; //Local Standard Time
    float longitude_hours = longitude / 15.041; //Convert longitude to hours, 15.041 degrees per hour
    LST = (GMST + longitude_hours) % 24; 
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