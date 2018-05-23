#include "lwdirpad.h"
#include "lwgl.h"
#include "lwcontext.h"
#include "linmath.h"
#include "laidoff.h"
#include <string.h>
#include "lwatlasenum.h"

void render_dir_pad(const LWCONTEXT* pLwc, float x, float y) {
    int shader_index = LWST_DEFAULT;
    const int vbo_index = LVT_CENTER_CENTER_ANCHORED_SQUARE;

    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_rotate_X(model, model, 0);
    mat4x4_scale_aniso(model, model, 0.05f, 0.05f, 0.05f);
    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, 0);
    mat4x4_mul(model, model_translate, model);

    mat4x4 view_model;
    mat4x4 view; mat4x4_identity(view);
    mat4x4_mul(view_model, view, model);

    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, pLwc->proj, view_model);
    lazy_glUseProgram(pLwc, shader_index);
    lazy_glBindBuffer(pLwc, vbo_index);
    bind_all_vertex_attrib(pLwc, vbo_index);
    glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(pLwc->shader[shader_index].diffuse_location, 0); // 0 means GL_TEXTURE0
    glBindTexture(GL_TEXTURE_2D, pLwc->tex_programmed[LPT_DIR_PAD]);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[vbo_index].vertex_count);
}

void render_dir_pad_joystick_type(const LWCONTEXT* pLwc, float x, float y, LW_ATLAS_ENUM lae, float ui_alpha) {
    int shader_index = LWST_DEFAULT;
    const int vbo_index = LVT_CENTER_CENTER_ANCHORED_SQUARE;
    const float size = 0.25f;
    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_rotate_X(model, model, 0);
    mat4x4_scale_aniso(model, model, size, size, size);
    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, 0);
    mat4x4_mul(model, model_translate, model);

    mat4x4 view_model;
    mat4x4 view; mat4x4_identity(view);
    mat4x4_mul(view_model, view, model);

    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, pLwc->proj, view_model);

    const LWSHADER* shader = &pLwc->shader[shader_index];

    lazy_glUseProgram(pLwc, shader_index);
    glUniform2fv(shader->vuvoffset_location, 1, default_uv_offset);
    glUniform2fv(shader->vuvscale_location, 1, default_uv_scale);
    glUniform2fv(shader->vs9offset_location, 1, default_uv_offset);
    glUniform1f(shader->alpha_multiplier_location, ui_alpha);
    glUniform1i(shader->diffuse_location, 0); // 0 means GL_TEXTURE0
    glUniform3f(shader->overlay_color_location, 1, 1, 1);
    glUniform1f(shader->overlay_color_ratio_location, 0);

    lazy_glBindBuffer(pLwc, vbo_index);
    bind_all_vertex_attrib(pLwc, vbo_index);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(shader->diffuse_location, 0); // 0 means GL_TEXTURE0
    lazy_tex_atlas_glBindTexture(pLwc, lae);
    set_tex_filter(GL_LINEAR, GL_LINEAR);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[vbo_index].vertex_count);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

void render_dir_pad_joystick_area(const LWCONTEXT* pLwc, float x, float y, float ui_alpha) {
    render_dir_pad_joystick_type(pLwc, x, y, LAE_JOYSTICKAREA, ui_alpha);
}

void render_dir_pad_joystick(const LWCONTEXT* pLwc, float x, float y, float ui_alpha) {
    render_dir_pad_joystick_type(pLwc, x, y, LAE_JOYSTICK, ui_alpha);
}


void render_dir_pad_with_start(const LWCONTEXT* pLwc, const LWDIRPAD* dir_pad) {
    // Current touch position
    render_dir_pad(pLwc, dir_pad->x, dir_pad->y);
    // Touch origin position
    if (dir_pad->dragging) {
        render_dir_pad(pLwc, dir_pad->start_x, dir_pad->start_y);
    }
}

void render_dir_pad_with_start_joystick(const LWCONTEXT* pLwc, const LWDIRPAD* dir_pad, float ui_alpha) {
    if (dir_pad->dragging) {
        render_dir_pad_joystick_area(pLwc, dir_pad->start_x, dir_pad->start_y, ui_alpha);
    } else {
        render_dir_pad_joystick_area(pLwc, dir_pad->origin_x, dir_pad->origin_y, ui_alpha);
    }
    // Current touch position
    render_dir_pad_joystick(pLwc, dir_pad->x, dir_pad->y, ui_alpha);
}

float get_dir_pad_size_radius() {
    return 0.5f;
}

void get_right_dir_pad_original_center(const float aspect_ratio, float *x, float *y) {
    const float sr = get_dir_pad_size_radius();
    if (aspect_ratio > 1) {
        *x = 1 * aspect_ratio - sr;
        *y = -1 + sr;
    } else {
        *x = 1 - sr;
        *y = -1 / aspect_ratio + sr;
    }
}

