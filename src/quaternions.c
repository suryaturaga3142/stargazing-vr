/*
* Basic quaternion operations
*/

#include "quaternions.h"

//Take in RA and Dec angles in degrees for a star and output that star's quaternion location
quaternion star_coords_to_quaternion(float angle_ra_deg, float angle_dec_deg)
{
    quaternion q;
    q.real = 0;
    q.i = cos(angle_dec_deg) * cos(angle_ra_deg);
    q.j = cos(angle_dec_deg) * sin(angle_ra_deg);
    q.k = sin(angle_dec_deg);
    return q;
}

//Create unit quaternion to represent rotation given a rotation axis and the amount to rotate by
//TODO: Is an array the best way to pass in the axis? Also should it be checked to verify it's a unit vector
quaternion rotation_to_quaternion(float angle, float axis[])
{
    quaternion q_rotation;
    q_rotation.real = cos(angle / 2);
    q_rotation.i = sin(angle / 2) * axis[0];
    q_rotation.j = sin(angle / 2) * axis[1];
    q_rotation.k = sin(angle / 2) * axis[2];
    return q_rotation;
}

//Take the conjugate of a quaternion
quaternion q_conjugate(quaternion q)
{
    quaternion q_c = {q.real, -1 * q.i, -1 * q.j, -1 * q.k};
    return q_c;
}

//Take the product of two quaternions q1 * q2
quaternion q_product(quaternion q1, quaternion q2)
{
    quaternion q3;
    q3.real = (q1.real * q2.real) - (q1.i * q2.i) - (q1.j * q2.j) - (q1.k * q2.k); //Calculate real component
    q3.i = (q1.real * q2.i) + (q1.i * q2.real) + (q1.j * q2.k) - (q1.k * q2.j); //Calculate i component
    q3.j = (q1.real * q2.j) - (q1.i * q2.k) + (q1.j * q2.real) + (q1.k * q2.i); //Calculate j component
    q3.k = (q1.real * q2.k) + (q1.i * q2.j) - (q1.j * q2.i) + (q1.k * q2.real); //Calculate k component
    return q3;
}

//Get new position of a star after it is rotated by the rotation quaternion
quaternion rotate_quaternion(quaternion q_star, quaternion q_rotation)
{
    quaternion q_star_new; //New position of star
    quaternion q_rot_inverse = q_conjugate(q_rotation); //Get inverse of quaternion (conjugate for unit quaternion)
    //TODO: Verify this doesn't need to be (Qr^-1 * Qs) * Qr because Earth is rotating instead of stars
    q_star_new = q_product(q_product(q_rotation, q_star), q_rot_inverse); //Calculate (Qr * Qs) * Qr^-1
    return q_star_new;
}
