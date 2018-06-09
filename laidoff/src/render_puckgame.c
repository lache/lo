#include "lwcontext.h"
#include "render_puckgame.h"
#include "render_solid.h"
#include "laidoff.h"
#include "lwlog.h"
#include "puckgame.h"
#include "render_field.h"
#include "lwtextblock.h"
#include "render_text_block.h"
#include "lwudp.h"
#include "lwdamagetext.h"
#include "lwdirpad.h"
#include "lwmath.h"
#include "puckgameupdate.h"
#include "lwtcp.h"
#include "lwfvbo.h"
#include <assert.h>
#include "lwtimepoint.h"
#include "render_leaderboard.h"
#include "script.h"

typedef struct _LWSPHERERENDERUNIFORM {
    float sphere_col_ratio[3];
    float sphere_pos[3][3];
    float sphere_col[3][3];
    float sphere_speed[3];
    float sphere_move_rad[3];
    float reflect_size[3];
    float arrowRotMat2[2][2];
    float arrow_center[2];
    float arrow_scale;
} LWSPHERERENDERUNIFORM;

typedef struct _LWTOWERRENDERDATA {
    LW_VBO_TYPE lvt;
    float r;
    float g;
    float b;
} LWTOWERRENDERDATA;

void mult_world_roll(float aspect_ratio, mat4x4 model, int axis, int dir, float angle) {
    // world roll
    mat4x4 world_roll_rot;
    mat4x4_identity(world_roll_rot);
    if (aspect_ratio > 1) {
        // roll left to right
        if (axis == 0) {
            mat4x4_rotate_X(world_roll_rot, world_roll_rot, dir ? -angle : angle);
        } else if (axis == 1) {
            mat4x4_rotate_Y(world_roll_rot, world_roll_rot, dir ? -angle : angle);
        } else {
            mat4x4_rotate_Z(world_roll_rot, world_roll_rot, dir ? -angle : angle);
        }
    } else {
        // roll down to up
        if (axis == 0) {
            mat4x4_rotate_Y(world_roll_rot, world_roll_rot, dir ? angle : -angle);
        } else if (axis == 1) {
            mat4x4_rotate_X(world_roll_rot, world_roll_rot, dir ? angle : -angle);
        } else {
            mat4x4_rotate_Z(world_roll_rot, world_roll_rot, dir ? angle : -angle);
        }
    }
    mat4x4_mul(model, world_roll_rot, model);
}

static void render_tower_normal_2(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, const LWPUCKGAME* puck_game, const float* pos, const LWPUCKGAMETOWER* tower, int remote) {
    int shader_index = LWST_DEFAULT;
    const LWSHADER* shader = &pLwc->shader[shader_index];
    lazy_glUseProgram(pLwc, shader_index);
    glUniform2fv(shader->vuvoffset_location, 1, default_uv_offset);
    glUniform2fv(shader->vuvscale_location, 1, default_uv_scale);
    glUniform2fv(shader->vs9offset_location, 1, default_uv_offset);
    glUniform1f(shader->alpha_multiplier_location, 1.0f);
    glUniform1i(shader->diffuse_location, 0); // 0 means GL_TEXTURE0
    glUniform1i(shader->alpha_only_location, 1); // 1 means GL_TEXTURE1
    glUniform3f(shader->overlay_color_location, 1, 1, 1);

    mat4x4 rot;
    mat4x4_identity(rot);
    float sx = puck_game->tower_radius / puck_game->tower_mesh_radius;
    float sy = sx;
    float sz = sx;

    glUniform1f(shader->overlay_color_ratio_location, 0);

    LWTOWERRENDERDATA tower_render_data[] = {
        { LVT_TOWER_BASE_2, 1.0f, 1.0f, 1.0f },
    };
    for (int i = 0; i < ARRAY_SIZE(tower_render_data); i++) {
        float x = pos[0];
        float y = pos[1];
        float z = pos[2];
        // Positional offset by shake
        if (tower->shake_remain_time > 0) {
            const float ratio = tower->shake_remain_time / pLwc->puck_game->tower_shake_time;
            const float shake_magnitude = 0.03f;
            x += ratio * (2 * rand() / (float)RAND_MAX - 1.0f) * shake_magnitude * pLwc->viewport_rt_x;
            y += ratio * (2 * rand() / (float)RAND_MAX - 1.0f) * shake_magnitude * pLwc->viewport_rt_y;
            float ratio4 = ratio * ratio;
            ratio4 *= ratio4;
            glUniform1f(shader->overlay_color_ratio_location, ratio4);
        } else {
            glUniform1f(shader->overlay_color_ratio_location, 0);
        }

        mat4x4 model;
        mat4x4_identity(model);
        mat4x4_mul(model, model, rot);
        mat4x4_scale_aniso(model, model, sx, sy, sz);
        //mat4x4_scale_aniso(model, model, 5, 5, 5);

        mat4x4 model_translate;
        mat4x4_translate(model_translate, x, y, z);

        mat4x4_mul(model, model_translate, model);

        mult_world_roll(pLwc->viewport_aspect_ratio, model, puck_game->world_roll_axis, puck_game->world_roll_dir, puck_game->world_roll);

        mat4x4 view_model;
        mat4x4_mul(view_model, view, model);

        mat4x4 proj_view_model;
        mat4x4_identity(proj_view_model);
        mat4x4_mul(proj_view_model, proj, view_model);

        const LWTOWERRENDERDATA* d = tower_render_data + i;
        float r = d->r;
        float g = d->g;
        float b = d->b;
        glUniform3f(shader->overlay_color_location, r, g, b);
        const LW_VBO_TYPE lvt = d->lvt;

        lazy_glBindBuffer(pLwc, lvt);
        bind_all_vertex_attrib(pLwc, lvt);
        glActiveTexture(GL_TEXTURE0);

        int tower_lae = 0;
        if (puck_game->player_no == 2) {
            tower_lae = tower->owner_player_no == 2 ? LAE_TOWER_BASE_2_PLAYER : LAE_TOWER_BASE_2_TARGET;
        } else {
            tower_lae = tower->owner_player_no == 2 ? LAE_TOWER_BASE_2_TARGET : LAE_TOWER_BASE_2_PLAYER;
        }
        lazy_tex_atlas_glBindTexture(pLwc, tower_lae);
        set_tex_filter(GL_LINEAR, GL_LINEAR);
        glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
        glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model);
        glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
    }
}

static void render_tower_collapsing(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, const LWPUCKGAME* puck_game, const float* pos, const LWPUCKGAMETOWER* tower, int remote, float tower_collapsing_z_rot_angle) {
    // collapsing tower model scale matches current world
    const float tower_scale = 1.0f; // puck_game->tower_radius / puck_game->tower_mesh_radius;
    LW_F_ANIM_TYPE anim_type = LFAT_TOWER_COLLAPSE;
    if (puck_game->game_map == LPGM_OCTAGON) {
        anim_type = LFAT_TOWER_COLLAPSE_OCTAGON;
    }
    render_fvbo(pLwc,
                puck_game,
                view,
                proj,
                LFT_TOWER,
                anim_type,
                fabsf(pos[0]),
                fabsf(pos[1]),
                pos[2],
                tower_scale,
                tower_collapsing_z_rot_angle,
                tower->collapsing_time,
                0, // no loop
                50.0f); // frames per sec (animatino speed)
}

static void render_tower(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, const LWPUCKGAME* puck_game, const float* pos, const LWPUCKGAMETOWER* tower, int remote, float tower_collapsing_z_rot_angle) {
    if (tower->collapsing == 0) {
        render_tower_normal_2(pLwc, view, proj, puck_game, pos, tower, remote);
    } else {
        render_tower_collapsing(pLwc, view, proj, puck_game, pos, tower, remote, tower_collapsing_z_rot_angle);
    }
}

static void render_go(const LWCONTEXT* pLwc,
                      const mat4x4 view,
                      const mat4x4 proj,
                      const LWPUCKGAME* puck_game,
                      const LWPUCKGAMEOBJECT* go,
                      int tex_index,
                      float render_scale,
                      const float* remote_pos,
                      const mat4x4 remote_rot,
                      int remote,
                      float speed,
                      int shader_index) {
    if (go->body == 0) {
        return;
    }
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


    float sx = render_scale * go->radius, sy = render_scale * go->radius, sz = render_scale * go->radius;
    if (go->capsule) {
        sx = sy = sz = 1;
    }
    float x = render_scale * (remote_pos ? remote_pos[0] : go->pos[0]);
    float y = render_scale * (remote_pos ? remote_pos[1] : go->pos[1]);
    float z = render_scale * (remote_pos ? remote_pos[2] : go->pos[2]);

    int puck_owner_player_no = remote ? pLwc->puck_game_state.bf.puck_owner_player_no : pLwc->puck_game->puck_owner_player_no;

    const float e = 2.718f;
    const float red_overlay_ratio = go->red_overlay ? LWMIN(1.0f, speed / go->puck_game->puck_damage_contact_speed_threshold) : 0;
    const float red_overlay_logistic_ratio = LWMIN(0.3f, 1 / (1 + powf(e, -(20.0f * (red_overlay_ratio - 0.8f)))));
    static const float red_overlay[] = { 1.0f, 1.0f, 1.0f };
    static const float red_multiply[] = { 1.0f, 0.3f, 0.3f };
    static const float blue_overlay[] = { 1.0f, 1.0f, 1.0f };
    static const float blue_multiply[] = { 0.3f, 0.5f, 1.0f };
    static const float white_overlay[] = { 1.0f, 1.0f, 1.0f };
    static const float white_multiply[] = { 1.0f, 1.0f, 1.0f };
    if (go->red_overlay) {
        if (puck_owner_player_no == 1) {
            if (pLwc->puck_game->player_no == 2) {
                glUniform3fv(shader->overlay_color_location, 1, red_overlay);
                glUniform3fv(shader->multiply_color_location, 1, red_multiply);
            } else {
                glUniform3fv(shader->overlay_color_location, 1, blue_overlay);
                glUniform3fv(shader->multiply_color_location, 1, blue_multiply);
            }
        } else if (puck_owner_player_no == 2) {
            if (pLwc->puck_game->player_no == 2) {
                glUniform3fv(shader->overlay_color_location, 1, blue_overlay);
                glUniform3fv(shader->multiply_color_location, 1, blue_multiply);
            } else {
                glUniform3fv(shader->overlay_color_location, 1, red_overlay);
                glUniform3fv(shader->multiply_color_location, 1, red_multiply);
            }
        } else {
            glUniform3fv(shader->overlay_color_location, 1, white_overlay);
            glUniform3fv(shader->multiply_color_location, 1, white_multiply);
        }
    } else {
        glUniform3fv(shader->overlay_color_location, 1, white_overlay);
        glUniform3fv(shader->multiply_color_location, 1, white_multiply);
    }
    glUniform1f(shader->overlay_color_ratio_location, red_overlay_logistic_ratio);

    mat4x4 rot;
    mat4x4_identity(rot);

    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_mul(model, model, rot);
    mat4x4_scale_aniso(model, model, sx, sy, sz);
    //mat4x4_scale_aniso(model, model, 5, 5, 5);

    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, z);

    mat4x4_mul(model, remote ? remote_rot : go->rot, model);
    mat4x4_mul(model, model_translate, model);

    mult_world_roll(pLwc->viewport_aspect_ratio, model, puck_game->world_roll_axis, puck_game->world_roll_dir, puck_game->world_roll);

    mat4x4 view_model;
    mat4x4_mul(view_model, view, model);

    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);

    const LW_VBO_TYPE lvt = go->red_overlay ? puck_game->puck_lvt : puck_game->player_lvt;
    lazy_glBindBuffer(pLwc, lvt);
    bind_all_vertex_attrib(pLwc, lvt);
    glActiveTexture(GL_TEXTURE0);
    assert(tex_index);
    glBindTexture(GL_TEXTURE_2D, tex_index);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
    //set_tex_filter(GL_NEAREST, GL_NEAREST);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
    glUniform3f(shader->multiply_color_location, 1, 1, 1);
}

