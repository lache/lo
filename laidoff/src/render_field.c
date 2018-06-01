#include "lwcontext.h"
#include "render_field.h"
#include "lwlog.h"
#include "lwmacro.h"
#include "laidoff.h"
#include "render_solid.h"
#include "render_skin.h"
#include "field.h"
#include "mq.h"
#include "render_fan.h"
#include "ps.h"
#include "render_ps.h"
#include "nav.h"
#include "lwtimepoint.h"
#include "lwfieldobject.h"
#include "render_ui.h"
#include <float.h>
#include "platform_detection.h"
#include "lwatlasenum.h"
#include <assert.h>

static void render_field_object_rot(const LWCONTEXT* pLwc, int vbo_index, GLuint tex_id, const mat4x4 view, const mat4x4 proj, float x, float y, float z, float sx, float sy, float sz, float alpha_multiplier, int mipmap, const mat4x4 rot) {
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
    
    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_mul(model, model, rot);
    mat4x4_scale_aniso(model, model, sx, sy, sz);
    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, z);
    mat4x4_mul(model, model_translate, model);
    
    mat4x4 view_model;
    mat4x4_mul(view_model, view, model);
    
    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);
    
    lazy_glUseProgram(pLwc, shader_index);
    lazy_glBindBuffer(pLwc, vbo_index);
    bind_all_vertex_attrib(pLwc, vbo_index);
    glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    assert(tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    set_tex_filter(mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR, GL_LINEAR);
    
    const float gird_uv_offset[2] = { 0, 0 };
    const float grid_uv_scale[2] = { 1, 1 };
    
    glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, gird_uv_offset);
    glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, grid_uv_scale);
    glUniform1fv(pLwc->shader[shader_index].alpha_multiplier_location, 1, &alpha_multiplier);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[vbo_index].vertex_count);
}

static void render_field_object(const LWCONTEXT* pLwc, int vbo_index, GLuint tex_id, const mat4x4 view, const mat4x4 proj, float x, float y, float z, float sx, float sy, float sz, float alpha_multiplier, int mipmap, float rot_z) {
    mat4x4 mat_iden, mat_rot;
    mat4x4_identity(mat_iden);
    mat4x4_rotate_Z(mat_rot, mat_iden, rot_z);
    render_field_object_rot(pLwc, vbo_index, tex_id, view, proj, x, y, z, sx, sy, sz, alpha_multiplier, mipmap, mat_rot);
}

void render_fist_button(const LWCONTEXT* pLwc) {
    const float fist_icon_margin_x = 0.3f;
    const float fist_icon_margin_y = 0.2f;
    const float fist_icon_width = 0.75f;
    const float fist_icon_height = 0.75f;
    lw_load_tex(pLwc, LAE_U_FIST_ICON_KTX);
    lw_load_tex(pLwc, LAE_U_FIST_ICON_ALPHA_KTX);
    render_solid_vb_ui_alpha(pLwc,
                             pLwc->viewport_aspect_ratio - fist_icon_margin_x,
                             -1 + fist_icon_margin_y,
                             fist_icon_width,
                             fist_icon_height,
                             pLwc->tex_atlas[LAE_U_FIST_ICON_KTX],
                             pLwc->tex_atlas[LAE_U_FIST_ICON_ALPHA_KTX],
                             LVT_RIGHT_BOTTOM_ANCHORED_SQUARE,
                             1,
                             0,
                             0,
                             0,
                             0);
}

