#include "lwcontext.h"
#include "render_solid.h"
#include "laidoff.h"
#include "lwanim.h"
#include "lwlog.h"
#include <assert.h>

void render_solid_box_ui_lvt_flip_y_uv(const LWCONTEXT* pLwc,
                                       float x,
                                       float y,
                                       float w,
                                       float h,
                                       GLuint tex_index,
                                       enum _LW_VBO_TYPE lvt,
                                       int flip_y_uv) {
    render_solid_vb_ui_flip_y_uv(pLwc,
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

void render_solid_box_ui(const LWCONTEXT* pLwc,
                         float x,
                         float y,
                         float w,
                         float h,
                         GLuint tex_index) {
    render_solid_vb_ui(pLwc,
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

void render_solid_box_ui_alpha(const LWCONTEXT* pLwc,
                               float x,
                               float y,
                               float w,
                               float h,
                               GLuint tex_index,
                               float alpha_multiplier) {
    render_solid_vb_ui(pLwc,
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

void render_solid_vb_ui_flip_y_uv(const LWCONTEXT* pLwc,
                                  float x,
                                  float y,
                                  float w,
                                  float h,
                                  GLuint tex_index,
                                  enum _LW_VBO_TYPE lvt,
                                  float alpha_multiplier,
                                  float over_r,
                                  float over_g,
                                  float over_b,
                                  float oratio,
                                  int flip_y_uv) {
    int shader_index = LWST_DEFAULT;
    render_solid_vb_ui_flip_y_uv_shader(pLwc,
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

void render_solid_vb_ui_flip_y_uv_shader(const LWCONTEXT* pLwc,
                                         float x,
                                         float y,
                                         float w,
                                         float h,
                                         GLuint tex_index,
                                         enum _LW_VBO_TYPE lvt,
                                         float alpha_multiplier,
                                         float over_r,
                                         float over_g,
                                         float over_b,
                                         float oratio,
                                         int flip_y_uv,
                                         int shader_index) {
    render_solid_vb_ui_flip_y_uv_shader_rot(pLwc,
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

void render_solid_vb_ui_flip_y_uv_shader_rot(const LWCONTEXT* pLwc,
                                             float x,
                                             float y,
                                             float w,
                                             float h,
                                             GLuint tex_index,
                                             enum _LW_VBO_TYPE lvt,
                                             float alpha_multiplier,
                                             float over_r,
                                             float over_g,
                                             float over_b,
                                             float oratio,
                                             int flip_y_uv,
                                             int shader_index,
                                             float rot) {
    render_solid_vb_ui_uv_shader_rot(pLwc,
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

void render_solid_vb_ui_uv_shader_rot(const LWCONTEXT* pLwc,
                                      float x,
                                      float y,
                                      float w,
                                      float h,
                                      GLuint tex_index,
                                      enum _LW_VBO_TYPE lvt,
                                      float alpha_multiplier,
                                      float over_r,
                                      float over_g,
                                      float over_b,
                                      float oratio,
                                      const float* uv_offset,
                                      const float* uv_scale,
                                      int shader_index,
                                      float rot) {
    mat4x4 identity_view;
    mat4x4_identity(identity_view);
    render_solid_vb_ui_uv_shader_rot_view_proj(pLwc,
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
                                               uv_offset,
                                               uv_scale,
                                               shader_index,
                                               rot,
                                               identity_view,
                                               pLwc->proj);
}

void render_solid_vb_uv_shader_rot_view_proj(const LWCONTEXT* pLwc,
                                             float x,
                                             float y,
                                             float z,
                                             float sx,
                                             float sy,
                                             float sz,
                                             GLuint tex_index,
                                             enum _LW_VBO_TYPE lvt,
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
    render_solid_general(pLwc,
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

void render_solid_vb_ui_uv_shader_rot_view_proj(const LWCONTEXT* pLwc,
                                                float x,
                                                float y,
                                                float w,
                                                float h,
                                                GLuint tex_index,
                                                enum _LW_VBO_TYPE lvt,
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
    render_solid_vb_uv_shader_rot_view_proj(pLwc,
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

void render_solid_vb_ui_alpha(const LWCONTEXT* pLwc,
                              float x,
                              float y,
                              float w,
                              float h,
                              GLuint tex_index,
                              GLuint tex_alpha_index,
                              enum _LW_VBO_TYPE lvt,
                              float alpha_multiplier,
                              float over_r,
                              float over_g,
                              float over_b,
                              float oratio) {
    render_solid_vb_ui_alpha_uv(pLwc,
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

void render_solid_vb_ui_alpha_uv(const LWCONTEXT* pLwc,
                                 float x,
                                 float y,
                                 float w,
                                 float h,
                                 GLuint tex_index,
                                 GLuint tex_alpha_index,
                                 enum _LW_VBO_TYPE lvt,
                                 float alpha_multiplier,
                                 float over_r,
                                 float over_g,
                                 float over_b,
                                 float oratio,
                                 const float* uv_offset,
                                 const float* uv_scale) {
    int shader_index = LWST_ETC1;
    render_solid_vb_ui_alpha_uv_shader(pLwc,
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

void render_solid_vb_ui_alpha_uv_shader_view_proj(const LWCONTEXT* pLwc,
                                                  float x,
                                                  float y,
                                                  float w,
                                                  float h,
                                                  GLuint tex_index,
                                                  GLuint tex_alpha_index,
                                                  enum _LW_VBO_TYPE lvt,
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
    render_solid_general(pLwc,
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

void render_solid_vb_ui_alpha_uv_shader(const LWCONTEXT* pLwc,
                                        float x,
                                        float y,
                                        float w,
                                        float h,
                                        GLuint tex_index,
                                        GLuint tex_alpha_index,
                                        enum _LW_VBO_TYPE lvt,
                                        float alpha_multiplier,
                                        float over_r,
                                        float over_g,
                                        float over_b,
                                        float oratio,
                                        const float* uv_offset,
                                        const float* uv_scale,
                                        int shader_index) {
    mat4x4 identity_view;
    mat4x4_identity(identity_view);
    render_solid_vb_ui_alpha_uv_shader_view_proj(pLwc, x, y, w, h, tex_index, tex_alpha_index, lvt, alpha_multiplier, over_r, over_g, over_b, oratio, uv_offset, uv_scale, shader_index, identity_view, pLwc->proj);
}

void render_solid_vb_ui(const LWCONTEXT* pLwc,
                        float x,
                        float y,
                        float w,
                        float h,
                        GLuint tex_index,
                        enum _LW_VBO_TYPE lvt,
                        float alpha_multiplier,
                        float over_r,
                        float over_g,
                        float over_b,
                        float oratio) {
    render_solid_vb_ui_flip_y_uv(pLwc,
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

void lwc_enable_additive_blending() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
}

void lwc_disable_additive_blending() {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

void render_solid_general(const LWCONTEXT* pLwc,
                          float x,
                          float y,
                          float z,
                          float sx,
                          float sy,
                          float sz,
                          GLuint tex_index,
                          GLuint tex_alpha_index,
                          enum _LW_VBO_TYPE lvt,
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
    const LWSHADER* shader = &pLwc->shader[shader_index];
    lazy_glUseProgram(pLwc, shader_index);
    glUniform2fv(shader->vuvoffset_location, 1, uv_offset);
    glUniform2fv(shader->vuvscale_location, 1, uv_scale);
    glUniform2fv(shader->vs9offset_location, 1, default_uv_offset);
    glUniform1f(shader->alpha_multiplier_location, alpha_multiplier);
    glUniform1i(shader->diffuse_location, 0); // 0 means GL_TEXTURE0
    glUniform1i(shader->alpha_only_location, 1); // 1 means GL_TEXTURE1
    glUniform3f(shader->overlay_color_location, over_r, over_g, over_b);
    glUniform1f(shader->overlay_color_ratio_location, oratio);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj);

    mat4x4 model_translate;
    mat4x4 model;
    mat4x4 view_model;
    mat4x4 proj_view_model;
    mat4x4 model_scale;
    mat4x4 model_rotate;
    mat4x4 model_scale_rotate;

    mat4x4_identity(model_scale);
    mat4x4_identity(model_rotate);
    mat4x4_scale_aniso(model_scale, model_scale, sx, sy, sz);
    mat4x4_rotate_Z(model_rotate, model_rotate, rot_z);
    mat4x4_mul(model_scale_rotate, model_rotate, model_scale);
    mat4x4_translate(model_translate, x, y, z);
    mat4x4_identity(model);
    mat4x4_mul(model, model_translate, model_scale_rotate);
    mat4x4_mul(view_model, view, model);
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);

    lazy_glBindBuffer(pLwc, lvt);
    if (shader_index == LWST_ETC1) {
        bind_all_vertex_attrib_etc1_with_alpha(pLwc, lvt);
        assert(tex_index);
    } else {
        bind_all_vertex_attrib(pLwc, lvt);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_index);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
    //set_tex_filter(GL_NEAREST, GL_NEAREST);
    if (shader_index == LWST_ETC1) {
        glActiveTexture(GL_TEXTURE1);
        assert(tex_alpha_index);
        glBindTexture(GL_TEXTURE_2D, tex_alpha_index);
        set_tex_filter(GL_LINEAR, GL_LINEAR);
    } else {
        assert(tex_alpha_index == 0);
    }
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}