static void render_radial_wave(const LWCONTEXT* pLwc,
                               const mat4x4 view,
                               const mat4x4 proj,
                               const LWPUCKGAME* puck_game,
                               const LWPUCKGAMEOBJECT* go,
                               int tex_index,
                               float render_scale,
                               const float* remote_pos,
                               const mat4x4 remote_rot,
                               int remote,
                               float speed) {
    if (go->geom == 0) {
        return;
    }
    const float uv_offset[2] = { -(float)puck_game->time / 2, 0.0f };
    const float uv_scale[2] = { 0.75f, 1.0f };
    const float scale = 0.75f;
    const float alpha = 0.20f;

    int shader_index = LWST_RADIALWAVE;
    const LWSHADER* shader = &pLwc->shader[shader_index];
    lazy_glUseProgram(pLwc, shader_index);
    glUniform2fv(shader->vuvoffset_location, 1, uv_offset);
    glUniform2fv(shader->vuvscale_location, 1, uv_scale);
    glUniform2fv(shader->vs9offset_location, 1, default_uv_offset);
    glUniform1f(shader->alpha_multiplier_location, alpha);
    glUniform1i(shader->diffuse_location, 0); // 0 means GL_TEXTURE0
    glUniform1i(shader->alpha_only_location, 1); // 1 means GL_TEXTURE1
    glUniform3f(shader->overlay_color_location, 1, 1, 1);
    glUniform1f(shader->overlay_color_ratio_location, 0);
    glUniform1f(shader->wrap_offset, 0);

    float x = render_scale * (remote_pos ? remote_pos[0] : go->pos[0]);
    float y = render_scale * (remote_pos ? remote_pos[1] : go->pos[1]);
    float z = render_scale * (remote_pos ? remote_pos[2] : go->pos[2]);

    mat4x4 rot, model, model_translate, view_model, proj_view_model;

    mat4x4_identity(rot);
    mat4x4_identity(model);
    mat4x4_mul(model, model, rot);
    mat4x4_scale_aniso(model, model, scale, scale, scale);
    mat4x4_translate(model_translate, x, y, z);
    mat4x4_mul(model, model_translate, model);
    mult_world_roll(pLwc->viewport_aspect_ratio, model, puck_game->world_roll_axis, puck_game->world_roll_dir, puck_game->world_roll);
    mat4x4_mul(view_model, view, model);
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    const LW_VBO_TYPE lvt = LVT_RADIALWAVE;
    lazy_glBindBuffer(pLwc, lvt);
    bind_all_vertex_attrib(pLwc, lvt);
    glActiveTexture(GL_TEXTURE0);
    lazy_tex_atlas_glBindTexture(pLwc, LAE_LINEARWAVE);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
    //set_tex_filter(GL_NEAREST, GL_NEAREST);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
    glUniform3f(shader->multiply_color_location, 1, 1, 1);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

void calculate_world_right_top_end_ui_point(const LWCONTEXT* pLwc, const LWPUCKGAME* puck_game, vec2 world_right_top_end_ui_point) {
    // calculate world right top point in ui coordinate
    vec4 world_right_top_end_vec4 = {
        2.11f, // half world model dimension including bezel hard-coded
        2.11f, // half world model dimension including bezel hard-coded
        0.76f, // half world model dimension including bezel hard-coded
        1.00f,
    };
    mat4x4 proj_view;
    mat4x4_identity(proj_view);
    mat4x4_mul(proj_view, pLwc->puck_game_proj, pLwc->puck_game_view);
    calculate_ui_point_from_world_point(pLwc->viewport_aspect_ratio, proj_view, world_right_top_end_vec4, world_right_top_end_ui_point);
    // ignore world rotation for player 2
    world_right_top_end_ui_point[0] = fabsf(world_right_top_end_ui_point[0]);
    world_right_top_end_ui_point[1] = fabsf(world_right_top_end_ui_point[1]);
}

static float hp_star_nickname_score_x(const LWCONTEXT* pLwc, int left, const vec2 world_right_top_end_ui_point) {
    if (pLwc->viewport_aspect_ratio > 1) {
        return (world_right_top_end_ui_point[0] + pLwc->viewport_rt_x) / 2 * (left ? -1 : +1);
    } else {
        return pLwc->viewport_rt_x / 2 * (left ? -1 : +1);
    }
}

static void render_hp_star(const LWCONTEXT* pLwc,
                           const LWPUCKGAME* puck_game,
                           float ui_alpha,
                           int hp,
                           int left,
                           float hp_shake_remain_time,
                           const vec2 world_right_top_end_ui_point) {
    // render at the center of margins
    float x = hp_star_nickname_score_x(pLwc, left, world_right_top_end_ui_point);
    float y = pLwc->viewport_rt_y - 0.425f;
    float size = 0.5f;
    if (hp_shake_remain_time > 0) {
        const float ratio = hp_shake_remain_time / pLwc->puck_game->hp_shake_time;
        const float shake_magnitude = 0.02f;
        x += ratio * (2 * rand() / (float)RAND_MAX - 1.0f) * shake_magnitude * pLwc->viewport_rt_x;
        y += ratio * (2 * rand() / (float)RAND_MAX - 1.0f) * shake_magnitude * pLwc->viewport_rt_y;
    }
    lw_load_tex(pLwc, LAE_HP_STAR_0 + 2 * hp);
    lw_load_tex(pLwc, LAE_HP_STAR_0 + 2 * hp + 1);
    render_solid_vb_ui_alpha_uv(pLwc,
                                x,
                                y,
                                size,
                                size,
                                pLwc->tex_atlas[LAE_HP_STAR_0 + 2 * hp],
                                pLwc->tex_atlas[LAE_HP_STAR_0 + 2 * hp + 1],
                                LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                ui_alpha,
                                0.0f,
                                0.0f,
                                0.0f,
                                0.0f,
                                default_uv_offset,
                                default_uv_scale);
}

static void render_timer(const LWCONTEXT* pLwc, float remain_sec, float total_sec, float ui_alpha) {
    // Render text
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_CENTER_CENTER;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_D;
    text_block.multiline = 1;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, ui_alpha);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, ui_alpha);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, ui_alpha);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, ui_alpha);
    char str[32];
    if (remain_sec < 0) {
        sprintf(str, u8"--");
    } else {
        sprintf(str, u8"%.0f", remain_sec);
    }
    float remain_ratio = 1.0f;
    if (total_sec != 0) {
        remain_ratio = remain_sec / total_sec;
    }
    float x = 0.0f;
    float y = pLwc->viewport_rt_y - 0.075f;
    text_block.text = str;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = x;
    text_block.text_block_y = y;
    render_text_block(pLwc, &text_block);

    const LWSHADER* shader = &pLwc->shader[LWST_RINGGAUGE];

    lazy_glUseProgram(pLwc, LWST_RINGGAUGE);
    glUniform3f(shader->full_color, 0, 1, 0);
    glUniform3f(shader->empty_color, 1, 0, 0);
    glUniform1f(shader->gauge_ratio, remain_ratio);

    // text origin point debug indicator
    render_solid_vb_ui_flip_y_uv_shader_rot(
        pLwc,
        x,
        y,
        0.0325f,
        0.0325f,
        0,//pLwc->tex_atlas[LVT_RINGGAUGE],
        LVT_RINGGAUGETHICK,
        ui_alpha,
        0,
        1,
        0,
        1,
        0,
        LWST_RINGGAUGE,
        (float)M_PI
    );
}

static void render_tutorial_guide(const LWCONTEXT* pLwc, const LWPUCKGAME* puck_game, float ui_alpha) {
    if (puck_game->game_state == LPGS_TUTORIAL) {
        LWTEXTBLOCK text_block;
        text_block.align = LTBA_CENTER_TOP;
        text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
        text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
        text_block.size = pLwc->viewport_aspect_ratio > 1 ? DEFAULT_TEXT_BLOCK_SIZE_B : DEFAULT_TEXT_BLOCK_SIZE_D;
        text_block.multiline = 1;
        SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, ui_alpha);
        SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, ui_alpha);
        SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, ui_alpha);
        SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, ui_alpha);
        text_block.text = puck_game->tutorial_guide_str;
        text_block.text_bytelen = (int)strlen(text_block.text);
        text_block.begin_index = 0;
        text_block.end_index = text_block.text_bytelen;
        text_block.text_block_x = 0;
        text_block.text_block_y = pLwc->viewport_aspect_ratio > 1 ? 0.95f : 0.9f;
        render_text_block(pLwc, &text_block);
    }
}