void render_top_button(const LWCONTEXT* pLwc) {
    const float fist_icon_margin_x = 0.3f;
    const float fist_icon_margin_y = 0.2f;
    const float fist_icon_width = 0.75f;
    const float fist_icon_height = 0.75f;
    lw_load_tex(pLwc, LAE_U_FIST_ICON_KTX);
    lw_load_tex(pLwc, LAE_U_FIST_ICON_ALPHA_KTX);
    render_solid_vb_ui_alpha(pLwc,
                             pLwc->viewport_aspect_ratio - fist_icon_margin_x,
                             1 - fist_icon_margin_y,
                             fist_icon_width,
                             fist_icon_height,
                             pLwc->tex_atlas[LAE_U_FIST_ICON_KTX],
                             pLwc->tex_atlas[LAE_U_FIST_ICON_ALPHA_KTX],
                             LVT_RIGHT_TOP_ANCHORED_SQUARE,
                             1,
                             0,
                             0,
                             0,
                             0);
}

static void s_render_ui(const LWCONTEXT* pLwc) {
    render_dir_pad(pLwc, pLwc->left_dir_pad.x, pLwc->left_dir_pad.y);
    render_basic_field_ui(pLwc);
}

void render_debug_sphere(const LWCONTEXT* pLwc, GLuint tex_id, const mat4x4 perspective, const mat4x4 view, float x, float y, float z, float s) {
    
    render_field_object(pLwc,
                        LVT_SPHERE,
                        tex_id,
                        view,
                        perspective,
                        x,
                        y,
                        z,
                        s,
                        s,
                        s,
                        1,
                        1,
                        0);
}

void render_tower_yaw(const LWCONTEXT* pLwc, const mat4x4 perspective, const mat4x4 view,
                      float x, float y, float yaw, LW_ACTION action, float animtime, int loop, float alpha,
                      LW_ATLAS_ENUM atlas, LW_SKIN_VBO_TYPE skin_vbo, LW_ARMATURE armature) {
    
    const float path_query_test_player_pos[] = { x, y, 0 };
    const float skin_scale_f = 0.5f;
    
    mat4x4 skin_trans;
    mat4x4_identity(skin_trans);
    mat4x4_translate(skin_trans, path_query_test_player_pos[0], path_query_test_player_pos[1], path_query_test_player_pos[2]);
    mat4x4 skin_scale;
    mat4x4_identity(skin_scale);
    mat4x4_scale_aniso(skin_scale, skin_scale, skin_scale_f, skin_scale_f, skin_scale_f);
    mat4x4 skin_rot;
    mat4x4_identity(skin_rot);
    mat4x4_rotate_Z(skin_rot, skin_rot, 0);
    
    mat4x4 skin_model;
    mat4x4_identity(skin_model);
    mat4x4_mul(skin_model, skin_rot, skin_model);
    mat4x4_mul(skin_model, skin_scale, skin_model);
    mat4x4_mul(skin_model, skin_trans, skin_model);
    
    const float flash = 0;
    lw_load_tex(pLwc, atlas);
    render_yaw_skin(pLwc,
                    pLwc->tex_atlas[atlas],
                    skin_vbo,
                    &pLwc->action[action],
                    &pLwc->armature[armature],
                    alpha, 1, 1, 1, flash, perspective, view, skin_model, animtime, loop, yaw);
}

void render_guntower_yaw(const LWCONTEXT* pLwc, const mat4x4 perspective, const mat4x4 view,
                         float x, float y, float yaw, LW_ACTION action, float animtime, int loop, float alpha) {
    
    render_tower_yaw(pLwc, perspective, view, x, y, yaw, action, animtime, loop, alpha, LAE_GUNTOWER_KTX, LSVT_GUNTOWER, LWAR_GUNTOWER_ARMATURE);
}

void render_guntower(const LWCONTEXT* pLwc, const mat4x4 perspective, const mat4x4 view, float x, float y, LW_ACTION action, float animtime, int loop) {
    
    float player_x = 0, player_y = 0, player_z = 0;
    get_field_player_position(pLwc->field, &player_x, &player_y, &player_z);
    
    float a = atan2f(player_y - y, player_x - x);
    
    render_guntower_yaw(pLwc, perspective, view, x, y, a, action, animtime, loop, 1.0f);
}

