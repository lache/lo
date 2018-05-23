#include "lwparabola.h"
#include <string.h>
#include "lwmacro.h"
#include "lwlog.h"

void lwparabola_three_points_on_2d_plane(const vec2 p0, const vec2 p1, const vec2 p2, vec3 abc) {
	const mat4x4 A = {
		p0[0] * p0[0],	p1[0] * p1[0],	p2[0] * p2[0],	0,
		p0[0],			p1[0],			p2[0],			0,
		1,				1,				1,				0,
		0,				0,				0,				1
	};
	const vec4 b = {
		p0[1],
		p1[1],
		p2[1],
		1
	};
	mat4x4 A_inv;
	mat4x4_invert(A_inv, A);
	vec4 x;
	mat4x4_mul_vec4(x, A_inv, b);
	memcpy(abc, x, sizeof(float) * 3);
}

float lwparabola_param_t(const vec3 abc, const vec2 p) {
	return p[0] + (abc[1] / (2 * abc[0]));
}

void lwparabola_p_from_param_t(const vec3 abc, const float t, vec2 p) {
	p[0] = t - abc[1] / (2 * abc[0]);
	p[1] = abc[0] * t * t + (4 * abc[0] * abc[2] - abc[1] * abc[1]) / (4 * abc[0]);
}

float lwparabola_pitch_rad_from_param_t(const vec3 abc, const float t) {
	return atan2f(2 * abc[0] * t, 1);
}

void lwparabola_three_points_on_plane_perpendicular_to_xy_plane(const vec3 p0, const vec3 p1, const vec3 p2, LWPARABOLA3D* parabola3d) {
	// Translate origin to p0
	vec4 p1_0 = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2], 1 };
	vec4 p2_0 = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2], 1 };
	// Calculate angle between positive x-axis and plane which consists of three points p0, p1 and p2
	parabola3d->angle = atan2f(p2_0[1], p2_0[0]);
	// Rotate p1_0 and p2_0 around z-axis by -angle
	mat4x4 rotz;
	mat4x4 identity;
	mat4x4_identity(identity);
	mat4x4_rotate_Z(rotz, identity, -parabola3d->angle);
	mat4x4_rotate_Z(parabola3d->rotzinv, identity, parabola3d->angle);
	vec4 p1_0_rotz;
	vec4 p2_0_rotz;
	mat4x4_mul_vec4(p1_0_rotz, rotz, p1_0);
	mat4x4_mul_vec4(p2_0_rotz, rotz, p2_0);
	// Revert origin
	vec3 p1_rotz = { p1_0_rotz[0] + p0[0], p1_0_rotz[1] + p0[1], p1_0_rotz[2] + p0[2] };
	vec3 p2_rotz = { p2_0_rotz[0] + p0[0], p2_0_rotz[1] + p0[1], p2_0_rotz[2] + p0[2] };
	// Project p0, p1_proj, p2_proj to xz plane
	vec2 p0_2d = { p0[0], p0[2] };
	vec2 p1_2d = { p1_rotz[0], p1_rotz[2] };
	vec2 p2_2d = { p2_rotz[0], p2_rotz[2] };
	// Calculate parabola equation (quadratic equation) from three points p0_2d, p1_2d, p2_2d
	lwparabola_three_points_on_2d_plane(p0_2d, p1_2d, p2_2d, parabola3d->abc);
	// Copy p0
	memcpy(parabola3d->p0, p0, sizeof(vec3));
	// Calculate p0t
	parabola3d->p0t = lwparabola_param_t(parabola3d->abc, p0_2d);
	// Calculate p2t
	parabola3d->p2t = lwparabola_param_t(parabola3d->abc, p2_2d);
}

void lwparabola_p_3d_from_param_t(const LWPARABOLA3D* parabola3d, const float t, vec3 p) {
	// Get 2d point first
	vec2 p2d;
	lwparabola_p_from_param_t(parabola3d->abc, t, p2d);
	// Get 3d point from 2d
	vec3 p3d_rotz = { p2d[0], parabola3d->p0[1], p2d[1] };
	// Translate origin to parabola3d->p0
	vec4 p3d_rotz_0 = {
		p3d_rotz[0] - parabola3d->p0[0],
		p3d_rotz[1] - parabola3d->p0[1],
		p3d_rotz[2] - parabola3d->p0[2],
		1
	};
	// Rotate by parabola3d->rotzinv
	vec4 p3d_0;
	mat4x4_mul_vec4(p3d_0, parabola3d->rotzinv, p3d_rotz_0);
	// Revert origin and save result
	p[0] = p3d_0[0] + parabola3d->p0[0];
	p[1] = p3d_0[1] + parabola3d->p0[1];
	p[2] = p3d_0[2] + parabola3d->p0[2];
}

static void s_lwparabola_test1() {
	//vec2 p0 = { -2, 1.75f };
	vec2 p0 = { -3, 0 };
	vec2 p1 = { 0, 2 };
	vec2 p2 = { 3, 0 };
	vec3 abc;
	lwparabola_three_points_on_2d_plane(p0, p1, p2, abc);
	LOGI("a = %f, b = %f, c = %f", abc[0], abc[1], abc[2]);
	float p0t = lwparabola_param_t(abc, p0);
	float p2t = lwparabola_param_t(abc, p2);
	LOGI("p0t = %f, p2t = %f", p0t, p2t);
	float tlen = fabsf(p0t - p2t);
	int tcount = 10;
	float tdelta = tlen / tcount;
	for (int i = 0; i < tcount + 1; i++) {
		vec2 p;
		float t = p0t + tdelta * i;
		lwparabola_p_from_param_t(abc, t, p);
		float pitch = lwparabola_pitch_rad_from_param_t(abc, t);
		LOGI("t = %f, x = %f, y = %f, pitch = %f deg", t, p[0], p[1], LWRAD2DEG(pitch));
	}
}

static void s_lwparabola_test2() {
	/*vec3 p0 = { -3, 0, 0 };
	vec3 p1 = { 0, 0, 2 };
	vec3 p2 = { 3, 0, 0 };*/
	vec3 p0 = { -3, -3, 1 };
	vec3 p1 = { 0, 0, 10 };
	vec3 p2 = { 30, 30, 0 };
	LWPARABOLA3D parabola3d;
	lwparabola_three_points_on_plane_perpendicular_to_xy_plane(p0, p1, p2, &parabola3d);
	LOGI("[3D] a = %f, b = %f, c = %f", parabola3d.abc[0], parabola3d.abc[1], parabola3d.abc[2]);
	LOGI("[3D] p0t = %f, p2t = %f", parabola3d.p0t, parabola3d.p2t);
	float tlen = fabsf(parabola3d.p0t - parabola3d.p2t);
	int tcount = 10;
	float tdelta = tlen / tcount;
	for (int i = 0; i < tcount + 1; i++) {
		vec3 p;
		float t = parabola3d.p0t + tdelta * i;
		lwparabola_p_3d_from_param_t(&parabola3d, t, p);
		float pitch = lwparabola_pitch_rad_from_param_t(parabola3d.abc, t);
		LOGI("[3D] t = %f, x = %f, y = %f, z = %f, pitch = %f deg", t, p[0], p[1], p[2], LWRAD2DEG(pitch));
	}
}

void lwparabola_test() {
	LOGI("[lwparabola_test begin]");
	s_lwparabola_test1();
	s_lwparabola_test2();
	LOGI("[lwparabola_test end]");
}