static void render_dash_ring_gauge(const LWCONTEXT* pLwc, vec4 player_pos, float ui_alpha) {
    const LWPUCKGAMEDASH* dash = puck_game_single_play_dash_object(pLwc->puck_game);
    float gauge_ratio = puck_game_dash_gauge_ratio(pLwc->puck_game, dash);
    if (gauge_ratio >= 1.0f) {
        return;
    }
    mat4x4 proj_view;
    mat4x4_identity(proj_view);
    mat4x4_mul(proj_view, pLwc->puck_game_proj, pLwc->puck_game_view);
    vec2 ui_point;
    calculate_ui_point_from_world_point(pLwc->viewport_aspect_ratio, proj_view, player_pos, ui_point);

    lazy_glUseProgram(pLwc, LWST_RINGGAUGE);
    glUniform3f(pLwc->shader[LWST_RINGGAUGE].full_color, 0, 1, 0);
    glUniform3f(pLwc->shader[LWST_RINGGAUGE].empty_color, 1, 0, 0);
    glUniform1f(pLwc->shader[LWST_RINGGAUGE].gauge_ratio, gauge_ratio);
    // text origin point debug indicator
    render_solid_vb_ui_flip_y_uv_shader_rot(
        pLwc,
        ui_point[0],
        ui_point[1],
        0.03f,
        0.03f,
        0,//pLwc->tex_atlas[LVT_RINGGAUGE],
        LVT_RINGGAUGE,
        0.75f * ui_alpha,
        0,
        1,
        0,
        1,
        0,
        LWST_RINGGAUGE,
        (float)M_PI
    );
}

static void render_searching_state(const LWCONTEXT* pLwc, const LWPUCKGAME* puck_game) {
    if (puck_game->searching_str[0] == 0) {
        return;
    }
    // Render text
    float ui_alpha = 0.3f + 0.7f * fabsf(sinf((float)(pLwc->app_time * 3.0f)));
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_CENTER_CENTER;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_C;
    text_block.multiline = 1;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, ui_alpha);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, ui_alpha);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, ui_alpha);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, ui_alpha);
    text_block.text = puck_game->searching_str;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = 0;
    text_block.text_block_y = 0;
    render_text_block(pLwc, &text_block);
}

static void render_nickname_score(const LWCONTEXT* pLwc,
                                  int left,
                                  float ui_alpha,
                                  const char* nickname,
                                  const char* score,
                                  const vec2 world_right_top_end_ui_point) {
    // render at the center of margins
    float x = hp_star_nickname_score_x(pLwc, left, world_right_top_end_ui_point);
    float y = pLwc->viewport_rt_y - 0.675f;
    // Render text
    LWTEXTBLOCK text_block;
    //text_block.align = left ? LTBA_LEFT_TOP : LTBA_RIGHT_TOP;
    text_block.align = left ? LTBA_CENTER_TOP : LTBA_CENTER_TOP;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_E;
    text_block.multiline = 1;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, ui_alpha);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, ui_alpha);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, ui_alpha);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, ui_alpha);
    // nickname
    text_block.text = nickname;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = x;
    text_block.text_block_y = y;
    render_text_block(pLwc, &text_block);
    // score
    text_block.text = score;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = x;
    text_block.text_block_y = y - 0.1f;
    render_text_block(pLwc, &text_block);
}

static void render_wall(const LWCONTEXT* pLwc,
                        const mat4x4 proj,
                        const LWPUCKGAME* puck_game,
                        int shader_index,
                        const mat4x4 view,
                        float x,
                        float y,
                        float z,
                        float x_rot,
                        float y_rot,
                        float z_rot,
                        LW_VBO_TYPE lvt,
                        float sx,
                        float sy,
                        float sz,
                        const LWSPHERERENDERUNIFORM* sphere_render_uniform,
                        LW_PUCK_GAME_BOUNDARY boundary) {
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
    glUniform1fv(shader->sphere_col_ratio, 3, sphere_render_uniform->sphere_col_ratio);
    glUniform3fv(shader->sphere_pos, 3, (const float*)sphere_render_uniform->sphere_pos);
    glUniform3fv(shader->sphere_col, 3, (const float*)sphere_render_uniform->sphere_col);
    glUniform1fv(shader->sphere_speed, 3, (const float*)sphere_render_uniform->sphere_speed);
    glUniform1fv(shader->sphere_move_rad, 3, (const float*)sphere_render_uniform->sphere_move_rad);
    glUniform3fv(shader->reflect_size, 1, (const float*)sphere_render_uniform->reflect_size);

    const int tex_index = 0;
    mat4x4 rot_x;
    mat4x4_identity(rot_x);
    mat4x4_rotate_X(rot_x, rot_x, x_rot);
    mat4x4 rot_y;
    mat4x4_identity(rot_y);
    mat4x4_rotate_Y(rot_y, rot_y, y_rot);
    mat4x4 rot_z;
    mat4x4_identity(rot_z);
    mat4x4_rotate_Z(rot_z, rot_z, z_rot);
    mat4x4 rot;
    mat4x4_mul(rot, rot_x, rot_y);
    mat4x4_mul(rot, rot_z, rot);

    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_mul(model, model, rot);
    mat4x4_scale_aniso(model, model, sx, sy, sz);
    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, z);
    mat4x4_mul(model, model_translate, model);

    mult_world_roll(pLwc->viewport_aspect_ratio, model, puck_game->world_roll_axis, puck_game->world_roll_dir, puck_game->world_roll);

    mat4x4 view_model;
    mat4x4_mul(view_model, view, model);

    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);

    int boundary_player_no = puck_game->boundary_impact_player_no[boundary];
    float boundary_impact = puck_game->boundary_impact[boundary];
    float boundary_impact_ratio = boundary_impact / puck_game->boundary_impact_start;
    if (puck_game->player_no != 2 ? boundary_player_no == 1 : boundary_player_no == 2) {
        glUniform3f(shader->overlay_color_location,
                    0.1f + 0.0f * boundary_impact_ratio,
                    0.1f + 0.0f * boundary_impact_ratio,
                    0.1f + 0.9f * boundary_impact_ratio);
    } else if (puck_game->player_no != 2 ? boundary_player_no == 2 : boundary_player_no == 1) {
        glUniform3f(shader->overlay_color_location,
                    0.1f + 0.9f * boundary_impact_ratio,
                    0.1f + 0.0f * boundary_impact_ratio,
                    0.1f + 0.0f * boundary_impact_ratio);
    } else {
        glUniform3f(shader->overlay_color_location,
                    0.1f,
                    0.1f,
                    0.1f);
    }
    glUniform1f(shader->overlay_color_ratio_location, 1.0f);

    //glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);

    lazy_glBindBuffer(pLwc, lvt);
    bind_all_vertex_attrib(pLwc, lvt);
    glActiveTexture(GL_TEXTURE0);
    //assert(tex_index); // tex_index is 0 for now
    glBindTexture(GL_TEXTURE_2D, tex_index);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
    //set_tex_filter(GL_NEAREST, GL_NEAREST);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

static void render_puck_game_menu(const LWCONTEXT *pLwc, const mat4x4 proj, const mat4x4 view, const LWPUCKGAME *puck_game) {
    int shader_index = LWST_DEFAULT;
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

    const int tex_index = pLwc->tex_atlas[LAE_PHYSICS_MENU];
    lw_load_tex(pLwc, LAE_PHYSICS_MENU);
    mat4x4 rot;
    mat4x4_identity(rot);
    mat4x4_rotate_Y(rot, rot, (float)LWDEG2RAD(180));

    float sx = 1.0f;
    float sy = 1.0f;
    float sz = 1.0f;
    float x = 0.0f, y = 0.0f, z = 0.0f;

    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_mul(model, model, rot);
    mat4x4_scale_aniso(model, model, sx, sy, sz);
    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, z);
    mat4x4_mul(model, model_translate, model);

    mult_world_roll(pLwc->viewport_aspect_ratio, model, puck_game->world_roll_axis, puck_game->world_roll_dir, puck_game->world_roll);

    mat4x4 view_model;
    mat4x4_mul(view_model, view, model);

    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);

    glUniform3f(shader->overlay_color_location, 0.5f, 0.5f, 0.5f);
    glUniform1f(shader->overlay_color_ratio_location, 0.0f);

    //glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);

    const LW_VBO_TYPE lvt = LVT_PHYSICS_MENU;

    lazy_glBindBuffer(pLwc, lvt);
    bind_all_vertex_attrib(pLwc, lvt);

    glActiveTexture(GL_TEXTURE0);
    assert(tex_index);
    glBindTexture(GL_TEXTURE_2D, tex_index);
    glUniform1i(pLwc->shader[shader_index].diffuse_location, 0);
    set_tex_filter(GL_LINEAR, GL_LINEAR);

    //set_tex_filter(GL_NEAREST, GL_NEAREST);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

static void render_custom_stage(const LWCONTEXT* pLwc,
                                const mat4x4 proj,
                                const mat4x4 view,
                                const LWPUCKGAME* puck_game,
                                LW_VBO_TYPE lvt,
                                LW_ATLAS_ENUM lae) {
    int shader_index = LWST_DEFAULT;
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
    mat4x4 rot;
    mat4x4_identity(rot);
    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_mul(model, model, rot);
    mat4x4_scale_aniso(model, model, 1, 1, 1);
    mat4x4 model_translate;
    mat4x4_translate(model_translate, 0, 0, 0);
    mat4x4_mul(model, model_translate, model);
    mult_world_roll(pLwc->viewport_aspect_ratio, model, puck_game->world_roll_axis, puck_game->world_roll_dir, puck_game->world_roll);
    mat4x4 view_model;
    mat4x4_mul(view_model, view, model);
    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);
    lazy_glBindBuffer(pLwc, lvt);
    bind_all_vertex_attrib(pLwc, lvt);
    glActiveTexture(GL_TEXTURE0);
    lazy_tex_atlas_glBindTexture(pLwc, lae);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

static void render_floor_cover(const LWCONTEXT* pLwc,
                               const mat4x4 proj,
                               const mat4x4 view,
                               const LWPUCKGAME* puck_game) {
    if (puck_game->game_map == LPGM_SQUARE) {
        render_custom_stage(pLwc, proj, view, puck_game, LVT_PUCK_FLOOR_COVER, LAE_PUCK_FLOOR_COVER);
    } else {
        render_custom_stage(pLwc, proj, view, puck_game, LVT_PUCK_FLOOR_COVER_OCTAGON, LAE_PUCK_FLOOR_COVER_OCTAGON);
    }
}