static void s_render_path_query_test_player(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj) {
    float spos[3], epos[3];
    nav_path_query_spos(field_nav(pLwc->field), spos);
    nav_path_query_epos(field_nav(pLwc->field), epos);
    
    render_debug_sphere(pLwc,
                        pLwc->tex_programmed[LPT_SOLID_RED],
                        proj,
                        view,
                        spos[0],
                        spos[1],
                        spos[2],
                        1.0f);
    
    render_debug_sphere(pLwc,
                        pLwc->tex_programmed[LPT_SOLID_BLUE],
                        proj,
                        view,
                        epos[0],
                        epos[1],
                        epos[2],
                        1.0f);
    
    if (!pLwc->fps_mode && nav_path_query_n_smooth_path(field_nav(pLwc->field))) {
        
        const float* path_query_test_player_pos = field_path_query_test_player_pos(pLwc->field);
        const float skin_scale_f = field_skin_scale(pLwc->field);
        
        mat4x4 skin_trans;
        mat4x4_identity(skin_trans);
        mat4x4_translate(skin_trans, path_query_test_player_pos[0], path_query_test_player_pos[1], path_query_test_player_pos[2]);
        mat4x4 skin_scale;
        mat4x4_identity(skin_scale);
        mat4x4_scale_aniso(skin_scale, skin_scale, skin_scale_f, skin_scale_f, skin_scale_f);
        mat4x4 skin_rot;
        mat4x4_identity(skin_rot);
        mat4x4_rotate_Z(skin_rot, skin_rot, field_path_query_test_player_rot(pLwc->field) + (float)LWDEG2RAD(90));
        
        mat4x4 skin_model;
        mat4x4_identity(skin_model);
        mat4x4_mul(skin_model, skin_rot, skin_model);
        mat4x4_mul(skin_model, skin_scale, skin_model);
        mat4x4_mul(skin_model, skin_trans, skin_model);
        lw_load_tex(pLwc, LAE_3D_PLAYER_TEX_KTX);
        render_skin(pLwc,
                    pLwc->tex_atlas[LAE_3D_PLAYER_TEX_KTX],
                    LSVT_HUMAN,
                    &pLwc->action[LWAC_HUMANACTION_WALKPOLISH],
                    &pLwc->armature[LWAR_HUMANARMATURE],
                    1, 1, 1, 1, field_test_player_flash(pLwc->field), proj, view, skin_model, pLwc->test_player_skin_time * 5, 1);
    }
}

static void s_render_construct_preview_model(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 perspective, float x, float y, float z, float a) {
    if (pLwc->construct.preview_enable) {
        render_tower_yaw(pLwc, perspective, view, x, y, a,
                         pLwc->construct.preview.anim_action_id,
                         FLT_MAX,
                         0,
                         0.25f,
                         pLwc->construct.preview.atlas,
                         pLwc->construct.preview.skin_vbo,
                         pLwc->construct.preview.armature);
    }
}

static void s_render_player_model(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, float x, float y, float z, float a, const LWANIMACTION* action, float skin_time, int loop, float flash) {
    const float skin_scale_f = field_skin_scale(pLwc->field);
    
    mat4x4 skin_trans;
    mat4x4_identity(skin_trans);
    mat4x4_translate(skin_trans, x, y, z);
    mat4x4 skin_scale;
    mat4x4_identity(skin_scale);
    mat4x4_scale_aniso(skin_scale, skin_scale, skin_scale_f, skin_scale_f, skin_scale_f);
    mat4x4 skin_rot;
    mat4x4_identity(skin_rot);
    mat4x4_rotate_Z(skin_rot, skin_rot, a + (float)LWDEG2RAD(90));
    
    mat4x4 skin_model;
    mat4x4_identity(skin_model);
    mat4x4_mul(skin_model, skin_rot, skin_model);
    mat4x4_mul(skin_model, skin_scale, skin_model);
    mat4x4_mul(skin_model, skin_trans, skin_model);
    
    if (pLwc->player_action) {
        lw_load_tex(pLwc, LAE_3D_PLAYER_TEX_KTX);
        render_skin(pLwc,
                    pLwc->tex_atlas[LAE_3D_PLAYER_TEX_KTX],
                    LSVT_HUMAN,
                    action,
                    &pLwc->armature[LWAR_HUMANARMATURE],
                    1, 1, 1, 1, flash, proj, view, skin_model, skin_time, loop);
    }
}

