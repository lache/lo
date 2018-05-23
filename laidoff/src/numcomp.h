#pragma once

#include "linmath.h"

typedef struct pcg_state_setseq_64 pcg32_random_t;

// compress 32-bit floating point value into
// c-bit fixed-point compressed value between [m, M].
typedef struct _LWNUMCOMPFLOATPRESET {
    int c; // compressed bit
    float m; // minimum original value (inclusive)
    float M; // maximum original value (inclusive)
    float l; // range
    float s; // step
    unsigned int M_comp; //maximum compressed value
} LWNUMCOMPFLOATPRESET;

typedef struct _LWNUMCOMPVEC3PRESET {
    LWNUMCOMPFLOATPRESET p[3]; // x, y, z
} LWNUMCOMPVEC3PRESET;

typedef struct _LWNUMCOMPQUATERNIONPRESET {
    LWNUMCOMPVEC3PRESET smallest_threes;
    int c_sum;
} LWNUMCOMPQUATERNIONPRESET;

// float
void numcomp_init_float_preset(LWNUMCOMPFLOATPRESET* preset, int c, float m, float M);
unsigned int numcomp_compress_float(float v, const LWNUMCOMPFLOATPRESET* preset);
float numcomp_decompress_float(unsigned int v_comp, const LWNUMCOMPFLOATPRESET* preset);
// vec3 (float x 3)
void numcomp_init_vec3_preset(LWNUMCOMPVEC3PRESET* preset,
                              int cx, float mx, float Mx,
                              int cy, float my, float My,
                              int cz, float mz, float Mz);
unsigned int numcomp_compress_vec3(const float v[3], const LWNUMCOMPVEC3PRESET* preset);
void numcomp_decompress_vec3(float v[3], unsigned int v_comp, const LWNUMCOMPVEC3PRESET* preset);
// quaternion
void numcomp_init_quaternion_preset(LWNUMCOMPQUATERNIONPRESET* preset);
unsigned int numcomp_compress_quaternion(const float q0[4], const LWNUMCOMPQUATERNIONPRESET* preset);
void numcomp_decompress_quaternion(float q[4], unsigned int c, const LWNUMCOMPQUATERNIONPRESET* preset);
// mat4x4
unsigned int numcomp_compress_mat4x4(const mat4x4 m, const LWNUMCOMPQUATERNIONPRESET* preset);
void numcomp_decompress_mat4x4(mat4x4 m, unsigned int v_comp, const LWNUMCOMPQUATERNIONPRESET* preset);
// misc
void numcomp_convert_euler_xyz_to_quaternion(float* q, float ex, float ey, float ez);
void numcomp_convert_quaternion_to_euler_xyz(const float q[4], float* ex, float* ey, float* ez);
float numcomp_wrap_max(float x, float max);
float numcomp_wrap_min_max(float x, float min, float max);
float numcomp_wrap_radian(float r);

void numcomp_test_all();
void numcomp_batch_test_float();
void numcomp_batch_test_vec3();
void numcomp_batch_test_quaternion();
void numcomp_batch_test_mat4x4();
void numcomp_batch_test_conversion();
float numcomp_float_random_01();
float numcomp_float_random_range(float m, float M);
float numcomp_float_random_01_local(pcg32_random_t* rng);
float numcomp_float_random_range_local(pcg32_random_t* rng, float m, float M);