static void render_floor(const LWCONTEXT *pLwc,
                         const mat4x4 proj,
                         const LWPUCKGAME *puck_game,
                         int shader_index,
                         const mat4x4 view,
                         const LWSPHERERENDERUNIFORM* sphere_render_uniform) {

    const LWSHADER* shader = &pLwc->shader[shader_index];
    lazy_glUseProgram(pLwc, shader_index);
    glUniform2fv(shader->vuvoffset_location, 1, default_uv_offset);
    const float uv_scale[2] = { 4, 4 };
    glUniform2fv(shader->vuvscale_location, 1, uv_scale);
    glUniform2fv(shader->vs9offset_location, 1, default_uv_offset);
    glUniform1f(shader->alpha_multiplier_location, 1.0f);
    glUniform1i(shader->diffuse_location, 0); // 0 means GL_TEXTURE0
    glUniform1i(shader->alpha_only_location, 1); // 1 means GL_TEXTURE1
    glUniform3f(shader->overlay_color_location, 1, 1, 1);
    glUniform1f(shader->overlay_color_ratio_location, 0);
    glUniform1fv(shader->sphere_col_ratio, 3, sphere_render_uniform->sphere_col_ratio);
    glUniform3fv(shader->sphere_pos, 3, (const float*)sphere_render_uniform->sphere_pos);
    glUniform3fv(shader->sphere_col, 3, (const float*)sphere_render_uniform->sphere_col);
    glUniform1fv(shader->sphere_speed, 3, (const float*)sphere_render_uniform->sphere_speed);
    glUniform1fv(shader->sphere_move_rad, 3, (const float*)sphere_render_uniform->sphere_move_rad);
    glUniformMatrix2fv(shader->arrowRotMat2, 1, 0, (const float*)sphere_render_uniform->arrowRotMat2);
    glUniform2fv(shader->arrow_center, 1, sphere_render_uniform->arrow_center);
    glUniform1f(shader->arrow_scale, sphere_render_uniform->arrow_scale);
    glUniform3fv(shader->reflect_size, 1, (const float*)sphere_render_uniform->reflect_size);

    const int tex_index = pLwc->tex_atlas[LAE_PUCK_FLOOR_KTX];
    const int arrow_tex_index = pLwc->tex_atlas[LAE_ARROW];
    lw_load_tex(pLwc, LAE_PUCK_FLOOR_KTX);
    lw_load_tex(pLwc, LAE_ARROW);
    mat4x4 rot;
    mat4x4_identity(rot);

    float sx = puck_game->world_width_half;
    float sy = puck_game->world_height_half;
    float sz = 1.0f;
    float x = 0.0f, y = 0.0f, z = 0.0f;

    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_mul(model, model, rot);
    mat4x4_scale_aniso(model, model, sx, sy, sz);
    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, z);
    mat4x4_mul(model, model_translate, model);

    mult_world_roll(pLwc->viewport_aspect_ratio, model, puck_game->world_roll_axis, puck_game->world_roll_dir, puck_game->world_roll);

    mat4x4 view_model;
    mat4x4_mul(view_model, view, model);

    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);

    glUniform3f(shader->overlay_color_location, 0.2f, 0.2f, 0.2f);
    glUniform1f(shader->overlay_color_ratio_location, 0.7f);

    //glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);

    LW_VBO_TYPE lvt = LVT_CENTER_CENTER_ANCHORED_SQUARE;
    GLenum mode = GL_TRIANGLES;
    if (puck_game->game_map == LPGM_OCTAGON) {
        lvt = LVT_OCTAGON_PLANE;
        mode = GL_TRIANGLE_FAN;
    }
    lazy_glBindBuffer(pLwc, lvt);
    bind_all_vertex_attrib(pLwc, lvt);

    glActiveTexture(GL_TEXTURE0);
    assert(tex_index);
    glBindTexture(GL_TEXTURE_2D, tex_index);
    glUniform1i(pLwc->shader[shader_index].diffuse_location, 0);
    set_tex_filter(GL_LINEAR, GL_LINEAR);

    glActiveTexture(GL_TEXTURE1);
    assert(arrow_tex_index);
    glBindTexture(GL_TEXTURE_2D, arrow_tex_index);
    glUniform1i(pLwc->shader[shader_index].diffuse_arrow_location, 1);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //set_tex_filter(GL_NEAREST, GL_NEAREST);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model);
    glDrawArrays(mode, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

static void render_wall_square(const LWCONTEXT *pLwc,
                               const mat4x4 proj,
                               const LWPUCKGAME *puck_game,
                               int wall_shader_index,
                               const mat4x4 view,
                               const LWSPHERERENDERUNIFORM* sphere_render_uniform) {
    // North wall
    render_wall(pLwc,
                proj,
                puck_game,
                wall_shader_index,
                view,
                0,
                puck_game->world_height_half,
                0,
                (float)LWDEG2RAD(90),
                0,
                0,
                LVT_CENTER_BOTTOM_ANCHORED_SQUARE,
                puck_game->world_width_half,
                puck_game->wall_height / 2,
                puck_game->world_width_half,
                sphere_render_uniform,
                LPGB_N);
    // South wall
    render_wall(pLwc,
                proj,
                puck_game,
                wall_shader_index,
                view,
                0,
                -puck_game->world_height_half,
                0,
                (float)LWDEG2RAD(-90),
                0,
                0,
                LVT_CENTER_TOP_ANCHORED_SQUARE,
                puck_game->world_width_half,
                puck_game->wall_height / 2,
                puck_game->world_width_half,
                sphere_render_uniform,
                LPGB_S);
    // East wall
    render_wall(pLwc,
                proj,
                puck_game,
                wall_shader_index,
                view,
                puck_game->world_width_half,
                0,
                0,
                0,
                (float)LWDEG2RAD(90),
                0,
                LVT_LEFT_CENTER_ANCHORED_SQUARE,
                puck_game->wall_height / 2,
                puck_game->world_height_half,
                puck_game->world_height_half,
                sphere_render_uniform,
                LPGB_E);
    // West wall
    render_wall(pLwc,
                proj,
                puck_game,
                wall_shader_index,
                view,
                -puck_game->world_width_half,
                0,
                0,
                0,
                (float)LWDEG2RAD(-90),
                0,
                LVT_RIGHT_CENTER_ANCHORED_SQUARE,
                puck_game->wall_height / 2,
                puck_game->world_height_half,
                puck_game->world_height_half,
                sphere_render_uniform,
                LPGB_W);
}

static void render_wall_octagon(const LWCONTEXT *pLwc,
                                const mat4x4 proj,
                                const LWPUCKGAME *puck_game,
                                int wall_shader_index,
                                const mat4x4 view,
                                const LWSPHERERENDERUNIFORM* sphere_render_uniform) {
    assert(puck_game->world_width_half == puck_game->world_height_half);
    float h = puck_game->world_width_half;
    float wall_length = 2 * h * tanf((float)(M_PI / 8));
    // North wall
    render_wall(pLwc,
                proj,
                puck_game,
                wall_shader_index,
                view,
                0,
                h,
                0,
                (float)LWDEG2RAD(90),
                0,
                0,
                LVT_CENTER_BOTTOM_ANCHORED_SQUARE,
                wall_length / 2,
                puck_game->wall_height / 2,
                wall_length / 2,
                sphere_render_uniform,
                LPGB_NN);
    // South wall
    render_wall(pLwc,
                proj,
                puck_game,
                wall_shader_index,
                view,
                0,
                -h,
                0,
                (float)LWDEG2RAD(-90),
                0,
                0,
                LVT_CENTER_TOP_ANCHORED_SQUARE,
                wall_length / 2,
                puck_game->wall_height / 2,
                wall_length / 2,
                sphere_render_uniform,
                LPGB_SS);
    // East wall
    render_wall(pLwc,
                proj,
                puck_game,
                wall_shader_index,
                view,
                h,
                0,
                0,
                0,
                (float)LWDEG2RAD(90),
                0,
                LVT_LEFT_CENTER_ANCHORED_SQUARE,
                puck_game->wall_height / 2,
                wall_length / 2,
                wall_length / 2,
                sphere_render_uniform,
                LPGB_EE);
    // West wall
    render_wall(pLwc,
                proj,
                puck_game,
                wall_shader_index,
                view,
                -h,
                0,
                0,
                0,
                (float)LWDEG2RAD(-90),
                0,
                LVT_RIGHT_CENTER_ANCHORED_SQUARE,
                puck_game->wall_height / 2,
                wall_length / 2,
                wall_length / 2,
                sphere_render_uniform,
                LPGB_WW);

    float c45 = cosf((float)M_PI / 4);
    float s45 = sinf((float)M_PI / 4);

    // East-North wall
    render_wall(pLwc,
                proj,
                puck_game,
                wall_shader_index,
                view,
                c45 * h,
                s45 * h,
                0,
                0,
                (float)LWDEG2RAD(90),
                (float)LWDEG2RAD(45),
                LVT_LEFT_CENTER_ANCHORED_SQUARE,
                puck_game->wall_height / 2,
                wall_length / 2,
                wall_length / 2,
                sphere_render_uniform,
                LPGB_EN);

    // North-West wall
    render_wall(pLwc,
                proj,
                puck_game,
                wall_shader_index,
                view,
                -s45 * h,
                +c45 * h,
                0,
                (float)LWDEG2RAD(90),
                0,
                (float)LWDEG2RAD(45),
                LVT_CENTER_BOTTOM_ANCHORED_SQUARE,
                wall_length / 2,
                puck_game->wall_height / 2,
                wall_length / 2,
                sphere_render_uniform,
                LPGB_NW);

    // West-South wall
    render_wall(pLwc,
                proj,
                puck_game,
                wall_shader_index,
                view,
                -c45 * h,
                -s45 * h,
                0,
                0,
                (float)LWDEG2RAD(-90),
                (float)LWDEG2RAD(45),
                LVT_RIGHT_CENTER_ANCHORED_SQUARE,
                puck_game->wall_height / 2,
                wall_length / 2,
                wall_length / 2,
                sphere_render_uniform,
                LPGB_WS);
    // South-East wall
    render_wall(pLwc,
                proj,
                puck_game,
                wall_shader_index,
                view,
                +s45 * h,
                -c45 * h,
                0,
                (float)LWDEG2RAD(-90),
                0,
                (float)LWDEG2RAD(45),
                LVT_CENTER_TOP_ANCHORED_SQUARE,
                wall_length / 2,
                puck_game->wall_height / 2,
                wall_length / 2,
                sphere_render_uniform,
                LPGB_SE);
}

static void render_default_stage(const LWCONTEXT *pLwc,
                                 const mat4x4 proj,
                                 const mat4x4 view,
                                 const LWPUCKGAME *puck_game,
                                 int floor_shader_index,
                                 int wall_shader_index,
                                 const LWSPHERERENDERUNIFORM* sphere_render_uniform) {

    // Floor cover
    render_floor_cover(pLwc, proj, view, puck_game);
    // Floor
    render_floor(pLwc, proj, puck_game, floor_shader_index, view, sphere_render_uniform);
    // Walls
    if (puck_game->game_map == LPGM_SQUARE) {
        render_wall_square(pLwc,
                           proj,
                           puck_game,
                           wall_shader_index,
                           view,
                           sphere_render_uniform);
    } else {
        render_wall_octagon(pLwc,
                            proj,
                            puck_game,
                            wall_shader_index,
                            view,
                            sphere_render_uniform);
    }
}

static void render_battle_result_popup(const LWCONTEXT* pLwc,
                                       const LWPUCKGAME* puck_game,
                                       int player_no,
                                       int update_tick,
                                       LWP_STATE_PHASE battle_phase,
                                       float ui_alpha) {
    const char* sprite_name;
    LW_ATLAS_ENUM lae;
    LW_ATLAS_ENUM lae_alpha;
    LW_ATLAS_CONF lac;
    const float sprite_width = 1.5f;
    const float x = 0.0f;
    float y = 0.0f;
    switch (battle_phase) {
    case LSP_READY:
        sprite_name = "ready.png";
        lae = LAE_PREPARE_TITLE_ATLAS;
        lae_alpha = LAE_PREPARE_TITLE_ATLAS_ALPHA;
        lac = LAC_PREPARE_TITLE;
        y = 0.5f;
        break;
    case LSP_STEADY:
        sprite_name = "steady.png";
        lae = LAE_PREPARE_TITLE_ATLAS;
        lae_alpha = LAE_PREPARE_TITLE_ATLAS_ALPHA;
        lac = LAC_PREPARE_TITLE;
        y = 0.5f;
        break;
    case LSP_GO:
        if (update_tick > 50) {
            return;
        }
        sprite_name = "go.png";
        lae = LAE_PREPARE_TITLE_ATLAS;
        lae_alpha = LAE_PREPARE_TITLE_ATLAS_ALPHA;
        lac = LAC_PREPARE_TITLE;
        y = 0.5f;
        break;
    case LSP_FINISHED_DRAW: // DRAW == TIMEOUT
        sprite_name = "timeout.png";
        lae = LAE_RESULT_TITLE_ATLAS;
        lae_alpha = LAE_RESULT_TITLE_ATLAS_ALPHA;
        lac = LAC_RESULT_TITLE;
        break;
    case LSP_FINISHED_VICTORY_P1:
        sprite_name = player_no == 2 ? "defeat.png" : "victory.png";
        lae = LAE_RESULT_TITLE_ATLAS;
        lae_alpha = LAE_RESULT_TITLE_ATLAS_ALPHA;
        lac = LAC_RESULT_TITLE;
        break;
    case LSP_FINISHED_VICTORY_P2:
        sprite_name = player_no == 2 ? "victory.png" : "defeat.png";
        lae = LAE_RESULT_TITLE_ATLAS;
        lae_alpha = LAE_RESULT_TITLE_ATLAS_ALPHA;
        lac = LAC_RESULT_TITLE;
        break;
    default:
        // required parameters will be invalid if this is the case.
        return;
    }
    // show score diff message
    if (puck_game->score_message[0]) {
        // Render text
        LWTEXTBLOCK text_block;
        text_block.align = LTBA_CENTER_CENTER;
        text_block.text_block_width = 2.0f;
        text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
        text_block.size = DEFAULT_TEXT_BLOCK_SIZE_A;
        text_block.multiline = 1;
        SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 0, 1);
        SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
        SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
        SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
        text_block.begin_index = 0;
        float text_block_x = 0.0f;
        float text_block_y = 0.5f;
        text_block.text = puck_game->score_message;
        text_block.text_block_x = text_block_x;
        text_block.text_block_y = text_block_y;
        text_block.text_bytelen = (int)strlen(text_block.text);
        text_block.end_index = text_block.text_bytelen;
        render_text_block(pLwc, &text_block);
    }
    render_atlas_sprite(pLwc,
                        lac,
                        sprite_name,
                        lae,
                        lae_alpha,
                        sprite_width,
                        x,
                        y,
                        ui_alpha,
                        LVT_CENTER_CENTER_ANCHORED_SQUARE);
}

