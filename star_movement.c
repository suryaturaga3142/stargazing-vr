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
    q_star_new = q_product(q_product(q_rotation, q_star), q_rot_inverse); //Calculate (Qr x Qs) x Qr-1
    return q_star_new;
}

//Take the cross product of two quaternions
struct quaternion q_product(struct quaternion q1, struct quaternion q2)
{
    struct quaternion q3;
    q3.real = (q1.real * q2.real) - (q1.i * q2.i) - (q1.j * q2.j) - (q1.k * q2.k); //Calculate real component
    q3.i = (q1.real * q2.i) + (q1.i * q2.real) + (q1.j * q2.k) - (q1.k * q2.j); //Calculate i component
    q3.j = (q1.real * q2.j) - (q1.i * q2.k) + (q1.j * q2.real) + (q1.k * q2.i); //Calculate j component
    q3.k = (q1.real * q2.k) + (q1.i * q2.j) - (q1.j * q2.i) + (q1.k * q2.real); //Calculate k component
    return q3;
}