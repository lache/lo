#include "render_dynamic_vbo.h"
#include "lwgl.h"
#include <math.h>
#include "lwcontext.h"
#include "vertices.h"
#include "laidoff.h"
#include "render_solid.h"
#include "lwmacro.h"

static GLuint dynamic_vbo = 0;
static GLuint dynamic_vao = 0;
static LWVERTEX square[] = {
    //x     y     z  r  g  b  u  v  s  s
    { -1.f, -1.f, 0, 1, 0, 0, 0, 1, 0, 0 },
    { +1.f, -1.f, 0, 0, 1, 0, 1, 1, 0, 0 },
    { +1.f, +1.f, 0, 0, 0, 1, 1, 0, 0, 0 },
    { +1.f, +1.f, 0, 1, 0, 0, 1, 0, 0, 0 },
    { -1.f, +1.f, 0, 0, 1, 0, 0, 0, 0, 0 },
    { -1.f, -1.f, 0, 0, 0, 1, 0, 1, 0, 0 },
};

void lwc_init_dynamic_vbo(LWCONTEXT* pLwc) {
    if (dynamic_vao == 0) {
        glGenBuffers(1, &dynamic_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, dynamic_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(square), square, GL_DYNAMIC_DRAW);
#if LW_SUPPORT_VAO
        glGenVertexArrays(1, &dynamic_vao);
        glBindVertexArray(dynamic_vao);
        set_vertex_attrib_pointer(pLwc, LWST_DEFAULT);
#endif
    }
}

static void update_vbo(const LWCONTEXT* pLwc) {
    const float v = sinf((float)pLwc->app_time);
    square[0].x = v;
    square[2].y = v;
    square[3].x = v;
    square[5].y = v;
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(square), square);
}

void lwc_render_dynamic_vbo(const LWCONTEXT* pLwc) {
    LW_GL_VIEWPORT();
    lw_clear_color();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int shader_index = LWST_DEFAULT;
    float alpha_multiplier = 1.0f;
    float over_r = 1.0f;
    float over_g = 1.0f;
    float over_b = 1.0f;
    float oratio = 0.0f;
    const vec4* proj = pLwc->proj;
    float sx = 1.0f;
    float sy = 1.0f;
    float sz = 1.0f;
    float rot_z = 0;
    mat4x4 view;
    mat4x4_identity(view);
    float x = 0;
    float y = 0;
    float z = 0;
    GLsizei vertex_count = sizeof(square) / sizeof(square[0]);

    const LWSHADER* shader = &pLwc->shader[shader_index];
    lazy_glUseProgram(pLwc, shader_index);
    glUniform2fv(shader->vuvoffset_location, 1, default_uv_offset);
    glUniform2fv(shader->vuvscale_location, 1, default_uv_scale);
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

    glBindBuffer(GL_ARRAY_BUFFER, dynamic_vbo); //lazy_glBindBuffer(pLwc, lvt);
#if LW_SUPPORT_VAO
    glBindVertexArray(dynamic_vao);
#else
    set_vertex_attrib_pointer(pLwc, shader_index);
#endif
    lazy_tex_atlas_glBindTexture(pLwc, LAE_TTL_CITY);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glDrawArrays(GL_TRIANGLES, 0, vertex_count);

    update_vbo(pLwc);
}