void render_caution_popup(const LWCONTEXT* pLwc, const char* str) {
    lw_load_tex(pLwc, LAE_UI_CAUTION_POPUP);
    lw_load_tex(pLwc, LAE_UI_CAUTION_POPUP_ALPHA);
    render_solid_vb_ui_alpha(pLwc,
                             0,
                             0,
                             1.5f,
                             1.5f,
                             pLwc->tex_atlas[LAE_UI_CAUTION_POPUP],
                             pLwc->tex_atlas[LAE_UI_CAUTION_POPUP_ALPHA],
                             LVT_CENTER_CENTER_ANCHORED_SQUARE,
                             1.0f,
                             0.0f,
                             0.0f,
                             0.0f,
                             0.0f);
    // Render text
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_LEFT_TOP;
    text_block.text_block_width = 1.0f;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_E;
    text_block.multiline = 1;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 0);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
    float x = -0.5f;
    float y = 0.15f;
    text_block.text = str;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = x;
    text_block.text_block_y = y;
    render_text_block(pLwc, &text_block);
}

static void render_popup_ui_layer(const LWCONTEXT* pLwc) {
    if (pLwc->tcp == 0) {
        char str[256];
        sprintf(str, "Could not connect to server.\n(is server ready?)");
        render_caution_popup(pLwc, str);
    } else if (pLwc->tcp->send_fail || (pLwc->udp && pLwc->udp->ready == 0)) {
        char str[256];
        sprintf(str, "Server disconnected.\nPlease check your internet\nconnection.");
        render_caution_popup(pLwc, str);
    }
}

static void render_main_menu(const LWCONTEXT* pLwc, const LWPUCKGAME* puck_game,
                             const mat4x4 view, const mat4x4 proj, float ui_alpha) {
    int shader_index = LWST_DEFAULT;
    const int vbo_index = LVT_CENTER_CENTER_ANCHORED_SQUARE;
    const float size = 0.75f;
    mat4x4 model;
    mat4x4_identity(model);
    //mat4x4_rotate_Y(model, model, (float)LWDEG2RAD(180));
    mat4x4_scale_aniso(model, model, size, size, size);
    mat4x4 model_translate;
    mat4x4_translate(model_translate, 0, 0, 0);
    mat4x4_mul(model, model, model_translate);

    //mult_world_roll(model, puck_game->world_roll_axis, puck_game->world_roll_dir, puck_game->world_roll);

    mat4x4 view_model;
    mat4x4 view_identity;
    mat4x4_identity(view_identity);
    mat4x4_mul(view_model, view_identity, model);

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
    lazy_tex_atlas_glBindTexture(pLwc, LAE_UI_MAIN_MENU_EN);
    set_tex_filter(GL_LINEAR, GL_LINEAR);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);

    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[vbo_index].vertex_count);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

static void render_icon_amount(const LWCONTEXT* pLwc,
                               const LWPUCKGAME* puck_game,
                               float x,
                               float y,
                               float width,
                               float height,
                               const char* str,
                               LW_ATLAS_ENUM lae,
                               LW_ATLAS_ENUM lae_alpha,
                               float font_r,
                               float font_g,
                               float font_b) {
    render_solid_vb_ui(pLwc,
                       x,
                       y,
                       width,
                       height,
                       0,
                       LVT_CENTER_CENTER_ANCHORED_SQUARE,
                       0.5f,
                       0.0f,
                       0.0f,
                       0.0f,
                       1.0f);
    if (lae_alpha == LAE_DONTCARE) {
        lw_load_tex(pLwc, lae);
        render_solid_vb_ui(pLwc,
                           x - width / 2 + 0.025f,
                           y,
                           height - 0.025f,
                           height - 0.025f,
                           pLwc->tex_atlas[lae],
                           LVT_LEFT_CENTER_ANCHORED_SQUARE,
                           1.0f,
                           0.0f,
                           0.0f,
                           0.0f,
                           0.0f);
    } else {
        lw_load_tex(pLwc, lae);
        lw_load_tex(pLwc, lae_alpha);
        render_solid_vb_ui_alpha(pLwc,
                                 x - width / 2 + 0.025f,
                                 y,
                                 height - 0.025f,
                                 height - 0.025f,
                                 pLwc->tex_atlas[lae],
                                 pLwc->tex_atlas[lae_alpha],
                                 LVT_LEFT_CENTER_ANCHORED_SQUARE,
                                 1.0f,
                                 0.0f,
                                 0.0f,
                                 0.0f,
                                 0.0f);
    }
    // Render text
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_LEFT_CENTER;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_C;
    text_block.multiline = 1;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, font_r, font_g, font_b, puck_game->main_menu_ui_alpha);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, puck_game->main_menu_ui_alpha);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, puck_game->main_menu_ui_alpha);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, puck_game->main_menu_ui_alpha);
    float tx = x - (width / 2) + (height - 0.025f) + 0.05f;
    float ty = y;
    text_block.text = str;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = tx;
    text_block.text_block_y = ty;
    render_text_block(pLwc, &text_block);
}

