#include <string.h>
#include <assert.h>
#include "lwsolid.h"
#include "lwshader.h"
#include "lwvbotype.h"

const static float default_uv_offset[2] = { 0, 0 };
const static float default_uv_scale[2] = { 1, 1 };
const static float default_flip_y_uv_scale[2] = { 1, -1 };

void lwsolid_default(LWSOLID* solid) {
    memset(solid, 0, sizeof(LWSOLID));
    solid->sx = solid->sy = solid->sz = 1.0f;
    solid->lvt = LVT_CENTER_CENTER_ANCHORED_SQUARE;
    solid->alpha_multiplier = 1.0f;
    solid->uv_scale[0] = solid->uv_scale[1] = 1.0f;
    solid->shader_index = LWST_DEFAULT;
    mat4x4_identity(solid->view);
    mat4x4_identity(solid->proj);
}

void lwsolid_make_command_box_ui_lvt_flip_y_uv(LWSOLID* solid,
                                               float x,
                                               float y,
                                               float w,
                                               float h,
                                               unsigned int tex_index,
                                               int lvt,
                                               int flip_y_uv) {
    lwsolid_make_command_vb_ui_flip_y_uv(solid,
                                         x,
                                         y,
                                         w,
                                         h,
                                         tex_index,
                                         lvt,
                                         1,
                                         1,
                                         1,
                                         1,
                                         0,
                                         flip_y_uv);
}

void lwsolid_make_command_box_ui(LWSOLID* solid,
                                 float x,
                                 float y,
                                 float w,
                                 float h,
                                 unsigned int tex_index) {
    lwsolid_make_command_vb_ui(solid,
                               x,
                               y,
                               w,
                               h,
                               tex_index,
                               LVT_LEFT_BOTTOM_ANCHORED_SQUARE,
                               1,
                               1,
                               1,
                               1,
                               0);
}

