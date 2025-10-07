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

//Take in RA and Dec angles in radians of a star and output that star's quaternion location
struct quaternion star_coords_to_quaternion(float angle_ra_rad, float angle_dec_rad)
{
    struct quaternion q;
    q.real = 0;
    q.i = math.cos(angle_dec_rad) * math.cos(angle_ra_rad);
    q.j = math.cos(angle_dec_rad) * math.sin(angle_ra_rad);
    q.k = math.sin(angle_dec_rad);
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
    //TODO: Check this conversion, not sure if the 15 degrees per hour still applies in sidereal time
    float longitude_hours = longitude / 15; //Convert longitude to hours, 15 degrees per hour
    LST = (GMST + longitude_hours) % 24; 
    return LST;
}