static void render_top_bar_nickname(const LWCONTEXT* pLwc, const LWPUCKGAME* puck_game, float* top_bar_x_cursor, float top_bar_x_cursor_margin) {
    const float width = 1.4f;
    const float height = 0.175f;
    const float x = *top_bar_x_cursor;
    const float y = 1.0f - height / 2;
    const LW_ATLAS_ENUM lae = LAE_PROFILE_ICON;
    const char* str = pLwc->puck_game->nickname[0];
    lw_load_tex(pLwc, lae);
    render_icon_amount(pLwc, puck_game, x, y, width, height, str, lae, LAE_DONTCARE, 1.0f, 1.0f, 1.0f);
    *top_bar_x_cursor += width + top_bar_x_cursor_margin;
}

static void render_main_menu_ui_layer(const LWCONTEXT* pLwc,
                                      const LWPUCKGAME* puck_game,
                                      const mat4x4 view,
                                      const mat4x4 proj,
                                      const mat4x4 ui_proj,
                                      int remote,
                                      const float* player_controlled_pos) {
    // reset viewport temporarily
    const int viewport_x = pLwc->viewport_x;
    const int viewport_y = pLwc->viewport_y;
    ((LWCONTEXT*)pLwc)->viewport_x = 0;
    ((LWCONTEXT*)pLwc)->viewport_y = 0;
    LW_GL_VIEWPORT();
    // leaderboard
    float leaderboard_x = -pLwc->viewport_rt_x + 0.1f;
    float leaderboard_y = pLwc->viewport_rt_y - 0.15f;
    render_leaderboard_table(pLwc, leaderboard_x, leaderboard_y, puck_game->main_menu_ui_alpha);
    // render buttons appended so far while temporary viewport configuration valid
    render_lwbutton(pLwc, &pLwc->button_list);
    // revert to default viewport
    ((LWCONTEXT*)pLwc)->viewport_x = viewport_x;
    ((LWCONTEXT*)pLwc)->viewport_y = viewport_y;
    LW_GL_VIEWPORT();
    float top_bar_x_cursor = 0;
    const float top_bar_x_cursor_margin = 0.05f;
    // nickname (background, icon, text)
    render_top_bar_nickname(pLwc, puck_game, &top_bar_x_cursor, top_bar_x_cursor_margin);
    if (puck_game->show_top_level_main_menu) {
        // render buttons as a single sprite
        render_main_menu(pLwc, puck_game, view, proj, puck_game->main_menu_ui_alpha);
        // buttons
        float button_alpha = 0.0f; // alpha zeroed intentionally (nonzero only when debugging)
        lwbutton_lae_append(pLwc,
                            &(((LWCONTEXT*)pLwc)->button_list),
                            "practice_button",
                            -0.75f,
                            +0.70f,
                            +0.70f,
                            +0.40f,
                            0,
                            0,
                            puck_game->main_menu_ui_alpha * button_alpha,
                            1.0f,
                            1.0f,
                            1.0f);
        lwbutton_lae_append(pLwc,
                            &(((LWCONTEXT*)pLwc)->button_list),
                            "tutorial_button",
                            +0.75f - 0.70f,
                            +0.70f,
                            +0.70f,
                            +0.40f,
                            0,
                            0,
                            puck_game->main_menu_ui_alpha * button_alpha,
                            1.0f,
                            1.0f,
                            1.0f);
        lwbutton_lae_append(pLwc,
                            &(((LWCONTEXT*)pLwc)->button_list),
                            "online_button",
                            -0.75f,
                            +0.25f,
                            +0.75f * 2,
                            +0.50f,
                            0,
                            0,
                            puck_game->main_menu_ui_alpha * button_alpha,
                            1.0f,
                            1.0f,
                            1.0f);
        lwbutton_lae_append(pLwc,
                            &(((LWCONTEXT*)pLwc)->button_list),
                            "change_nickname_button",
                            -0.75f,
                            +0.25f - 0.50f - 0.05f,
                            +0.70f,
                            +0.40f,
                            0,
                            0,
                            puck_game->main_menu_ui_alpha * button_alpha,
                            1.0f,
                            1.0f,
                            1.0f);
        lwbutton_lae_append(pLwc,
                            &(((LWCONTEXT*)pLwc)->button_list),
                            "more_button",
                            +0.75f - 0.70f,
                            +0.25f - 0.50f - 0.05f,
                            +0.70f,
                            +0.40f,
                            0,
                            0,
                            puck_game->main_menu_ui_alpha * button_alpha,
                            1.0f,
                            1.0f,
                            1.0f);
    }
    if (puck_game->show_html_ui) {
        // reset viewport temporarily
        const int viewport_x = pLwc->viewport_x;
        const int viewport_y = pLwc->viewport_y;
        ((LWCONTEXT*)pLwc)->viewport_x = 0;
        ((LWCONTEXT*)pLwc)->viewport_y = 0;
        LW_GL_VIEWPORT();
        // render FBO (HTML UI)
        render_solid_box_ui_lvt_flip_y_uv(pLwc,
                                          0,
                                          0,
                                          2 * pLwc->viewport_rt_x,
                                          2 * pLwc->viewport_rt_y,
                                          pLwc->shared_fbo.color_tex,
                                          LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                          1);
        // revert to default viewport
        ((LWCONTEXT*)pLwc)->viewport_x = viewport_x;
        ((LWCONTEXT*)pLwc)->viewport_y = viewport_y;
        LW_GL_VIEWPORT();
    }
}

static void render_tower_invincible_mark(const LWCONTEXT* pLwc,
                                         const vec3 tower_pos,
                                         float ui_alpha,
                                         float elapsed_from_last_damage,
                                         float invincible_time) {
    vec4 tower_pos_vec4 = {
        tower_pos[0],
        tower_pos[1],
        tower_pos[2],
        1,
    };
    float ratio = elapsed_from_last_damage / invincible_time;
    int flicker_loop_count = 5;
    float size = 0.1f;
    mat4x4 proj_view;
    mat4x4_identity(proj_view);
    mat4x4_mul(proj_view, pLwc->puck_game_proj, pLwc->puck_game_view);
    vec2 ui_point;
    calculate_ui_point_from_world_point(pLwc->viewport_aspect_ratio, proj_view, tower_pos_vec4, ui_point);
    int lae = LAE_STOP_MARK;
    int lae_alpha = LAE_STOP_MARK_ALPHA;
    lw_load_tex(pLwc, lae);
    lw_load_tex(pLwc, lae_alpha);
    render_solid_vb_ui_alpha_uv(pLwc,
                                ui_point[0],
                                ui_point[1],
                                size,
                                size,
                                pLwc->tex_atlas[lae],
                                pLwc->tex_atlas[lae_alpha],
                                LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                ui_alpha * fabsf(sinf((float)(M_PI * ratio * flicker_loop_count))),
                                0,
                                0,
                                0,
                                0,
                                default_uv_offset,
                                default_uv_scale);
}

void render_puck_exclamation_mark(const LWCONTEXT* pLwc, const LWPUCKGAME* puck_game, const vec3 puck_pos, float puck_speed, float ui_alpha) {
    vec4 puck_pos_vec4 = {
        puck_pos[0],
        puck_pos[1],
        puck_pos[2],
        1.0f,
    };
    if (puck_game_state_phase_battling(puck_game->battle_phase) == 0
        && puck_game->battle_phase != LSP_TUTORIAL) {
        return;
    }
    if (puck_game->go[LPGO_PUCK].geom == 0) {
        return;
    }
    if (puck_game->puck_owner_player_no != 0 && puck_speed >= 1.0f) {
        float size = 0.125f;
        mat4x4 proj_view;
        mat4x4_identity(proj_view);
        mat4x4_mul(proj_view, pLwc->puck_game_proj, pLwc->puck_game_view);
        vec2 ui_point;
        calculate_ui_point_from_world_point(pLwc->viewport_aspect_ratio, proj_view, puck_pos_vec4, ui_point);
        int lae = LAE_EXCLAMATION_MARK;
        int lae_alpha = LAE_EXCLAMATION_MARK_ALPHA;
        lw_load_tex(pLwc, lae);
        lw_load_tex(pLwc, lae_alpha);
        render_solid_vb_ui_alpha_uv(pLwc,
                                    ui_point[0],
                                    ui_point[1],
                                    size,
                                    size,
                                    pLwc->tex_atlas[lae],
                                    pLwc->tex_atlas[lae_alpha],
                                    LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                    ui_alpha * fabsf(sinf((float)(M_PI * puck_game->time * 10))),
                                    0,
                                    0,
                                    0,
                                    0,
                                    default_uv_offset,
                                    default_uv_scale);
    }
}

void render_all_tower_invincible_mark(const LWCONTEXT* pLwc, const LWPUCKGAME* puck_game, float ui_alpha) {
    if (puck_game_state_phase_battling(puck_game->battle_phase) == 0
        && puck_game->battle_phase != LSP_TUTORIAL) {
        return;
    }
    for (int i = 0; i < LW_PUCK_GAME_TOWER_COUNT; i++) {
        int tower_index = i;
        const LWPUCKGAMETOWER* tower = &puck_game->tower[tower_index];
        if (tower->geom == 0) {
            continue;
        }
        vec3 tower_pos = {
            puck_game->tower_pos * puck_game->tower_pos_multiplier[tower_index][0],
            puck_game->tower_pos * puck_game->tower_pos_multiplier[tower_index][1],
            0.845f, // hard-coded
        };
        float elapsed_from_last_damage = (float)(lwtimepoint_now_seconds() - tower->last_damaged_at);
        float invincible_time = 1.0f;
        if (elapsed_from_last_damage < invincible_time || tower->invincible) {
            render_tower_invincible_mark(pLwc, tower_pos, ui_alpha, elapsed_from_last_damage, invincible_time);
        }
    }
}

static void render_dash_ring_gauge_player_pos(const LWCONTEXT* pLwc, const LWPUCKGAME* puck_game, const vec3 player_controlled_pos, float ui_alpha) {
    vec4 player_controlled_pos_vec4 = {
        player_controlled_pos[0],
        player_controlled_pos[1],
        player_controlled_pos[2],
        1.0f,
    };
    vec4 player_controlled_pos_vec4_world_roll;
    mat4x4 world_roll_mat;
    mat4x4_identity(world_roll_mat);
    mult_world_roll(pLwc->viewport_aspect_ratio, world_roll_mat, puck_game->world_roll_axis, puck_game->world_roll_dir, puck_game->world_roll);
    mat4x4_mul_vec4(player_controlled_pos_vec4_world_roll, world_roll_mat, player_controlled_pos_vec4);
    render_dash_ring_gauge(pLwc, player_controlled_pos_vec4_world_roll, ui_alpha);
}