static void s_render_field_sphere(const LWCONTEXT* pLwc, struct _LWFIELD* const field, const mat4x4 view, const mat4x4 proj) {
    // Render player-spawned field spheres
    for (int i = 0; i < MAX_FIELD_SPHERE; i++) {
        float pos[3];
        if (field_sphere_pos(field, i, pos)) {
            const float s_x = 1.5f;
            const float s_y = 4.0f;
            const float s_z = 4.0f;
            mat4x4 rot;
            vec3 xaxis = { 1, 0, 0 };
            vec3 vel, vel_norm;
            field_sphere_vel(field, i, vel);
            if (vec3_len(vel)) {
                vec3_norm(vel_norm, vel);
                rotation_matrix_from_vectors(rot, xaxis, vel_norm);
            } else {
                mat4x4_identity(rot);
            }
            lw_load_tex(pLwc, LAE_BEAM_KTX);
            render_field_object_rot(pLwc,
                                    LVT_BEAM,
                                    pLwc->tex_atlas[LAE_BEAM_KTX],
                                    view,
                                    proj,
                                    pos[0],
                                    pos[1],
                                    pos[2],
                                    s_x,
                                    s_y,
                                    s_z,
                                    1,
                                    0,
                                    rot);
        }
    }
    // Render remote-spawned field spheres
    for (int i = 0; i < MAX_FIELD_REMOTE_SPHERE; i++) {
        float pos[3];
        if (field_remote_sphere_pos(field, i, pos)) {
            const float s_x = 1.5f;
            const float s_y = 4.0f;
            const float s_z = 4.0f;
            mat4x4 rot;
            vec3 xaxis = { 1, 0, 0 };
            vec3 vel, vel_norm;
            field_remote_sphere_vel(field, i, vel);
            if (vec3_len(vel)) {
                vec3_norm(vel_norm, vel);
                rotation_matrix_from_vectors(rot, xaxis, vel_norm);
            } else {
                mat4x4_identity(rot);
            }
            lw_load_tex(pLwc, LAE_BEAM_KTX);
            render_field_object_rot(pLwc,
                                    LVT_BEAM,
                                    pLwc->tex_atlas[LAE_BEAM_KTX],
                                    view,
                                    proj,
                                    pos[0],
                                    pos[1],
                                    pos[2],
                                    s_x,
                                    s_y,
                                    s_z,
                                    1,
                                    0,
                                    rot);
        }
    }
}

static void s_render_field_sphere_shadow(const LWCONTEXT* pLwc, struct _LWFIELD* const field, const mat4x4 view, const mat4x4 proj) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
    glDepthMask(GL_FALSE);
    for (int i = 0; i < MAX_FIELD_SPHERE; i++) {
        float pos[3];
        if (field_sphere_pos(field, i, pos)) {
            lw_load_tex(pLwc, LAE_CIRCLE_SHADOW_KTX);
            render_field_object(pLwc,
                                LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                pLwc->tex_atlas[LAE_CIRCLE_SHADOW_KTX],
                                view,
                                proj,
                                pos[0],
                                pos[1],
                                0.001f,
                                1.0f,
                                1.0f,
                                1.0f,
                                0.75f, // shadow darkness (0 - no shadow, 1 - darkest)
                                0,
                                0);
        }
    }
    for (int i = 0; i < MAX_FIELD_REMOTE_SPHERE; i++) {
        float pos[3];
        if (field_remote_sphere_pos(field, i, pos)) {
            lw_load_tex(pLwc, LAE_CIRCLE_SHADOW_KTX);
            render_field_object(pLwc,
                                LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                pLwc->tex_atlas[LAE_CIRCLE_SHADOW_KTX],
                                view,
                                proj,
                                pos[0],
                                pos[1],
                                0.001f,
                                1.0f,
                                1.0f,
                                1.0f,
                                0.75f, // shadow darkness (0 - no shadow, 1 - darkest)
                                0,
                                0);
        }
    }
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