void get_left_dir_pad_original_center(const float aspect_ratio, float* x, float* y) {
    const float sr = get_dir_pad_size_radius();
    if (aspect_ratio > 1) {
        *x = -1 * aspect_ratio + sr;
        *y = -1 + sr;
    } else {
        *x = -1 + sr;
        *y = -1 / aspect_ratio + sr;
    }
}

int lw_get_normalized_dir_pad_input(const LWCONTEXT* pLwc, const LWDIRPAD* dir_pad, float *dx, float *dy, float *dlen) {
    if (!dir_pad->dragging) {
        return 0;
    }

    *dx = dir_pad->x - dir_pad->start_x;
    *dy = dir_pad->y - dir_pad->start_y;

    *dlen = sqrtf(*dx * *dx + *dy * *dy);

    if (*dlen < LWEPSILON) {
        *dlen = 0;
        *dx = 0;
        *dy = 0;
    } else {
        *dx /= *dlen;
        *dy /= *dlen;
    }

    return 1;
}

void reset_dir_pad_position(LWDIRPAD* dir_pad) {
    dir_pad->x = dir_pad->origin_x;
    dir_pad->y = dir_pad->origin_y;
}

int dir_pad_press(LWDIRPAD* dir_pad, float x, float y, int pointer_id,
                  float dir_pad_center_x, float dir_pad_center_y, float sr) {
    if (fabs(dir_pad_center_x - x) < sr && fabs(dir_pad_center_y - y) < sr
        && !dir_pad->dragging) {
        dir_pad->start_x = x;
        dir_pad->start_y = y;
        dir_pad->touch_began_x = x;
        dir_pad->touch_began_y = y;
        dir_pad->x = x;
        dir_pad->y = y;
        dir_pad->dragging = 1;
        dir_pad->pointer_id = pointer_id;
        return 0;
    }
    return -1;
}

void dir_pad_move(LWDIRPAD* dir_pad, float x, float y, int pointer_id,
                  float dir_pad_center_x, float dir_pad_center_y, float sr) {
    if (dir_pad->dragging && dir_pad->pointer_id == pointer_id) {
        if (x < dir_pad_center_x - sr) {
            x = dir_pad_center_x - sr;
        }

        if (x > dir_pad_center_x + sr) {
            x = dir_pad_center_x + sr;
        }

        if (y < dir_pad_center_y - sr) {
            y = dir_pad_center_y - sr;
        }

        if (y > dir_pad_center_y + sr) {
            y = dir_pad_center_y + sr;
        }

        dir_pad->x = x;
        dir_pad->y = y;
    }
}

int dir_pad_release(LWDIRPAD* dir_pad, int pointer_id) {
    int have_dragged = 0;
    if (dir_pad->pointer_id == pointer_id) {
        reset_dir_pad_position(dir_pad);
        have_dragged = dir_pad->dragging;
        dir_pad->dragging = 0;
    }
    return have_dragged;
}

void dir_pad_follow_start_position(LWDIRPAD* dir_pad) {
    if (dir_pad->dragging) {
        const float dx = dir_pad->x - dir_pad->start_x;
        const float dy = dir_pad->y - dir_pad->start_y;
        const float cur_dist = sqrtf(dx*dx + dy*dy);

        if (cur_dist > dir_pad->max_follow_distance) {
            float new_start_x = dir_pad->x + (-dx) / cur_dist * dir_pad->max_follow_distance;
            float new_start_y = dir_pad->y + (-dy) / cur_dist * dir_pad->max_follow_distance;

            const float dx2 = new_start_x - dir_pad->touch_began_x;
            const float dy2 = new_start_y - dir_pad->touch_began_y;
            const float org_dist = sqrtf(dx2*dx2 + dy2*dy2);
            const float max_org_dist = dir_pad->max_began_distance;
            if (org_dist < max_org_dist) {
                dir_pad->start_x = new_start_x;
                dir_pad->start_y = new_start_y;
            } else {
                dir_pad->start_x = dir_pad->touch_began_x + dx2 / org_dist * max_org_dist;
                dir_pad->start_y = dir_pad->touch_began_y + dy2 / org_dist * max_org_dist;
            }
        }
    }
}

void dir_pad_init(LWDIRPAD* dir_pad,
                  float origin_x,
                  float origin_y,
                  float max_follow_distance,
                  float max_began_distance) {
    memset(dir_pad, 0, sizeof(LWDIRPAD));
    dir_pad->x = origin_x;
    dir_pad->y = origin_y;
    dir_pad->start_x = origin_x;
    dir_pad->start_y = origin_y;
    dir_pad->touch_began_x = origin_x;
    dir_pad->touch_began_y = origin_y;
    dir_pad->origin_x = origin_x;
    dir_pad->origin_y = origin_y;
    dir_pad->max_follow_distance = max_follow_distance;
    dir_pad->max_began_distance = max_began_distance;
}
