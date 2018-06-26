
#pragma once

#include "linmath.h"

typedef struct _LWPARABOLA3D {
	float abc[3];
	float angle;
	vec3 p0;
	float p0t;
	float p2t;
	mat4x4 rotzinv;
} LWPARABOLA3D;

void lwparabola_three_points_on_2d_plane(const vec2 p0, const vec2 p1, const vec2 p2, vec3 abc);
float lwparabola_param_t(const vec3 abc, const vec2 p);
void lwparabola_p_from_param_t(const vec3 abc, const float t, vec2 p);
float lwparabola_pitch_rad_from_param_t(const vec3 abc, const float t);
void lwparabola_three_points_on_plane_perpendicular_to_xy_plane(const vec3 p0, const vec3 p1, const vec3 p2, LWPARABOLA3D* parabola3d);
void lwparabola_p_3d_from_param_t(const LWPARABOLA3D* parabola3d, const float t, vec3 p);
void lwparabola_test();