static void s_render_render_command(const LWCONTEXT* pLwc, mat4x4 view, mat4x4 proj) {
    const double now = lwtimepoint_now_seconds();
    for (int i = 0; i < MAX_RENDER_QUEUE_CAPACITY; i++) {
        const LWFIELDRENDERCOMMAND* cmd = &pLwc->render_command[i];
        if (cmd->key == 0) {
            continue;
        }
        if (cmd->objtype == 1) {
            render_tower_yaw(pLwc, proj, view, cmd->pos[0], cmd->pos[1], cmd->angle,
                             cmd->actionid, (float)rendercommand_animtime(cmd, now), cmd->loop, 1.0f,
                             cmd->atlas, cmd->skin_vbo, cmd->armature);
        } else {
            mat4x4 rot, iden;
            mat4x4_identity(rot);
            mat4x4_identity(iden);
            mat4x4_rotate_Z(rot, iden, cmd->angle);
            lw_load_tex(pLwc, cmd->atlas);
            render_field_object_rot(pLwc,
                                    cmd->vbo,
                                    pLwc->tex_atlas[cmd->atlas],
                                    view,
                                    proj,
                                    cmd->pos[0],
                                    cmd->pos[1],
                                    cmd->pos[2],
                                    cmd->scale[0],
                                    cmd->scale[1],
                                    cmd->scale[2],
                                    1,
                                    0,
                                    rot);
        }
    }
}

static void s_render_render_command_shadow(const LWCONTEXT* pLwc, mat4x4 view, mat4x4 proj) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
    glDepthMask(GL_FALSE);
    for (int i = 0; i < MAX_RENDER_QUEUE_CAPACITY; i++) {
        const LWFIELDRENDERCOMMAND* cmd = &pLwc->render_command[i];
        if (cmd->key == 0) {
            continue;
        }
        if (cmd->objtype == 1) {
            // No shadows for guntowers
        } else {
            lw_load_tex(pLwc, LAE_CIRCLE_SHADOW_KTX);
            render_field_object(pLwc,
                                LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                pLwc->tex_atlas[LAE_CIRCLE_SHADOW_KTX],
                                view,
                                proj,
                                cmd->pos[0],
                                cmd->pos[1],
                                0.001f,
                                0.5f,
                                0.5f,
                                0.5f,
                                0.75f, // shadow darkness (0 - no shadow, 1 - darkest)
                                0,
                                0);
        }
    }
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

