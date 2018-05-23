#include "lwcontext.h"
#include "render_morph.h"
#include "laidoff.h"
#include "lwlog.h"
#include "lwgl.h"
#include "lwcontext.h"
#include <assert.h>
#include <string.h>

void render_morph(const LWCONTEXT* pLwc,
                  GLuint tex_index,
                  LW_MORPH_VBO_TYPE lmvt,
                  float alpha_multiplier,
                  float over_r,
                  float over_g,
                  float over_b,
                  float oratio,
                  const mat4x4 proj,
                  const mat4x4 view,
                  const mat4x4 model,
                  const float* uv_offset,
                  const float* uv_scale,
                  const float earth_globe_morph_weight) {

    int shader_index = LWST_MORPH;

    lazy_glUseProgram(pLwc, shader_index);
    glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, uv_offset);
    glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, uv_scale);
    glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, alpha_multiplier);
    glUniform1i(pLwc->shader[shader_index].diffuse_location, 0); // 0 means GL_TEXTURE0
    glUniform1i(pLwc->shader[shader_index].alpha_only_location, 1); // 1 means GL_TEXTURE1
    glUniform3f(pLwc->shader[shader_index].overlay_color_location, over_r, over_g, over_b);
    glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, oratio);
    glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj);
    glUniform1f(pLwc->shader[shader_index].morph_weight, earth_globe_morph_weight);

    mat4x4 view_model;
    mat4x4 proj_view_model;

    mat4x4_mul(view_model, view, model);
    mat4x4_mul(proj_view_model, proj, view_model);

    glBindBuffer(GL_ARRAY_BUFFER, pLwc->morph_vertex_buffer[lmvt].vertex_buffer);
    bind_all_morph_vertex_attrib(pLwc, lmvt);
    glActiveTexture(GL_TEXTURE0);
    assert(tex_index);
    glBindTexture(GL_TEXTURE_2D, tex_index);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
    glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->morph_vertex_buffer[lmvt].vertex_count);

    glActiveTexture(GL_TEXTURE0);
}