static void render_battle_ui_layer(const LWCONTEXT* pLwc,
                                   const LWPUCKGAME* puck_game,
                                   const mat4x4 view,
                                   const mat4x4 proj,
                                   const mat4x4 ui_proj,
                                   int remote,
                                   const float* player_controlled_pos) {
    const LWPSTATE* state = &pLwc->puck_game_state;
    const LWPUCKGAMEPLAYER* player = &puck_game->pg_player[0];
    const LWPUCKGAMEPLAYER* target = &puck_game->pg_target[0];
    float ui_alpha = puck_game->battle_ui_alpha;
    float control_ui_alpha = puck_game->battle_control_ui_alpha;
    vec2 world_right_top_end_ui_point;
    calculate_world_right_top_end_ui_point(pLwc, puck_game, world_right_top_end_ui_point);
    //render_solid_vb_ui(pLwc, world_right_top_end_ui_point[0], world_right_top_end_ui_point[1], 0.1f, 0.1f, 0, LVT_CENTER_CENTER_ANCHORED_SQUARE, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f);
    // HP gauges (player & target)
    const char* target_nickname = puck_game->battle_id ? puck_game->target_nickname[0] : "Bogus Opponent";
    const int player_current_hp = remote ? state->bf.player_current_hp : puck_game->pg_player[0].current_hp;
    if (puck_game->hide_hp_star == 0 && puck_game->tower[0].geom) {
        char player_score[32];
        sprintf(player_score, "%d", puck_game->player_score[0]);
        //        const float gauge_width = pLwc->viewport_aspect_ratio * 0.9f;
        //        const float gauge_height = 0.075f;
        //        const float gauge1_x = (-world_right_top_end_ui_point[0] - pLwc->viewport_aspect_ratio) / 2 + gauge_width / 2;
        //        const float gauge1_y = 1.0f - gauge_height;
        //        const int player_total_hp = remote ? state->bf.player_total_hp : puck_game->player.total_hp;
        //        render_hp_gauge(pLwc,
        //                        gauge_width,
        //                        gauge_height,
        //                        gauge1_x,
        //                        gauge1_y,
        //                        player_current_hp,
        //                        player_total_hp,
        //                        player->hp_shake_remain_time,
        //                        1,
        //                        puck_game->nickname,
        //                        player_score,
        //                        ui_alpha);
        render_nickname_score(pLwc, 1, ui_alpha, puck_game->nickname[0], player_score, world_right_top_end_ui_point);
    }
    const int target_current_hp = remote ? state->bf.target_current_hp : puck_game->pg_target[0].current_hp;
    if (puck_game->hide_hp_star == 0 && puck_game->tower[1].geom) {
        char target_score[32];
        sprintf(target_score, "%d", puck_game->target_score[0]);
        //        const float gauge_width = pLwc->viewport_aspect_ratio * 0.9f;
        //        const float gauge_height = 0.075f;
        //        const float gauge2_x = pLwc->viewport_aspect_ratio - gauge_width / 2;
        //        const float gauge2_y = 1.0f - gauge_height;
        //        const int target_total_hp = remote ? state->bf.target_total_hp : puck_game->target.total_hp;
        //        render_hp_gauge(pLwc,
        //                        gauge_width,
        //                        gauge_height,
        //                        gauge2_x,
        //                        gauge2_y,
        //                        target_current_hp,
        //                        target_total_hp,
        //                        target->hp_shake_remain_time,
        //                        0,
        //                        target_nickname,
        //                        target_score,
        //                        ui_alpha);
        render_nickname_score(pLwc, 0, ui_alpha, target_nickname, target_score, world_right_top_end_ui_point);
    }
    // Battle timer (center top of the screen)
    const float remain_time = puck_game_remain_time(puck_game->total_time,
                                                    remote ? state->update_tick : puck_game->update_tick,
                                                    pLwc->update_frequency);
    // battle timer
    if ((puck_game->control_flags & LPGCF_HIDE_TIMER) == 0) {
        render_timer(pLwc,
                     remain_time,
                     puck_game->total_time,
                     ui_alpha);
    }
    // HP star (test)
    if (puck_game->hide_hp_star == 0 && puck_game->tower[0].geom) {
        render_hp_star(pLwc, puck_game, ui_alpha, player_current_hp, 1, player->hp_shake_remain_time, world_right_top_end_ui_point);
    }
    if (puck_game->hide_hp_star == 0 && puck_game->tower[1].geom) {
        render_hp_star(pLwc, puck_game, ui_alpha, target_current_hp, 0, target->hp_shake_remain_time, world_right_top_end_ui_point);
    }
    // Match state text (bottom of the screen)
    //render_match_state(pLwc, ui_alpha);
    // Dash ring gauge
    render_dash_ring_gauge_player_pos(pLwc, puck_game, player_controlled_pos, ui_alpha * control_ui_alpha);
    // render puck exclamation mark
    render_puck_exclamation_mark(pLwc, puck_game, puck_game->go[LPGO_PUCK].pos, puck_game->go[LPGO_PUCK].speed, ui_alpha);
    // render tower invincible mark
    render_all_tower_invincible_mark(pLwc, puck_game, ui_alpha);
    // tutorial guide text
    render_tutorial_guide(pLwc, puck_game, ui_alpha);
    // Render damage texts
    render_damage_text(pLwc, view, proj, ui_proj, ui_alpha);
    // Render left dir pad
    if ((puck_game->control_flags & LPGCF_HIDE_LEFT_DIR_PAD) == 0) {
        render_dir_pad_with_start_joystick(pLwc, &pLwc->left_dir_pad, ui_alpha * control_ui_alpha);
    }
    // Render right joystick
    if (pLwc->control_flags & LCF_PUCK_GAME_RIGHT_DIR_PAD) {
        render_dir_pad_with_start(pLwc, &pLwc->right_dir_pad);
    }
    // Dash button
    //render_fist_button(pLwc);
    // Pull button
    //render_top_button(pLwc);
    // Dash cooltime gauge
    //render_dash_gauge(pLwc);
    // battle result
    render_battle_result_popup(pLwc,
                               puck_game,
                               puck_game->player_no,
                               puck_game->update_tick,
                               puck_game->battle_phase,
                               puck_game->battle_ui_alpha);
    // Register as a button
    const float button_size = 0.35f;
    const float button_margin_x = 0.025f;
    const float button_x_interval = button_size + button_margin_x;
    const float button_x_0 = 0.25f;
    const float button_y_interval = 0.25f;
    const float button_y_0 = -0.50f;
    if ((pLwc->control_flags & LCF_PUCK_GAME_PULL)
        && ((puck_game->control_flags & LPGCF_HIDE_PULL_BUTTON) == 0)) {
        int pull_puck = puck_game->remote_control[LW_PUCK_GAME_PLAYER_TEAM][0].pull_puck;
        lwbutton_lae_append(pLwc,
                            &(((LWCONTEXT*)pLwc)->button_list),
                            "pull_button",
                            +1.000f + button_size * 1.75f / 2 - button_size / 2,
                            +0.175f,
                            button_size,
                            button_size,
                            LAE_BUTTON_PULL,
                            LAE_BUTTON_PULL_ALPHA,
                            ui_alpha * control_ui_alpha,
                            pull_puck ? 0.2f : 1.0f,
                            1.0f,
                            pull_puck ? 0.2f : 1.0f);
    }
    if (pLwc->control_flags & LCF_PUCK_GAME_DASH
        && ((puck_game->control_flags & LPGCF_HIDE_DASH_BUTTON) == 0)) {
        float cx, cy;
        float sx, sy;
        get_left_dir_pad_original_center(pLwc->viewport_aspect_ratio, &cx, &cy);
        sx = button_size * 1.75f;
        sy = sx;
        lwbutton_lae_append(pLwc,
                            &(((LWCONTEXT*)pLwc)->button_list),
                            "dash_button",
                            -cx - sx / 2,
                            cy + sy / 2,
                            sx,
                            sy,
                            LAE_BUTTON_DASH,
                            LAE_BUTTON_DASH_ALPHA,
                            ui_alpha * control_ui_alpha,
                            1.0f,
                            1.0f,
                            1.0f);
    }
    if (pLwc->control_flags & LCF_PUCK_GAME_JUMP) {
        lwbutton_lae_append(pLwc,
                            &(((LWCONTEXT*)pLwc)->button_list),
                            "jump_button",
                            button_x_0 + button_x_interval * 3,
                            button_y_0 + button_y_interval * 2,
                            button_size,
                            button_size,
                            LAE_BUTTON_JUMP,
                            LAE_BUTTON_JUMP_ALPHA,
                            ui_alpha * control_ui_alpha,
                            1.0f,
                            1.0f,
                            1.0f);
    }
    if (puck_game->game_state == LPGS_PRACTICE
        || (puck_game->game_state == LPGS_TUTORIAL && puck_game_is_tutorial_stoppable(puck_game))
        || (puck_game->game_state == LPGS_BATTLE && puck_game_state_phase_finished(puck_game->battle_phase))) {
        // return to main menu button
        lwbutton_lae_append(pLwc,
                            &(((LWCONTEXT*)pLwc)->button_list),
                            "back_button",
                            -pLwc->viewport_rt_x,
                            +pLwc->viewport_rt_y,
                            button_size * 1.5f,
                            button_size * 1.5f,
                            LAE_UI_BACK_BUTTON,
                            LAE_UI_BACK_BUTTON,
                            ui_alpha,
                            1.0f,
                            1.0f,
                            1.0f);
    }
}