static void s_render_field_object_shadow(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
    glDepthMask(GL_FALSE);
    for (int i = 0; i < MAX_FIELD_OBJECT; i++) {
        LWFIELDOBJECT* fo = field_object(pLwc->field, i);
        if (fo->valid) {
            if (fo->skin == 0) {
                lw_load_tex(pLwc, LAE_CIRCLE_SHADOW_KTX);
                render_field_object(pLwc,
                                    LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                    pLwc->tex_atlas[LAE_CIRCLE_SHADOW_KTX],
                                    view,
                                    proj,
                                    fo->x,
                                    fo->y,
                                    0.001f,
                                    fo->sx * (0.25f + fo->z / 0.7f * 0.1f), // search for nav_jump_height
                                    fo->sy * (0.25f + fo->z / 0.7f * 0.1f), // search for nav_jump_height
                                    1.0f,
                                    0.75f, // shadow darkness (0 - no shadow, 1 - darkest)
                                    0,
                                    fo->rot_z);
            }
        }
    }
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

static void s_render_field_object(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj) {
    for (int i = 0; i < MAX_FIELD_OBJECT; i++) {
        LWFIELDOBJECT* fo = field_object(pLwc->field, i);
        if (fo->valid) {
            if (fo->skin == 0) {
                render_field_object(pLwc,
                                    fo->lvt,
                                    fo->tex_id,
                                    view,
                                    proj,
                                    fo->x,
                                    fo->y,
                                    fo->z,
                                    fo->sx,
                                    fo->sy,
                                    1.0f,
                                    fo->alpha_multiplier,
                                    0,
                                    fo->rot_z);
            } else {
                render_guntower(pLwc, proj, view, fo->x, fo->y, LWAC_RECOIL, pLwc->test_player_skin_time, 1);
            }
        }
    }
}

static void s_render_remote_player(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj) {
    mq_lock_mutex(pLwc->mq);
    LWPOSSYNCMSG* value = mq_possync_first(pLwc->mq);
    while (value) {
        const char* cursor = mq_possync_cursor(pLwc->mq);
        // Only render player model if anim_action is not null.
        // Also, exclude the player itself.
        if (value->anim_action && !mq_cursor_player(pLwc->mq, cursor)) {
            s_render_player_model(pLwc, view, proj, value->x, value->y, value->z, value->a, value->anim_action, pLwc->player_skin_time, 1, value->flash);
        }
        value = mq_possync_next(pLwc->mq);
    }
    mq_unlock_mutex(pLwc->mq);
}

static void s_render_player_aim_fan(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, float player_x, float player_y, float player_z) {
    float rscale[FAN_VERTEX_COUNT_PER_ARRAY];
    rscale[0] = 0; // center vertex has no meaningful rscale
    mq_lock_mutex(pLwc->mq);
    for (int i = 1; i < FAN_VERTEX_COUNT_PER_ARRAY; i++) {
        rscale[i] = (float)field_ray_nearest_depth(pLwc->field, LRI_AIM_SECTOR_FIRST_INCLUSIVE + i - 1);
    }
    mq_unlock_mutex(pLwc->mq);
    
    if (pLwc->player_state_data.state == LPS_AIM || pLwc->player_state_data.state == LPS_FIRE) {
        render_fan(pLwc, proj, view,
                   player_x, player_y, player_z,
                   pLwc->player_state_data.rot_z, pLwc->player_state_data.aim_theta, rscale);
    }
}

static void s_render_test_particle(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj) {
    // For gl_PointSize support on vertex shader
#if LW_PLATFORM_WIN32
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
    LWEMITTER2OBJECT* emit_object = ps_emit_object_begin(field_ps(pLwc->field));
    mat4x4 proj_view;
    mat4x4_mul(proj_view, proj, view);
    lwc_enable_additive_blending();
    while (emit_object) {
        mat4x4 model;
        mat4x4_identity(model);
        mat4x4_translate_in_place(model, emit_object->pos[0], emit_object->pos[1], emit_object->pos[2]);
        mat4x4_rotate_Z(model, model, (float)LWDEG2RAD(45));
        ps_render_explosion(pLwc, pLwc->ps_context, emit_object, proj_view, model);
        emit_object = ps_emit_object_next(field_ps(pLwc->field), emit_object);
    }
    lwc_disable_additive_blending();
}

void lwc_render_field(const LWCONTEXT* pLwc) {
    LW_GL_VIEWPORT();
    lw_clear_color();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    int shader_index = LWST_DEFAULT;
    
    lazy_glUseProgram(pLwc, shader_index);
    glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, default_uv_offset);
    glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, default_uv_scale);
    glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, 1);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(pLwc->shader[shader_index].diffuse_location, 0);
    lazy_tex_atlas_glBindTexture(pLwc, pLwc->tex_atlas_index);
    glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0);
    glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)pLwc->proj);
    
    
    
    mat4x4 perspective;
    mat4x4_perspective(perspective, (float)(LWDEG2RAD(49.134) / pLwc->viewport_aspect_ratio), pLwc->viewport_aspect_ratio, 1.0f, 500.0f);
    
    float player_x = 0, player_y = 0, player_z = 0;
    get_field_player_position(pLwc->field, &player_x, &player_y, &player_z);
    
    const float* path_query_test_player_pos = field_path_query_test_player_pos(pLwc->field);
    const float path_query_test_player_rot = field_path_query_test_player_rot(pLwc->field);
    
    mat4x4 view;
    vec3 eye = {
        path_query_test_player_pos[0],
        path_query_test_player_pos[1],
        path_query_test_player_pos[2] + 5
    };
    
    vec3 center = {
        path_query_test_player_pos[0] + cosf(path_query_test_player_rot),
        path_query_test_player_pos[1] + sinf(path_query_test_player_rot),
        path_query_test_player_pos[2] + 5
    };
    
    if (!pLwc->fps_mode) {
        if (field_follow_cam(pLwc->field)) {
            const float cam_dist = 30;
            
            eye[0] = player_x;
            eye[1] = player_y - cam_dist;
            eye[2] = player_z + cam_dist;
            
            center[0] = player_x;
            center[1] = player_y;
            center[2] = player_z;
        } else {
            const float cam_dist = 230;
            
            eye[0] = 270 + player_x;
            eye[1] = player_y - cam_dist + 200;
            eye[2] = cam_dist - 100;
            
            center[0] = player_x;
            center[1] = player_y;
            center[2] = player_z + 60;
        }
    }
    
    vec3 up = { 0, 0, 1 };
    mat4x4_look_at(view, eye, center, up);
    
    //render_ground(pLwc, view, perspective);
    
    if (!pLwc->hide_field) {
        int field_mesh_count = field_field_mesh_count(pLwc->field);
        for (int i = 0; i < field_mesh_count; i++) {
            render_field_object(pLwc,
                                field_field_vbo(pLwc->field, i),
                                field_field_tex_id(pLwc->field, i),
                                view,
                                perspective,
                                0,
                                0,
                                0,
                                1,
                                1,
                                1,
                                1,
                                field_field_tex_mip(pLwc->field, i),
                                0);
        }
    }
    // Render field object shadows
    s_render_field_object_shadow(pLwc, view, perspective);
    // Render bullet shadows
    s_render_field_sphere_shadow(pLwc, pLwc->field, view, perspective);
    // Render objects driven by render commands
    s_render_render_command_shadow(pLwc, view, perspective);
    // Render field objects
    s_render_field_object(pLwc, view, perspective);
    // Render player
    s_render_player_model(pLwc, view, perspective, player_x, player_y, player_z, pLwc->player_state_data.rot_z, pLwc->player_action, pLwc->player_state_data.skin_time, pLwc->player_action_loop, field_player_flash(pLwc->field));
    // Render player construct preview model
    s_render_construct_preview_model(pLwc, view, perspective, player_x, player_y, player_z, pLwc->player_state_data.rot_z);
    // Render remote players
    s_render_remote_player(pLwc, view, perspective);
    // Render path query test player
    s_render_path_query_test_player(pLwc, view, perspective);
    // Render objects driven by render commands
    s_render_render_command(pLwc, view, perspective);
    // Render bullets
    s_render_field_sphere(pLwc, pLwc->field, view, perspective);
    // Render player aim fan
    s_render_player_aim_fan(pLwc, view, perspective, player_x, player_y, player_z);
    // Render test particle emitters
    s_render_test_particle(pLwc, view, perspective);
    // Render UI
    s_render_ui(pLwc);
    // Give up const-ness...
    ((LWCONTEXT*)pLwc)->render_count++;
}
