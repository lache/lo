#include "lwfvbo.h"
#include "lwfanim.h"
#include "lwcontext.h"
#include "file.h"
#include <assert.h>
#include "laidoff.h"
#include "puckgame.h"
#include "render_puckgame.h"

void load_fvbo(LWCONTEXT* pLwc, const char* filename, LWFVBO* fvbo) {
    size_t file_size = 0;
    char* data = create_binary_from_file(filename, &file_size);
    LWFVBO_FILE_LAYOUT layout;
    layout.total_cell_object_count = ((LWFVBO_FILE_LAYOUT*)data)->total_cell_object_count;
    layout.num_vertices = (int*)(data + 4);
    layout.buffer_data = (float*)(data + 4 + layout.total_cell_object_count * 4);

    assert(layout.total_cell_object_count <= LWFVBO_MAX_CELL_COUNT);
    layout.total_cell_object_count = LWMIN(layout.total_cell_object_count, LWFVBO_MAX_CELL_COUNT);
    fvbo->total_vertex_count = 0;
    for (int i = 0; i < layout.total_cell_object_count; i++) {
        fvbo->total_vertex_count += layout.num_vertices[i];
    }
    memcpy(fvbo->vertex_count_per_cell, layout.num_vertices, sizeof(int) * layout.total_cell_object_count);

    GLuint vbo = 0;
    int buffer_size = lwvertex_stride_in_bytes * fvbo->total_vertex_count;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, buffer_size, layout.buffer_data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    fvbo->vertex_buffer = vbo;
    fvbo->total_cell_count = layout.total_cell_object_count;
    release_binary(data);
}

void render_fvbo(const LWCONTEXT* pLwc, const LWPUCKGAME* puck_game, const mat4x4 view, const mat4x4 proj,
                 LW_FVBO_TYPE lft, LW_F_ANIM_TYPE lfat, float x, float y, float z, float scale, float z_rot_angle,
                 float frame_time, int loop, float frames_per_sec) {

    int shader_index = LWST_DEFAULT_NORMAL;
    const LWSHADER* shader = &pLwc->shader[shader_index];
    lazy_glUseProgram(pLwc, shader_index);
    glUniform2fv(shader->vuvoffset_location, 1, default_uv_offset);
    glUniform2fv(shader->vuvscale_location, 1, default_uv_scale);
    glUniform2fv(shader->vs9offset_location, 1, default_uv_offset);
    glUniform1f(shader->alpha_multiplier_location, 1.0f);
    glUniform1i(shader->diffuse_location, 0); // 0 means GL_TEXTURE0
    glUniform1i(shader->alpha_only_location, 1); // 1 means GL_TEXTURE1
    glUniform3f(shader->overlay_color_location, 1, 1, 1);
    glUniform1f(shader->overlay_color_ratio_location, 0);

    float sx = scale;
    float sy = scale;
    float sz = scale;

    glUniform3f(shader->overlay_color_location, 1, 0, 0);
    glUniform3f(shader->multiply_color_location, 1, 0, 0);
    glUniform1f(shader->overlay_color_ratio_location, 0);

    const LWFVBO* fvbo = &pLwc->fvertex_buffer[lft];
    glBindBuffer(GL_ARRAY_BUFFER, fvbo->vertex_buffer);
    bind_all_fvertex_attrib(pLwc, lft); // TODO dummy value...
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
    
    mat4x4 proj_view;
    mat4x4_mul(proj_view, proj, view);
    const LWFANIM* fanim = &pLwc->fanim[lfat];
    int frame = (int)(frame_time * frames_per_sec);
    if (loop) {
        frame %= fanim->total_frame_count;
    } else {
        frame = LWMIN(frame, fanim->total_frame_count - 1);
    }
    mat4x4 identity;
    mat4x4_identity(identity);
    int vertex_first = 0;
    for (int i = 0; i < fvbo->total_cell_count; i++) {
        const LWFANIMKEY* anim_key = &fanim->anim_key[frame * fanim->total_cell_count + i];
        mat4x4 anim_rot;
        mat4x4 model;
        mat4x4_identity(model);
        mat4x4_from_quat(anim_rot, &anim_key->qw);
        mat4x4_mul(model, anim_rot, model);
        mat4x4 model_translate;
        mat4x4_translate(model_translate,
                         x + sx * anim_key->x,
                         y + sy * anim_key->y,
                         z + sz * anim_key->z);
        mat4x4_scale_aniso(model, model, sx, sy, sz);
        mat4x4_mul(model, model_translate, model);
        mat4x4_rotate_Z_2(model, model, z_rot_angle);
        mult_world_roll(model, puck_game->world_roll_axis, puck_game->world_roll_dir, puck_game->world_roll);
        mat4x4 proj_view_model;
        mat4x4_mul(proj_view_model, proj_view, model);
        glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
        glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model);
        glDrawArrays(GL_TRIANGLES, vertex_first, fvbo->vertex_count_per_cell[i]);
        vertex_first += fvbo->vertex_count_per_cell[i];
    }
    // draw all cells at once
    //glDrawArrays(GL_TRIANGLES, 0, fvbo->total_vertex_count);
    glUniform3f(shader->multiply_color_location, 1, 1, 1); // revert this
}
