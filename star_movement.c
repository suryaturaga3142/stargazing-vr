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
struct quaternion quaternion_conjugate(struct quaternion q)
{
    struct quaternion q_c = {q.real, -1 * q.i, -1 * q.j, -1 * q.k};
    return q_c;
}

