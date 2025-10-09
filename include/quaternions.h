#ifndef QUATERNIONS_H
#define QUATERNIONS_H

#include <math.h>

typedef struct {
    float real;
    float i;
    float j;
    float k;
} quaternion;


/* Function definitions are in quaternions.c */

//Take in RA and Dec angles in degrees for a star and output that star's quaternion location
quaternion star_coords_to_quaternion(float angle_ra_deg, float angle_dec_deg);

//Create unit quaternion to represent rotation given a rotation axis and the amount to rotate by
quaternion rotation_to_quaternion(float angle, float[] axis);

//Take the conjugate of a quaternion
quaternion q_conjugate(quaternion q);

//Take the product of two quaternions q1 * q2
quaternion q_product(quaternion q1, quaternion q2);

//Get new position of a star after it is rotated by the rotation quaternion
quaternion rotate_quaternion(quaternion q_star, quaternion q_rotation);

#endif