void lwsolid_make_command_box_ui_alpha(LWSOLID* solid,
                                       float x,
                                       float y,
                                       float w,
                                       float h,
                                       unsigned int tex_index,
                                       float alpha_multiplier) {
    lwsolid_make_command_vb_ui(solid,
                               x,
                               y,
                               w,
                               h,
                               tex_index,
                               LVT_LEFT_BOTTOM_ANCHORED_SQUARE,
                               alpha_multiplier,
                               1,
                               1,
                               1,
                               0);
}

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
                                          int flip_y_uv) {
    int shader_index = LWST_DEFAULT;
    lwsolid_make_command_vb_ui_flip_y_uv_shader(solid,
                                                x,
                                                y,
                                                w,
                                                h,
                                                tex_index,
                                                lvt,
                                                alpha_multiplier,
                                                over_r,
                                                over_g,
                                                over_b,
                                                oratio,
                                                flip_y_uv,
                                                shader_index);
}

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
                                                 int shader_index) {
    lwsolid_make_command_vb_ui_flip_y_uv_shader_rot(solid,
                                                    x,
                                                    y,
                                                    w,
                                                    h,
                                                    tex_index,
                                                    lvt,
                                                    alpha_multiplier,
                                                    over_r,
                                                    over_g,
                                                    over_b,
                                                    oratio,
                                                    flip_y_uv,
                                                    shader_index,
                                                    0);
}

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
                                                     float rot) {
    lwsolid_make_command_vb_ui_uv_shader_rot(solid,
                                             x,
                                             y,
                                             w,
                                             h,
                                             tex_index,
                                             lvt,
                                             alpha_multiplier,
                                             over_r,
                                             over_g,
                                             over_b,
                                             oratio,
                                             default_uv_offset,
                                             flip_y_uv ? default_flip_y_uv_scale : default_uv_scale,
                                             shader_index,
                                             rot);
}

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
                                              float rot) {
    assert(0);
    //mat4x4 identity_view;
    //mat4x4_identity(identity_view);
    //lwsolid_make_command_vb_ui_uv_shader_rot_view_proj(solid,
    //                                           x,
    //                                           y,
    //                                           w,
    //                                           h,
    //                                           tex_index,
    //                                           lvt,
    //                                           alpha_multiplier,
    //                                           over_r,
    //                                           over_g,
    //                                           over_b,
    //                                           oratio,
    //                                           uv_offset,
    //                                           uv_scale,
    //                                           shader_index,
    //                                           rot,
    //                                           identity_view,
    //                                           pLwc->proj);
}

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
                                                     const mat4x4 proj) {
    lwsolid_make_command_general(solid,
                                 x,
                                 y,
                                 z,
                                 sx,
                                 sy,
                                 sz,
                                 tex_index,
                                 0,
                                 lvt,
                                 alpha_multiplier,
                                 over_r,
                                 over_g,
                                 over_b,
                                 oratio,
                                 uv_offset,
                                 uv_scale,
                                 shader_index,
                                 rot_z,
                                 view,
                                 proj);
}

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
                                                        const mat4x4 proj) {
    const float sx = w / 2;
    const float sy = h / 2;
    const float sz = 1.0f;
    lwsolid_make_command_vb_uv_shader_rot_view_proj(solid,
                                                    x,
                                                    y,
                                                    0,
                                                    sx,
                                                    sy,
                                                    sz,
                                                    tex_index,
                                                    lvt,
                                                    alpha_multiplier,
                                                    over_r,
                                                    over_g,
                                                    over_b,
                                                    oratio,
                                                    uv_offset,
                                                    uv_scale,
                                                    shader_index,
                                                    rot_z,
                                                    view,
                                                    proj);
}

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
                                      float oratio) {
    lwsolid_make_command_vb_ui_alpha_uv(solid,
                                        x,
                                        y,
                                        w,
                                        h,
                                        tex_index,
                                        tex_alpha_index,
                                        lvt,
                                        alpha_multiplier,
                                        over_r,
                                        over_g,
                                        over_b,
                                        oratio,
                                        default_uv_offset,
                                        default_uv_scale);
}

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
                                         const float* uv_scale) {
    int shader_index = LWST_ETC1;
    lwsolid_make_command_vb_ui_alpha_uv_shader(solid,
                                               x,
                                               y,
                                               w,
                                               h,
                                               tex_index,
                                               tex_alpha_index,
                                               lvt,
                                               alpha_multiplier,
                                               over_r,
                                               over_g,
                                               over_b,
                                               oratio,
                                               uv_offset,
                                               uv_scale,
                                               shader_index);
}

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
                                                          const mat4x4 proj) {
    const float sx = w / 2;
    const float sy = h / 2;
    lwsolid_make_command_general(solid,
                                 x,
                                 y,
                                 0,
                                 sx,
                                 sy,
                                 1.0f,
                                 tex_index,
                                 tex_alpha_index,
                                 lvt,
                                 alpha_multiplier,
                                 over_r,
                                 over_g,
                                 over_b,
                                 oratio,
                                 uv_offset,
                                 uv_scale,
                                 shader_index,
                                 0,
                                 view,
                                 proj);
}

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
                                                int shader_index) {
    assert(0);
    /*mat4x4 identity_view;
    mat4x4_identity(identity_view);
    lwsolid_make_command_vb_ui_alpha_uv_shader_view_proj(solid, x, y, w, h, tex_index, tex_alpha_index, lvt, alpha_multiplier, over_r, over_g, over_b, oratio, uv_offset, uv_scale, shader_index, identity_view, pLwc->proj);*/
}

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
                                float oratio) {
    lwsolid_make_command_vb_ui_flip_y_uv(solid,
                                         x,
                                         y,
                                         w,
                                         h,
                                         tex_index,
                                         lvt,
                                         alpha_multiplier,
                                         over_r,
                                         over_g,
                                         over_b,
                                         oratio,
                                         0);
}

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
                                  const mat4x4 proj) {
    solid->x = x;
    solid->y = y;
    solid->z = z;
    solid->sx = sx;
    solid->sy = sy;
    solid->sz = sz;
    solid->tex_index = tex_index;
    solid->tex_alpha_index = tex_alpha_index;
    solid->lvt = lvt;
    solid->alpha_multiplier = alpha_multiplier;
    solid->over_r = over_r;
    solid->over_g = over_g;
    solid->over_b = over_b;
    solid->oratio = oratio;
    memcpy(solid->uv_offset, uv_offset, sizeof(float) * 2);
    memcpy(solid->uv_scale, uv_scale, sizeof(float) * 2);
    solid->shader_index = shader_index;
    solid->rot_z = rot_z;
    memcpy(solid->view, view, sizeof(mat4x4));
    memcpy(solid->proj, proj, sizeof(mat4x4));
}
