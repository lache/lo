#include <math.h>
#include <string.h>
#include "numcomp.h"
#include "lwlog.h"
#include "lwmacro.h"
#include "pcg_basic.h"

void numcomp_init_float_preset(LWNUMCOMPFLOATPRESET* preset, int c, float m, float M) {
    preset->c = c;
    preset->m = m;
    preset->M = M;
    preset->l = M - m;
    preset->s = preset->l / (powf(2.0f, (float)c) - 1.0f);
    preset->M_comp = (unsigned int)((1 << preset->c) - 1);
}

unsigned int numcomp_compress_float(float v, const LWNUMCOMPFLOATPRESET* preset) {
    if (v <= preset->m) {
        return 0;
    }
    if (v >= preset->M) {
        return preset->M_comp;
    }
    float base = v - preset->m;
    return (unsigned int)((base / preset->s) + 0.5f);
}

float numcomp_decompress_float(unsigned int v_comp, const LWNUMCOMPFLOATPRESET* preset) {
    return preset->m + v_comp * preset->s;
}

float numcomp_test_float(float v, const LWNUMCOMPFLOATPRESET* preset) {
    unsigned int v_comp = numcomp_compress_float(v, preset);
    float v_decomp = numcomp_decompress_float(v_comp, preset);
    return fabsf(v - v_decomp);
}

void numcomp_test_float_print(float v, const LWNUMCOMPFLOATPRESET* preset) {
    float err = numcomp_test_float(v, preset);
    LOGI("float:%.4f (err:%.6f)", v, err);
}

void numcomp_init_vec3_preset(LWNUMCOMPVEC3PRESET* preset,
                              int cx, float mx, float Mx,
                              int cy, float my, float My,
                              int cz, float mz, float Mz) {
    numcomp_init_float_preset(&preset->p[0], cx, mx, Mx);
    numcomp_init_float_preset(&preset->p[1], cy, my, My);
    numcomp_init_float_preset(&preset->p[2], cz, mz, Mz);
}

unsigned int numcomp_compress_vec3(const float v[3], const LWNUMCOMPVEC3PRESET* preset) {
    unsigned int v_comp_x = numcomp_compress_float(v[0], &preset->p[0]);
    unsigned int v_comp_y = numcomp_compress_float(v[1], &preset->p[1]);
    unsigned int v_comp_z = numcomp_compress_float(v[2], &preset->p[2]);
    return v_comp_x
           | (v_comp_y << (preset->p[0].c))
           | (v_comp_z << (preset->p[0].c + preset->p[1].c));
}

void numcomp_decompress_vec3(float v[3], unsigned int v_comp, const LWNUMCOMPVEC3PRESET* preset) {
    unsigned int v_comp_x = (v_comp                                     ) & ((1 << preset->p[0].c) - 1);
    unsigned int v_comp_y = (v_comp >> (preset->p[0].c)                 ) & ((1 << preset->p[1].c) - 1);
    unsigned int v_comp_z = (v_comp >> (preset->p[0].c + preset->p[1].c)) & ((1 << preset->p[2].c) - 1);
    v[0] = numcomp_decompress_float(v_comp_x, &preset->p[0]);
    v[1] = numcomp_decompress_float(v_comp_y, &preset->p[1]);
    v[2] = numcomp_decompress_float(v_comp_z, &preset->p[2]);
}

