#pragma once
#include "linmath.h"

typedef struct _LWSOLID {
    float x;
    float y;
    float z;
    float sx;
    float sy;
    float sz;
    unsigned int tex_index;
    unsigned int tex_alpha_index;
    int lvt;
    float alpha_multiplier;
    float over_r;
    float over_g;
    float over_b;
    float oratio;
    float uv_offset[2];
    float uv_scale[2];
    int shader_index;
    float rot_z;
    mat4x4 view;
    mat4x4 proj;
} LWSOLID;

#ifdef __cplusplus
extern "C" {;
#endif
void lwsolid_default(LWSOLID* solid);

void lwsolid_make_command_box_ui_lvt_flip_y_uv(LWSOLID* solid,
                                               float x,
                                               float y,
                                               float w,
                                               float h,
                                               unsigned int tex_index,
                                               int lvt,
                                               int flip_y_uv);
void lwsolid_make_command_box_ui(LWSOLID* solid,
                                 float x,
                                 float y,
                                 float w,
                                 float h,
                                 unsigned int tex_index);
void lwsolid_make_command_box_ui_alpha(LWSOLID* solid,
                                       float x,
                                       float y,
                                       float w,
                                       float h,
                                       unsigned int tex_index,
                                       float alpha_multiplier);
void lwsolid_make_command_vb_ui(LWSOLID* solid,
                                float x,
                                float y,
                                float w,
                                float h,
                                unsigned int tex_index,
                                int lvt,
                                float alpha_multiplier,
                                float over_r,
                                float over_g,
                                float over_b,
                                float oratio);
void lwsolid_make_command_vb_ui_alpha(LWSOLID* solid,
                                      float x,
                                      float y,
                                      float w,
                                      float h,
                                      unsigned int tex_index,
                                      unsigned int tex_alpha_index,
                                      int lvt,
                                      float alpha_multiplier,
                                      float over_r,
                                      float over_g,
                                      float over_b,
                                      float oratio);
void lwsolid_make_command_vb_ui_flip_y_uv(LWSOLID* solid,
                                          float x,
                                          float y,
                                          float w,
                                          float h,
                                          unsigned int tex_index,
                                          int lvt,
                                          float alpha_multiplier,
                                          float over_r,
                                          float over_g,
                                          float over_b,
                                          float oratio,
                                          int flip_y_uv);
void lwsolid_make_command_vb_ui_flip_y_uv_shader(LWSOLID* solid,
                                                 float x,
                                                 float y,
                                                 float w,
                                                 float h,
                                                 unsigned int tex_index,
                                                 int lvt,
                                                 float alpha_multiplier,
                                                 float over_r,
                                                 float over_g,
                                                 float over_b,
                                                 float oratio,
                                                 int flip_y_uv,
                                                 int shader_index);
void lwsolid_make_command_vb_ui_flip_y_uv_shader_rot(LWSOLID* solid,
                                                     float x,
                                                     float y,
                                                     float w,
                                                     float h,
                                                     unsigned int tex_index,
                                                     int lvt,
                                                     float alpha_multiplier,
                                                     float over_r,
                                                     float over_g,
                                                     float over_b,
                                                     float oratio,
                                                     int flip_y_uv,
                                                     int shader_index,
                                                     float rot);
void lwsolid_make_command_vb_ui_alpha_uv(LWSOLID* solid,
                                         float x,
                                         float y,
                                         float w,
                                         float h,
                                         unsigned int tex_index,
                                         unsigned int tex_alpha_index,
                                         int lvt,
                                         float alpha_multiplier,
                                         float over_r,
                                         float over_g,
                                         float over_b,
                                         float oratio,
                                         const float* uv_offset,
                                         const float* uv_scale);
void lwsolid_make_command_vb_ui_alpha_uv_shader_view_proj(LWSOLID* solid,
                                                          float x,
                                                          float y,
                                                          float w,
                                                          float h,
                                                          unsigned int tex_index,
                                                          unsigned int tex_alpha_index,
                                                          int lvt,
                                                          float alpha_multiplier,
                                                          float over_r,
                                                          float over_g,
                                                          float over_b,
                                                          float oratio,
                                                          const float* uv_offset,
                                                          const float* uv_scale,
                                                          int shader_index,
                                                          const mat4x4 view,
                                                          const mat4x4 proj);
void lwsolid_make_command_vb_ui_alpha_uv_shader(LWSOLID* solid,
                                                float x,
                                                float y,
                                                float w,
                                                float h,
                                                unsigned int tex_index,
                                                unsigned int tex_alpha_index,
                                                int lvt,
                                                float alpha_multiplier,
                                                float over_r,
                                                float over_g,
                                                float over_b,
                                                float oratio,
                                                const float* uv_offset,
                                                const float* uv_scale,
                                                int shader_index);
void lwsolid_make_command_vb_ui_uv_shader_rot(LWSOLID* solid,
                                              float x,
                                              float y,
                                              float w,
                                              float h,
                                              unsigned int tex_index,
                                              int lvt,
                                              float alpha_multiplier,
                                              float over_r,
                                              float over_g,
                                              float over_b,
                                              float oratio,
                                              const float* uv_offset,
                                              const float* uv_scale,
                                              int shader_index,
                                              float rot);
void lwsolid_make_command_vb_ui_uv_shader_rot_view_proj(LWSOLID* solid,
                                                        float x,
                                                        float y,
                                                        float w,
                                                        float h,
                                                        unsigned int tex_index,
                                                        int lvt,
                                                        float alpha_multiplier,
                                                        float over_r,
                                                        float over_g,
                                                        float over_b,
                                                        float oratio,
                                                        const float* uv_offset,
                                                        const float* uv_scale,
                                                        int shader_index,
                                                        float rot_z,
                                                        const mat4x4 view,
                                                        const mat4x4 proj);
void lwsolid_make_command_vb_uv_shader_rot_view_proj(LWSOLID* solid,
                                                     float x,
                                                     float y,
                                                     float z,
                                                     float sx,
                                                     float sy,
                                                     float sz,
                                                     unsigned int tex_index,
                                                     int lvt,
                                                     float alpha_multiplier,
                                                     float over_r,
                                                     float over_g,
                                                     float over_b,
                                                     float oratio,
                                                     const float* uv_offset,
                                                     const float* uv_scale,
                                                     int shader_index,
                                                     float rot_z,
                                                     const mat4x4 view,
                                                     const mat4x4 proj);
void lwsolid_make_command_general(LWSOLID* solid,
                                  float x,
                                  float y,
                                  float z,
                                  float sx,
                                  float sy,
                                  float sz,
                                  unsigned int tex_index,
                                  unsigned int tex_alpha_index,
                                  int lvt,
                                  float alpha_multiplier,
                                  float over_r,
                                  float over_g,
                                  float over_b,
                                  float oratio,
                                  const float* uv_offset,
                                  const float* uv_scale,
                                  int shader_index,
                                  float rot_z,
                                  const mat4x4 view,
                                  const mat4x4 proj);
#ifdef __cplusplus
}
#endif