void lwc_render_puck_game(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj) {
    const LWPUCKGAME* puck_game = pLwc->puck_game;
    const LWPSTATE* state = &pLwc->puck_game_state;
    LW_GL_VIEWPORT();
    lw_clear_color();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //int shader_index = LWST_DEFAULT;
    int wall_shader_index = pLwc->lowend_device ? LWST_DEFAULT : LWST_SPHERE_REFLECT;
    int floor_shader_index = pLwc->lowend_device ? LWST_DEFAULT : LWST_SPHERE_REFLECT_FLOOR;

    const int remote = puck_game_remote(pLwc, pLwc->puck_game);
    const float* player_pos = puck_game->go[LPGO_PLAYER].pos;
    const float* target_pos = puck_game->go[LPGO_TARGET].pos;
    const float* puck_pos = puck_game->go[LPGO_PUCK].pos;
    const float* remote_player_pos = 0;
    const float* remote_puck_pos = 0;
    const float* remote_target_pos = 0;
    if (remote) {
        remote_player_pos = state->player;
        remote_puck_pos = state->puck;
        remote_target_pos = state->target;
    } else {
        remote_player_pos = player_pos;
        remote_puck_pos = puck_pos;
        remote_target_pos = target_pos;
    }

    const float* player_controlled_pos = remote_player_pos;
    if (puck_game->player_no == 2) {
        player_controlled_pos = remote_target_pos;
    }

    float arrow_scale = 0.4f;
    float arrowAngle = (float)LWDEG2RAD(70.0f);
    float dx = 0, dy = 0, dlen = 0;
    int fire = 0;
    if (lw_get_normalized_dir_pad_input(pLwc, &pLwc->right_dir_pad, &dx, &dy, &dlen) && (dx || dy)) {
        fire = 1;
        arrowAngle = atan2f(dy, dx);
        arrow_scale = LWMIN(0.01f + dlen * 0.7f, 1.0f);
    }
    if (pLwc->puck_game->player_no != 2) {
        arrow_scale *= -1;
    }

    const float* puck_sphere_col;
    int puck_owner_player_no = remote ? state->bf.puck_owner_player_no : puck_game->puck_owner_player_no;
    // these are ground lighting colors, NOT sphere colors itself
    static const float gray_sphere[3] = { 0.6f, 0.6f, 0.6f };
    static const float reddish_sphere[3] = { 1.0f, 0.0f, 0.0f };
    static const float bluish_sphere[3] = { 0.0f, 0.0f, 1.0f };
    if (puck_owner_player_no == 0) {
        puck_sphere_col = gray_sphere;
    } else if (puck_owner_player_no == 1) {
        if (puck_game->player_no == 2) {
            puck_sphere_col = reddish_sphere;
        } else {
            puck_sphere_col = bluish_sphere;
        }
    } else {
        if (puck_game->player_no == 2) {
            puck_sphere_col = bluish_sphere;
        } else {
            puck_sphere_col = reddish_sphere;
        }
    }
    LWSPHERERENDERUNIFORM sphere_render_uniform = {
        // float sphere_col_ratio[3];
        { 1.0f, 1.0f, 1.0f },
        // float sphere_pos[3][3];
        {
            { remote_player_pos[0], remote_player_pos[1], remote_player_pos[2] },
            { remote_target_pos[0], remote_target_pos[1], remote_target_pos[2] },
            { remote_puck_pos[0], remote_puck_pos[1], remote_puck_pos[2] }
        },
        // float sphere_col[3][3]; --- ground lighting color, NOT sphere color itself
        {
            { 0.0f, 1.0f, 0.8f },
            { 1.0f, 0.0f, 0.0f },
            { puck_sphere_col[0], puck_sphere_col[1], puck_sphere_col[2] }
        },
        // float sphere_speed[3];
        {
            !remote ? puck_game->go[LPGO_PLAYER].speed : state->player_speed,
            !remote ? puck_game->go[LPGO_TARGET].speed : state->target_speed,
            1.5f * (!remote ? puck_game->go[LPGO_PUCK].speed : state->puck_speed)
        },
        // float sphere_move_rad[3];
        {
            !remote ? puck_game->go[LPGO_PLAYER].move_rad : state->player_move_rad,
            !remote ? puck_game->go[LPGO_TARGET].move_rad : state->target_move_rad,
            !remote ? puck_game->go[LPGO_PUCK].move_rad : state->puck_move_rad
        },
        // float reflect_size[3];
        {
            puck_game->go[LPGO_PLAYER].body ? 1.0f : 0.0f,
            puck_game->go[LPGO_TARGET].body ? 1.0f : 0.0f,
            puck_game->go[LPGO_PUCK].body ? (remote ? state->puck_reflect_size : puck_game->puck_reflect_size) : 0.0f,
        },
        // float arrowRotMat[2][2];
        {
            { cosf(-arrowAngle), -sinf(-arrowAngle) },
            { sinf(-arrowAngle), +cosf(-arrowAngle) },
        },
        // float arrow_center[2];
        {
            fire ? +player_controlled_pos[0] / puck_game->world_width : 999.0f,
            fire ? -player_controlled_pos[1] / puck_game->world_height : 999.0f,
        },
        // float arrow_scale;
        arrow_scale,
    };
    // Switch sphere reflection color if player number is 2.
    if (puck_game->player_no == 2) {
        sphere_render_uniform.sphere_col[0][0] = 1.0f;
        sphere_render_uniform.sphere_col[0][1] = 0.0f;
        sphere_render_uniform.sphere_col[0][2] = 0.0f;

        sphere_render_uniform.sphere_col[1][0] = 0.0f;
        sphere_render_uniform.sphere_col[1][1] = 1.0f;
        sphere_render_uniform.sphere_col[1][2] = 0.8f;
    }

    render_puck_game_menu(pLwc, proj, view, puck_game);

    if (pLwc->puck_game_stage_lvt != LVT_DONTCARE
        && pLwc->puck_game_stage_lae != LAE_DONTCARE) {
        render_custom_stage(pLwc,
                            proj,
                            view,
                            puck_game,
                            pLwc->puck_game_stage_lvt,
                            pLwc->puck_game_stage_lae);
    } else {
        render_default_stage(pLwc,
                             proj,
                             view,
                             puck_game,
                             floor_shader_index,
                             wall_shader_index,
                             &sphere_render_uniform);
    }

    const int player_no = pLwc->puck_game->player_no;
    // Game object: Puck
    const int puck_lae = puck_game->puck_lae;
    lw_load_tex(pLwc, puck_lae);
    render_go(pLwc,
              view,
              proj,
              puck_game,
              &puck_game->go[LPGO_PUCK],
              pLwc->tex_atlas[puck_lae],
              1.0f,
              remote_puck_pos,
              state->puck_rot,
              remote,
              !remote ? puck_game->go[LPGO_PUCK].speed : state->puck_speed,
              LWST_DEFAULT);
    // Game object: Player
    const int player_lae = player_no == 2 ? puck_game->target_lae : puck_game->player_lae;
    lw_load_tex(pLwc, player_lae);
    render_go(pLwc,
              view,
              proj,
              puck_game,
              &puck_game->go[LPGO_PLAYER],
              pLwc->tex_atlas[player_lae],
              1.0f,
              remote_player_pos,
              state->player_rot,
              remote,
              0,
              LWST_DEFAULT);
    if (remote ? state->bf.player_pull : puck_game->remote_control[LW_PUCK_GAME_PLAYER_TEAM][0].pull_puck) {
        render_radial_wave(pLwc,
                           view,
                           proj,
                           puck_game,
                           &puck_game->go[LPGO_PLAYER],
                           pLwc->tex_atlas[player_lae],
                           1.0f,
                           remote_player_pos,
                           state->player_rot,
                           remote,
                           0);
    }
    // Game object: Enemy
    const int target_lae = player_no == 2 ? puck_game->player_lae : puck_game->target_lae;
    lw_load_tex(pLwc, target_lae);
    render_go(pLwc,
              view,
              proj,
              puck_game,
              &puck_game->go[LPGO_TARGET],
              pLwc->tex_atlas[target_lae],
              1.0f,
              remote_target_pos,
              state->target_rot, remote,
              0,
              LWST_DEFAULT);
    if (state->bf.target_pull) {
        render_radial_wave(pLwc,
                           view,
                           proj,
                           puck_game,
                           &puck_game->go[LPGO_TARGET],
                           pLwc->tex_atlas[target_lae],
                           1.0f,
                           remote_target_pos,
                           state->target_rot,
                           remote,
                           0);
    }
    // Towers
    for (int i = 0; i < LW_PUCK_GAME_TOWER_COUNT; i++) {
        if (puck_game->tower[i].geom == 0) {
            continue;
        }
        float tower_pos[] = {
            puck_game->tower_pos * puck_game->tower_pos_multiplier[i][0],
            puck_game->tower_pos * puck_game->tower_pos_multiplier[i][1],
            0
        };
        render_tower(pLwc,
                     view,
                     proj,
                     puck_game,
                     tower_pos,
                     &puck_game->tower[i],
                     remote,
                     puck_game->tower_collapsing_z_rot_angle[i]);
    }

    switch (puck_game->game_state) {
    case LPGS_SEARCHING:
    {
        render_searching_state(pLwc, puck_game);
        const float button_size = 0.35f;
        // search cancel button
        lwbutton_lae_append(pLwc,
                            &(((LWCONTEXT*)pLwc)->button_list),
                            "back_button",
                            +0.0f - button_size * 1.5f / 2,
                            -0.5f + button_size * 1.5f / 2,
                            button_size * 1.5f,
                            button_size * 1.5f,
                            LAE_UI_BACK_BUTTON,
                            LAE_UI_BACK_BUTTON,
                            1.0f,
                            1.0f,
                            1.0f,
                            1.0f);
        break;
    }
    case LPGS_MAIN_MENU:
    {
        if (puck_game->main_menu_ui_alpha) {
            render_main_menu_ui_layer(pLwc, puck_game, view, proj, pLwc->proj, remote, player_controlled_pos);
        }
        break;
    }
    case LPGS_TUTORIAL:
    case LPGS_PRACTICE:
    case LPGS_BATTLE:
    {
        if (puck_game->battle_ui_alpha) {
            glDisable(GL_DEPTH_TEST);
            render_battle_ui_layer(pLwc, puck_game, view, proj, pLwc->proj, remote, player_controlled_pos);
            glEnable(GL_DEPTH_TEST);
        }
        break;
    }
    case LPGS_BATTLE_PREPARE:
    {
        break;
    }
    }
    glDisable(GL_DEPTH_TEST);
    // render buttons (shared)
    render_lwbutton(pLwc, &pLwc->button_list);
    // popup UI layer
    render_popup_ui_layer(pLwc);
    glEnable(GL_DEPTH_TEST);
}