float numcomp_test_vec3(const float* v, const LWNUMCOMPVEC3PRESET* preset) {
    unsigned int v_comp = numcomp_compress_vec3(v, preset);
    float v_decomp[3];
    numcomp_decompress_vec3(v_decomp, v_comp, preset);
    float dx = v[0] - v_decomp[0];
    float dy = v[1] - v_decomp[1];
    float dz = v[2] - v_decomp[2];
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

void numcomp_test_vec3_print(const float* v, const LWNUMCOMPVEC3PRESET* preset) {
    float err = numcomp_test_vec3(v, preset);
    LOGI("vec3:(%.4f,%.4f,%.4f) (err:%.6f)", v[0], v[1], v[2], err);
}

float numcomp_float_random_01() {
    const int random_mask = 0x7fffffff;
    return (float)(pcg32_random() & random_mask) / random_mask;
}

float numcomp_float_random_range(float m, float M) {
    return m + numcomp_float_random_01() * (M - m);
}

float numcomp_float_random_01_local(pcg32_random_t* rng) {
    const int random_mask = 0x7fffffff;
    return (float)(pcg32_random_r(rng) & random_mask) / random_mask;
}

float numcomp_float_random_range_local(pcg32_random_t* rng, float m, float M) {
    return m + numcomp_float_random_01_local(rng) * (M - m);
}

void numcomp_test_all() {
    LOGI("*************************************************");
    numcomp_batch_test_float();
    LOGI("*************************************************");
    numcomp_batch_test_vec3();
    LOGI("*************************************************");
    numcomp_batch_test_conversion();
    LOGI("*************************************************");
    numcomp_batch_test_quaternion();
    LOGI("*************************************************");
    numcomp_batch_test_mat4x4();
}

void numcomp_batch_test_vec3() {
    LWNUMCOMPVEC3PRESET p2;
    numcomp_init_vec3_preset(&p2,
                             11, -2.0f, +2.0f,
                             11, -2.0f, +2.0f,
                             10, +0.0f, +5.0f);
    float v_high_z[3] = { 0, 0, 10.0f };
    numcomp_test_vec3_print(v_high_z, &p2);
    float v_low_z[3] = { 0, 0, -10.0f };
    numcomp_test_vec3_print(v_low_z, &p2);
    for (int i = 0; i < 500; i++) {
        float v[3] = {
                numcomp_float_random_range(-2.0f, +2.0f),
                numcomp_float_random_range(-2.0f, +2.0f),
                numcomp_float_random_range(+0.0f, +5.1f),
        };
        numcomp_test_vec3_print(v, &p2);
    }
}

void numcomp_batch_test_float() {
    LWNUMCOMPFLOATPRESET p1;
    numcomp_init_float_preset(&p1, 11, -2.0f, 2.0f);
    numcomp_test_float_print(-100.0f, &p1);
    float slightly_smaller_than_M = p1.M - p1.s * 0.1f;
    numcomp_test_float_print(slightly_smaller_than_M, &p1);
    for (int i = 0; i < 500; i++) {
        numcomp_test_float_print(p1.m + 0.0123f * i, &p1);
    }
    // small test
    {
        LWNUMCOMPFLOATPRESET p2;
        numcomp_init_float_preset(&p2, 2, -2.0f, 2.0f);
        numcomp_test_float_print(1.99f, &p2);
        numcomp_test_float_print(-1.4f, &p2);
        numcomp_test_float_print(-1.2f, &p2);
    }
}

unsigned int numcomp_compress_quaternion(const float q0[4], const LWNUMCOMPQUATERNIONPRESET* preset) {
    // abs_max_idx  smallest three components
    // [  2-bit  ][  10-bit  ][  10-bit  ][  10-bit  ]
    float q[4];
    memcpy(q, q0, sizeof(float) * 4);
//    int all_negative = 1;
//    for (int i = 0; i < 4; i++) {
//        if (q[i] > 0) {
//            all_negative = 0;
//            break;
//        }
//    }
//    if (all_negative) {
//        for (int i = 0; i < 4; i++) {
//            q[i] *= -1;
//        }
//    }
    // Find abs max component index
    int abs_max_idx = 0;
    float abs_max = fabsf(q[0]);
    int abs_max_negative = q[0] < 0;
    for (int i = 1; i < 4; i++) {
        if (abs_max < fabsf(q[i])) {
            abs_max = fabsf(q[i]);
            abs_max_idx = i;
            abs_max_negative = q[i] < 0;
        }
    }
    LOGIx("abs_max_idx = %d (negative = %d)", abs_max_idx, abs_max_negative);
    if (abs_max_negative) {
        for (int i = 0; i < 4; i++) {
            q[i] *= -1;
        }
    }
    float smallest_threes[3];
    int st_index = 0;
    for (int i = 0; i < 4; i++) {
        if (i == abs_max_idx) {
            continue;
        }
        smallest_threes[st_index] = q[i];
        st_index++;
    }
    return numcomp_compress_vec3(smallest_threes, &preset->smallest_threes)
           | (abs_max_idx << preset->c_sum);
}

void numcomp_decompress_quaternion(float q[4], unsigned int c, const LWNUMCOMPQUATERNIONPRESET* preset) {
    int abs_max_idx = c >> preset->c_sum;
    unsigned int v_comp = c & ((1 << preset->c_sum) - 1);
    float smallest_threes[3];
    numcomp_decompress_vec3(smallest_threes, v_comp, &preset->smallest_threes);
    float q_sqr_sum = 0;
    int st_index = 0;
    for (int i = 0; i < 4; i++) {
        if (i == abs_max_idx) {
            continue;
        }

        q[i] = smallest_threes[st_index];
        q_sqr_sum += q[i] * q[i];
        st_index++;
    }
    q[abs_max_idx] = sqrtf(1.0f - q_sqr_sum);
}

// Three.js math
// q = [w, x, y, z]
void numcomp_convert_euler_xyz_to_quaternion(float *q, float ex, float ey, float ez) {
    float c1 = cosf( ex / 2.0f );
    float c2 = cosf( ey / 2.0f );
    float c3 = cosf( ez / 2.0f );
    float s1 = sinf( ex / 2.0f );
    float s2 = sinf( ey / 2.0f );
    float s3 = sinf( ez / 2.0f );
    q[0] = c1 * c2 * c3 - s1 * s2 * s3;
    q[1] = s1 * c2 * c3 + c1 * s2 * s3;
    q[2] = c1 * s2 * c3 - s1 * c2 * s3;
    q[3] = c1 * c2 * s3 + s1 * s2 * c3;
}

// Three.js math
// Rotation matrix --> XYZ Euler
void numcomp_convert_rotation_matrix_to_euler(float* ex, float* ey, float* ez, const mat4x4 m) {
    float m11 = m[0][0], m12 = m[1][0], m13 = m[2][0];
    float                m22 = m[1][1], m23 = m[2][1];
    float                m32 = m[1][2], m33 = m[2][2];
    *ey = asinf((float)LWMAX(LWMIN(m13, 1.0), -1.0));
    if (fabsf(m13) < 0.999999f) {
        *ex = atan2f(-m23, m33);
        *ez = atan2f(-m12, m11);
    } else {
        *ex = atan2f(m32, m22);
        *ez = 0;
    }
}

// https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
// http://www.andre-gaschler.com/rotationconverter/
// q = [w, x, y, z]
void numcomp_convert_quaternion_to_euler_xyz(const float q[4],
                                             float* ex,
                                             float* ey,
                                             float* ez) {
    mat4x4 m;
    mat4x4_from_quat(m, q);
    numcomp_convert_rotation_matrix_to_euler(ex, ey, ez, m);
}

/* change to `float/fmodf` or `long float/fmodl` or `int/%` as appropriate */

/* wrap x -> [0,max) */
float numcomp_wrap_max(float x, float max) {
    /* integer math: `(max + x % max) % max` */
    return fmodf(max + fmodf(x, max), max);
}

/* wrap x -> [min,max) */
float numcomp_wrap_min_max(float x, float min, float max) {
    return min + numcomp_wrap_max(x - min, max - min);
}

float numcomp_wrap_radian(float r) {
    return numcomp_wrap_min_max(r, (float) -M_PI, (float) M_PI);
}

void numcomp_test_quaternion(float ex, float ey, float ez, const LWNUMCOMPQUATERNIONPRESET* preset) {
    if (ex) {
        ex = numcomp_wrap_radian(ex);
    }
    if (ey) {
        ey = numcomp_wrap_radian(ey);
    }
    if (ez) {
        ez = numcomp_wrap_radian(ez);
    }
    float q[4];
    numcomp_convert_euler_xyz_to_quaternion(q, ex, ey, ez);
    float ex_copy, ey_copy, ez_copy;
    numcomp_convert_quaternion_to_euler_xyz(q, &ex_copy, &ey_copy, &ez_copy);

    int q_comp = numcomp_compress_quaternion(q, preset);
    float q_decomp[4], q_decomp_neg[4];
    numcomp_decompress_quaternion(q_decomp, q_comp, preset);
    for (int i = 0; i < 4; i++) {
        q_decomp_neg[i] = -q_decomp[i];
    }
    float ex_decomp, ey_decomp, ez_decomp;
    float ex_decomp_neg, ey_decomp_neg, ez_decomp_neg;
    numcomp_convert_quaternion_to_euler_xyz(q_decomp, &ex_decomp, &ey_decomp, &ez_decomp);
    numcomp_convert_quaternion_to_euler_xyz(q_decomp_neg, &ex_decomp_neg, &ey_decomp_neg, &ez_decomp_neg);
    float ex_diff = ex - ex_decomp;
    float ey_diff = ey - ey_decomp;
    float ez_diff = ez - ez_decomp;
    float err = sqrtf(ex_diff*ex_diff + ey_diff*ey_diff + ez_diff*ez_diff);
    LOGI("quat ex-ey-ez:(%f,%f,%f) --- quaternion:(%f,%f,%f,%f)",
         ex, ey, ez,
         q[0], q[1], q[2], q[3]);
    LOGI("quat ex-ey-ez:(%f,%f,%f) --- quaternion:(%f,%f,%f,%f) <decompressed>",
         ex_decomp, ey_decomp, ez_decomp,
         q_decomp[0], q_decomp[1], q_decomp[2], q_decomp[3]);
    LOGI("quat ex-ey-ez:(%f,%f,%f) --- quaternion:(%f,%f,%f,%f) <decompressed> (negative)",
         ex_decomp_neg, ey_decomp_neg, ez_decomp_neg,
         q_decomp_neg[0], q_decomp_neg[1], q_decomp_neg[2], q_decomp_neg[3]);
    LOGI("quat ex-ey-ez: error %f",
         err);
}

void numcomp_init_quaternion_preset(LWNUMCOMPQUATERNIONPRESET* preset) {
    float m = 1.0f / sqrtf(2);
    int c = 10;
    numcomp_init_vec3_preset(&preset->smallest_threes,
                             c, -m, m,
                             c, -m, m,
                             c, -m, m);
    preset->c_sum = c + c + c;
}

void numcomp_test_conversion(float ex, float ey, float ez) {
    float q[4];
    numcomp_convert_euler_xyz_to_quaternion(q, ex, ey, ez);
    LOGI("----------------");
    LOGI("pry: %f d %f d %f d ---> quat %f %f %f %f",
         LWRAD2DEG(ex), LWRAD2DEG(ey), LWRAD2DEG(ez), q[0], q[1], q[2], q[3]);
    float ex_r, ey_r, ez_r;
    numcomp_convert_quaternion_to_euler_xyz(q, &ex_r, &ey_r, &ez_r);
    LOGI("quat %f %f %f %f ---> pry: %f d %f d %f d",
         q[0], q[1], q[2], q[3], LWRAD2DEG(ex_r), LWRAD2DEG(ey_r), LWRAD2DEG(ez_r));
}

void numcomp_batch_test_conversion() {
    numcomp_test_conversion(0, 0, 0);
    numcomp_test_conversion((float)M_PI, 0, 0);
    numcomp_test_conversion(0, (float)M_PI, 0);
    numcomp_test_conversion(0, 0, (float)M_PI);
    numcomp_test_conversion(-(float)M_PI, 0, 0);
    numcomp_test_conversion(0, -(float)M_PI, 0);
    numcomp_test_conversion(0, 0, -(float)M_PI);
    numcomp_test_conversion(2.566371f,-1.150444f,-1.415926f);
}

void numcomp_batch_test_quaternion() {
    LWNUMCOMPQUATERNIONPRESET preset;
    numcomp_init_quaternion_preset(&preset);

    float zeros[3] = {-0.000000119209304f,};
    unsigned int zeros_comp = numcomp_compress_vec3(zeros, &preset.smallest_threes);
    float zeros_decomp[3];
    numcomp_decompress_vec3(zeros_decomp, zeros_comp, &preset.smallest_threes);

    numcomp_test_quaternion(+ 0.000f, + 0.000f,  0.00f, &preset);
    numcomp_test_quaternion(+ 1.000f, + 0.000f,  0.00f, &preset);
    numcomp_test_quaternion(+ 0.000f, + 1.000f,  0.00f, &preset);
    numcomp_test_quaternion(+ 0.000f, + 0.000f,  1.00f, &preset);
    numcomp_test_quaternion(+ 1.000f, + 2.000f,  3.00f, &preset);
    numcomp_test_quaternion(+10.000f, -20.000f, 30.00f, &preset); //***
    numcomp_test_quaternion(-10.000f, -20.000f, 30.00f, &preset);
    numcomp_test_quaternion(+ 1.234f, - 0.051f,  2.99f, &preset);
    numcomp_test_quaternion(+ 1.234f, - 3.100f,  0.00f, &preset);
    numcomp_test_quaternion(- 1.234f, - 3.100f,  0.00f, &preset);
    numcomp_test_quaternion(- 1.234f, - 3.100f,  3.00f, &preset);
    numcomp_test_quaternion(- 1.234f, - 3.100f, 30.00f, &preset);
}

void numcomp_test_mat4x4(float ex, float ey, float ez, const LWNUMCOMPQUATERNIONPRESET* preset) {
    if (ex) {
        ex = numcomp_wrap_radian(ex);
    }
    if (ey) {
        ey = numcomp_wrap_radian(ey);
    }
    if (ez) {
        ez = numcomp_wrap_radian(ez);
    }
    float q[4];
    numcomp_convert_euler_xyz_to_quaternion(q, ex, ey, ez);
    float ex_copy, ey_copy, ez_copy;
    numcomp_convert_quaternion_to_euler_xyz(q, &ex_copy, &ey_copy, &ez_copy);

    mat4x4 m;
    mat4x4_from_quat(m, q);
    int m_comp = numcomp_compress_mat4x4(m, preset);
    mat4x4 m_decomp;
    numcomp_decompress_mat4x4(m_decomp, m_comp, preset);
    quat q_decomp;
    quat_from_mat4x4(q_decomp, m_decomp);

    float ex_decomp, ey_decomp, ez_decomp;
    numcomp_convert_quaternion_to_euler_xyz(q_decomp, &ex_decomp, &ey_decomp, &ez_decomp);
    float ex_diff = ex - ex_decomp;
    float ey_diff = ey - ey_decomp;
    float ez_diff = ez - ez_decomp;
    float err = sqrtf(ex_diff*ex_diff + ey_diff*ey_diff + ez_diff*ez_diff);
    LOGI("mat4x4 ex-ey-ez:(%f,%f,%f) --- quaternion:(%f,%f,%f,%f)",
         ex, ey, ez,
         q[0], q[1], q[2], q[3]);
    LOGI("mat4x4 ex-ey-ez:(%f,%f,%f) --- quaternion:(%f,%f,%f,%f) <decompressed>",
         ex_decomp, ey_decomp, ez_decomp,
         q_decomp[0], q_decomp[1], q_decomp[2], q_decomp[3]);
    LOGI("mat4x4 ex-ey-ez: error %f",
         err);
}

void numcomp_batch_test_mat4x4() {
    LWNUMCOMPQUATERNIONPRESET preset;
    numcomp_init_quaternion_preset(&preset);
    numcomp_test_mat4x4(+ 0.000f, + 0.000f,  0.00f, &preset);
    numcomp_test_mat4x4(+ 1.000f, + 0.000f,  0.00f, &preset);
    numcomp_test_mat4x4(+ 0.000f, + 1.000f,  0.00f, &preset);
    numcomp_test_mat4x4(+ 0.000f, + 0.000f,  1.00f, &preset);
    numcomp_test_mat4x4(+ 1.000f, + 2.000f,  3.00f, &preset);
    numcomp_test_mat4x4(+10.000f, -20.000f, 30.00f, &preset); //***
    numcomp_test_mat4x4(-10.000f, -20.000f, 30.00f, &preset);
    numcomp_test_mat4x4(+ 1.234f, - 0.051f,  2.99f, &preset);
    numcomp_test_mat4x4(+ 1.234f, - 3.100f,  0.00f, &preset);
    numcomp_test_mat4x4(- 1.234f, - 3.100f,  0.00f, &preset);
    numcomp_test_mat4x4(- 1.234f, - 3.100f,  3.00f, &preset);
    numcomp_test_mat4x4(- 1.234f, - 3.100f, 30.00f, &preset);
}

unsigned int numcomp_compress_mat4x4(const mat4x4 m, const LWNUMCOMPQUATERNIONPRESET* preset) {
    quat q;
    quat_from_mat4x4(q, m);
    return numcomp_compress_quaternion(q, preset);
}

void numcomp_decompress_mat4x4(mat4x4 m, unsigned int v_comp, const LWNUMCOMPQUATERNIONPRESET* preset) {
    quat q;
    numcomp_decompress_quaternion(q, v_comp, preset);
    mat4x4_from_quat(m, q);
}
