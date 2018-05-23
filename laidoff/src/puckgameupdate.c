#include "platform_detection.h"
#if LW_PLATFORM_WIN32
#include <WinSock2.h>
#endif
#include "puckgame.h"
#include "lwmacro.h"
#include "lwlog.h"
#include "lwcontext.h"
#include "input.h"
#include "lwudp.h"
#include "puckgamepacket.h"
#include "lwtcp.h"
#include "puckgameupdate.h"
#include "lwtcpclient.h"
#include "lwmath.h"
#include "sound.h"
#include "script.h"
#include "render_puckgame.h"
#include "file.h"

void puck_game_update_world_roll(LWPUCKGAME* puck_game) {
    if (puck_game->world_roll_dirty) {
        const float world_roll_diff = puck_game->world_roll_target - puck_game->world_roll;
        if (fabsf(world_roll_diff) < LWEPSILON) {
            puck_game->world_roll = puck_game->world_roll_target;
            puck_game->world_roll_dirty = 0;
            LOGI("World roll transition finished");
        } else {
            const float world_roll_diff_delta = world_roll_diff * puck_game->world_roll_target_follow_ratio;
            if (fabsf(world_roll_diff_delta) < LWEPSILON/5) {
                puck_game->world_roll = puck_game->world_roll_target;
                puck_game->world_roll_dirty = 0;
                LOGI("World roll transition finished");
            } else {
                puck_game->world_roll += world_roll_diff_delta;
            }
        }
    }
    puck_game->battle_ui_alpha = LWMAX(0.0f, 1.0f - puck_game->world_roll);
    puck_game->main_menu_ui_alpha = LWMAX(0.0f, 1.0f - fabsf((float)LWDEG2RAD(180) - puck_game->world_roll));
    LWCONTEXT* pLwc = (LWCONTEXT*)puck_game->pLwc;
    if (pLwc->game_scene == LGS_PUCK_GAME) {
        vec2 world_right_top_end_ui_point;
        calculate_world_right_top_end_ui_point(pLwc, puck_game, world_right_top_end_ui_point);
        const float width_ratio_of_world = fabsf(world_right_top_end_ui_point[0]) / pLwc->aspect_ratio;
        pLwc->viewport_x = (int)(puck_game->main_menu_ui_alpha * pLwc->width * width_ratio_of_world / 2);
        pLwc->viewport_y = 0;
    } else {
        pLwc->viewport_x = 0;
        pLwc->viewport_y = 0;
    }
}

void update_boundary_impact(LWPUCKGAME* puck_game, float delta_time) {
    for (int i = 0; i < LPGB_COUNT; i++) {
        puck_game->boundary_impact[i] -= puck_game->boundary_impact_falloff_speed * delta_time;
        if (puck_game->boundary_impact[i] < 0) {
            puck_game->boundary_impact[i] = 0;
            puck_game->boundary_impact_player_no[i] = 0;
        }
    }
}

void update_shake(LWPUCKGAME* puck_game, float delta_time) {
    for (int i = 0; i < 2; i++) {
        // Decrease shake remain time
        if (puck_game->remote_dash[i][0].shake_remain_time > 0) {
            puck_game->remote_dash[i][0].shake_remain_time = LWMAX(0, puck_game->remote_dash[i][0].shake_remain_time - (float)delta_time);
        }
        // Jump
        if (puck_game->remote_jump[i][0].shake_remain_time > 0) {
            puck_game->remote_jump[i][0].shake_remain_time = LWMAX(0, puck_game->remote_jump[i][0].shake_remain_time - (float)delta_time);
        }
    }
    // Decrease HP remain time (player)
    if (puck_game->pg_player[0].hp_shake_remain_time > 0) {
        puck_game->pg_player[0].hp_shake_remain_time = LWMAX(0, puck_game->pg_player[0].hp_shake_remain_time - (float)delta_time);
    }
    // Decrease HP remain time (target)
    if (puck_game->pg_target[0].hp_shake_remain_time > 0) {
        puck_game->pg_target[0].hp_shake_remain_time = LWMAX(0, puck_game->pg_target[0].hp_shake_remain_time - (float)delta_time);
    }
    // Tower shake
    for (int i = 0; i < LW_PUCK_GAME_TOWER_COUNT; i++) {
        if (puck_game->tower[i].shake_remain_time > 0) {
            puck_game->tower[i].shake_remain_time = LWMAX(0, puck_game->tower[i].shake_remain_time - (float)delta_time);
        }
    }
}

static void puck_game_on_puck_wall_collision(LWPUCKGAME* puck_game, float vdlen, float depth) {
    // play hit sound only if sufficient velocity difference
    LOGIx("puck_game_on_puck_wall_collision vdlen=%f, depth=%f", vdlen, depth);
    if (vdlen > 0.5f && depth > LWEPSILON) {
        play_sound(LWS_COLLISION);
    }
}

static void puck_game_on_puck_tower_collision(LWPUCKGAME* puck_game, float vdlen, float depth) {
    // play hit sound only if sufficient velocity difference
    LOGIx("puck_game_on_puck_wall_collision vdlen=%f, depth=%f", vdlen, depth);
    if (vdlen > 0.5f && depth > LWEPSILON) {
        play_sound(LWS_COLLISION);
    }
}

static void puck_game_on_puck_player_collision(LWPUCKGAME* puck_game, float vdlen, float depth) {
    // play hit sound only if sufficient velocity difference
    LOGIx("puck_game_on_puck_wall_collision vdlen=%f, depth=%f", vdlen, depth);
    if (vdlen > 0.5f && depth > LWEPSILON) {
        play_sound(LWS_COLLISION);
    }
    LWCONTEXT* pLwc = (LWCONTEXT*)puck_game->pLwc;
    script_on_near_puck_player(pLwc->script, puck_game_dashing(&puck_game->remote_dash[LW_PUCK_GAME_PLAYER_TEAM][0]));
}

static void puck_game_on_player_dash(LWPUCKGAME* puck_game) {
    play_sound(LWS_DASH2);
}

static void puck_game_on_ready(LWPUCKGAME* puck_game) {
    play_sound(LWS_READY);
}

static void puck_game_on_steady(LWPUCKGAME* puck_game) {
    play_sound(LWS_STEADY);
}

static void puck_game_on_go(LWPUCKGAME* puck_game) {
    play_sound(LWS_GO);
}

static void puck_game_on_finished(LWPUCKGAME* puck_game, int winner) {
    if (puck_game->player_no == winner) {
        play_sound(LWS_VICTORY);
    } else {
        play_sound(LWS_DEFEAT);
    }
    play_sound(LWS_COLLAPSE);
}

static void update_tower(LWPUCKGAME* puck_game, float delta_time) {
    for (int i = 0; i < LW_PUCK_GAME_TOWER_COUNT; i++) {
        if (puck_game->tower[i].collapsing) {
            puck_game->tower[i].collapsing_time += delta_time;
        }
    }
}

static void send_udp_frame_sync(LWCONTEXT* pLwc, LWPUCKGAME* puck_game, int remote) {
    // syncing at 60 fps
    float app_time = (float)pLwc->app_time;
    if (app_time < puck_game->last_udp_frame_sync_sent + 1/60.0f) {
        return;
    }
    puck_game->last_udp_frame_sync_sent = app_time;
    
    float dx, dy, dlen;
    int dir_pad_dragging = lw_get_normalized_dir_pad_input(pLwc, &pLwc->left_dir_pad, &dx, &dy, &dlen);
    const float dlen_max = pLwc->left_dir_pad.max_follow_distance;
    if (dlen > dlen_max) {
        dlen = dlen_max;
    }
    float dlen_ratio = dlen / dlen_max;
    // make dlen_ratio ^ 4
    dlen_ratio *= dlen_ratio;
    dlen_ratio *= dlen_ratio;
    puck_game->remote_control[LW_PUCK_GAME_PLAYER_TEAM][0].dir_pad_dragging = pLwc->left_dir_pad.dragging;
    puck_game->remote_control[LW_PUCK_GAME_PLAYER_TEAM][0].dx = dx;
    puck_game->remote_control[LW_PUCK_GAME_PLAYER_TEAM][0].dy = dy;
    puck_game->remote_control[LW_PUCK_GAME_PLAYER_TEAM][0].dlen = dlen_ratio;

    const int send_udp = remote && puck_game_state_phase_finished(pLwc->puck_game_state.bf.phase) == 0;
    
    if (dir_pad_dragging) {
        LOGIx("dx=%.2f, dy=%.2f, dlen=%.2f, dlen_max=%.2f, dlen_ratio=%.2f", dx, dy, dlen, dlen_max, dlen_ratio);
        if (send_udp) {
            LWPMOVE packet_move;
            packet_move.type = LPGP_LWPMOVE;
            packet_move.battle_id = pLwc->puck_game->battle_id;
            packet_move.token = pLwc->puck_game->token;
            packet_move.dx = pLwc->puck_game->player_no == 2 ? -dx : dx;
            packet_move.dy = pLwc->puck_game->player_no == 2 ? -dy : dy;
            packet_move.dlen = dlen_ratio;
            udp_send(pLwc->udp, (const char*)&packet_move, sizeof(packet_move));
        }
    } else {
        if (send_udp) {
            LWPSTOP packet_stop;
            packet_stop.type = LPGP_LWPSTOP;
            packet_stop.battle_id = pLwc->puck_game->battle_id;
            packet_stop.token = pLwc->puck_game->token;
            udp_send(pLwc->udp, (const char*)&packet_stop, sizeof(packet_stop));
        }
    }
    
    // Pull
    if (puck_game->control_flags & LPGCF_HIDE_PULL_BUTTON) {
        if (puck_game->remote_control[LW_PUCK_GAME_PLAYER_TEAM][0].pull_puck) {
            if (send_udp) {
                LWPPULLSTART p;
                p.type = LPGP_LWPPULLSTART;
                p.battle_id = pLwc->puck_game->battle_id;
                p.token = pLwc->puck_game->token;
                udp_send(pLwc->udp, (const char*)&p, sizeof(p));
            }
        } else {
            if (send_udp) {
                LWPPULLSTOP p;
                p.type = LPGP_LWPPULLSTOP;
                p.battle_id = pLwc->puck_game->battle_id;
                p.token = pLwc->puck_game->token;
                udp_send(pLwc->udp, (const char*)&p, sizeof(p));
            }
        }
    }
}

void update_puck_game(LWCONTEXT* pLwc, LWPUCKGAME* puck_game, double delta_time) {
    if (!puck_game || !puck_game->world) {
        return;
    }
    const int remote = puck_game_remote(pLwc, puck_game);
    puck_game->on_player_damaged = remote ? 0 : puck_game_player_tower_decrease_hp_test;
    puck_game->on_target_damaged = remote ? 0 : puck_game_target_tower_decrease_hp_test;
    puck_game->on_puck_wall_collision = puck_game_on_puck_wall_collision;
    puck_game->on_puck_tower_collision = puck_game_on_puck_tower_collision;
    puck_game->on_puck_player_collision = puck_game_on_puck_player_collision;
    puck_game->on_player_dash = puck_game_on_player_dash;
    puck_game->on_ready = puck_game_on_ready;
    puck_game->on_steady = puck_game_on_steady;
    puck_game->on_go = puck_game_on_go;
    puck_game->on_finished = puck_game_on_finished;
    // tick physics engine only if practice mode (single play mode)
    if (puck_game->game_state == LPGS_PRACTICE || puck_game->game_state == LPGS_TUTORIAL) {
        // update puck game time is done in the function below
        puck_game_update_tick(puck_game, pLwc->update_frequency);
    } else {
        // update puck game time according to update tick which is synced with server
        puck_game->time = puck_game_elapsed_time(puck_game->update_tick, pLwc->update_frequency);
    }
    // set boundary impact according to wall hit bits
    // (no branches by map)
    if (puck_game->game_map == LPGM_SQUARE) {
        for (int i = 0; i <= LPGB_N - LPGB_E; i++) {
            if ((puck_game->wall_hit_bit >> i) & 1) {
                int boundary = LPGB_E + i;
                puck_game->boundary_impact[boundary] = puck_game->boundary_impact_start;
                puck_game->boundary_impact_player_no[boundary] = puck_game->puck_owner_player_no;
                if (remote) {
                    play_sound(LWS_COLLISION);
                }
            }
        }
    } else {
        for (int i = 0; i <= LPGB_SE - LPGB_EE; i++) {
            if ((puck_game->wall_hit_bit >> i) & 1) {
                int boundary = LPGB_EE + i;
                puck_game->boundary_impact[boundary] = puck_game->boundary_impact_start;
                puck_game->boundary_impact_player_no[boundary] = puck_game->puck_owner_player_no;
                if (remote) {
                    play_sound(LWS_COLLISION);
                }
            }
        }
    }
    
    // clear wall hit bit here only on online mode
    if (puck_game->game_state == LPGS_BATTLE) {
        puck_game->wall_hit_bit = 0;
    }
    // change control UI alpha according to the battle phase
    if (puck_game->battle_phase == LSP_READY) {
        puck_game->battle_control_ui_alpha = 0.0f;
    } else if (puck_game->battle_phase == LSP_STEADY) {
        puck_game->battle_control_ui_alpha = 0.2f;
    } else if (puck_game->battle_phase == LSP_GO) {
        puck_game->battle_control_ui_alpha = 1.0f;
    }
    
    // send outgoing frame-sync udp packets
    send_udp_frame_sync(pLwc, puck_game, remote);

    // control bogus (AI player)
    if (puck_game->bogus_disabled == 0) {
        if (puck_game->game_state == LPGS_TUTORIAL) {
            LWPUCKGAMEBOGUSPARAM bogus_param = {
                0.0075f, // target_follow_agility
                0.30f, // dash_detect_radius
                0.2f, // dash_frequency
                0.8f, // dash_cooltime_lag_min
                1.2f, // dash_cooltime_lag_max
            };
            puck_game_control_bogus(puck_game, &bogus_param);
        } else {
            LWPUCKGAMEBOGUSPARAM bogus_param = {
                0.0075f, // target_follow_agility
                0.75f, // dash_detect_radius
                0.5f, // dash_frequency
                0.4f, // dash_cooltime_lag_min
                0.6f, // dash_cooltime_lag_max
            };
            if (puck_game_state_phase_battling(puck_game->battle_phase)) {
                puck_game_control_bogus(puck_game, &bogus_param);
            }
        }
    }
    for (int i = 0; i < 2; i++) {
        puck_game_update_remote_player(puck_game, (float)delta_time, i);
    }

    // -------- Client only --------

    update_shake(puck_game, (float)delta_time);
    update_puck_ownership(puck_game);
    update_puck_reflect_size(puck_game, (float)delta_time);
    // update_world_roll() call makes race condition with rendering thread... move to rendering thread.
    //update_world_roll(puck_game);
    update_boundary_impact(puck_game, (float)delta_time);
    update_tower(puck_game, (float)delta_time);
    puck_game_update_battle_result_popup(pLwc, puck_game, puck_game->battle_phase, puck_game->player_no);
}

void puck_game_jump(LWCONTEXT* pLwc, LWPUCKGAME* puck_game) {
    // Check params
    if (!pLwc || !puck_game) {
        return;
    }
    // Check already effective jump
    if (puck_game_jumping(&puck_game->remote_jump[LW_PUCK_GAME_PLAYER_TEAM][0])) {
        return;
    }
    // Check effective move input
    //float dx, dy, dlen;
    /*if (!lw_get_normalized_dir_pad_input(pLwc, &dx, &dy, &dlen)) {
    return;
    }*/

    // Check cooltime
    if (puck_game_jump_cooltime(puck_game) < puck_game->jump_interval) {
        puck_game->remote_jump[LW_PUCK_GAME_PLAYER_TEAM][0].shake_remain_time = puck_game->jump_shake_time;
        return;
    }

    // Start jump!
    puck_game_commit_jump(puck_game, &puck_game->remote_jump[LW_PUCK_GAME_PLAYER_TEAM][0], 1);

    const int remote = puck_game_remote(pLwc, puck_game);

    if (puck_game_state_phase_finished(pLwc->puck_game_state.bf.phase) == 0
        && remote) {
        LWPJUMP packet_jump;
        packet_jump.type = LPGP_LWPJUMP;
        packet_jump.battle_id = pLwc->puck_game->battle_id;
        packet_jump.token = pLwc->puck_game->token;
        udp_send(pLwc->udp, (const char*)&packet_jump, sizeof(packet_jump));
    }
}

void puck_game_dash_and_send(LWCONTEXT* pLwc, LWPUCKGAME* puck_game) {
    LWPUCKGAMEDASH* dash = &puck_game->remote_dash[puck_game->player_no == 2 ? 1 : 0][0];
    if (puck_game_dash(puck_game, dash, puck_game->player_no == 2 ? 2 : 1) == 0) {
        const int remote = puck_game_remote(pLwc, puck_game);
        const int send_udp = remote && puck_game_state_phase_finished(pLwc->puck_game_state.bf.phase) == 0;
        if (send_udp) {
            LWPDASH packet_dash;
            packet_dash.type = LPGP_LWPDASH;
            packet_dash.battle_id = pLwc->puck_game->battle_id;
            packet_dash.token = pLwc->puck_game->token;
            udp_send(pLwc->udp, (const char*)&packet_dash, sizeof(packet_dash));
        }
    }
}

void puck_game_target_move(LWPUCKGAME* puck_game, float dx, float dy) {
    dJointID tcj = puck_game->target_control_joint[0];
    float player_speed = 0.5f;
    dJointSetLMotorParam(tcj, dParamVel1, player_speed * dx);
    dJointSetLMotorParam(tcj, dParamVel2, player_speed * dy);
    puck_game->last_remote_dx = dx;
    puck_game->last_remote_dy = dy;
}

void puck_game_target_stop(LWPUCKGAME* puck_game) {
    dJointID tcj = puck_game->target_control_joint[0];
    dJointSetLMotorParam(tcj, dParamVel1, 0);
    dJointSetLMotorParam(tcj, dParamVel2, 0);
}

void puck_game_target_dash(LWPUCKGAME* puck_game, int player_no) {
    puck_game_commit_dash(puck_game,
                          &puck_game->remote_dash[player_no - 1][0],
                          puck_game->last_remote_dx,
                          puck_game->last_remote_dy,
                          player_no);
}

void puck_game_pull_puck_start(LWCONTEXT* pLwc, LWPUCKGAME* puck_game) {
    puck_game->remote_control[LW_PUCK_GAME_PLAYER_TEAM][0].pull_puck = 1;
}

void puck_game_pull_puck_stop(LWCONTEXT* pLwc, LWPUCKGAME* puck_game) {
    puck_game->remote_control[LW_PUCK_GAME_PLAYER_TEAM][0].pull_puck = 0;
}

void puck_game_pull_puck_toggle(LWCONTEXT* pLwc, LWPUCKGAME* puck_game) {
    if (puck_game->remote_control[LW_PUCK_GAME_PLAYER_TEAM][0].pull_puck) {
        puck_game_pull_puck_stop(pLwc, puck_game);
    } else {
        puck_game_pull_puck_start(pLwc, puck_game);
    }
}

void puck_game_clear_match_data(LWCONTEXT* pLwc, LWPUCKGAME* puck_game) {
    puck_game->battle_id = 0;
    puck_game->token = 0;
    puck_game->player_no = 1;
    puck_game->battle_control_ui_alpha = 0;
    memset(&pLwc->puck_game_state, 0, sizeof(pLwc->puck_game_state));
}

void puck_game_reset_view_proj(LWCONTEXT* pLwc, LWPUCKGAME* puck_game) {
    // Setup puck game view, proj matrices
    
    mat4x4_perspective(pLwc->puck_game_proj, (float)(LWDEG2RAD(49.1343) / pLwc->aspect_ratio), pLwc->aspect_ratio, 1.0f, 500.0f);
    vec3 eye = { 0.0f, 0.0f, 10.0f /*12.0f*/ };
    vec3 center = { 0, 0, 0 };
    vec3 up = { 0, 1, 0 };
    if (puck_game->player_no == 2) {
        up[1] = -1.0f;
    }
    mat4x4_look_at(pLwc->puck_game_view, eye, center, up);
}


// half_height: 3.0f
// near: 0.1f, far: 100.0f
// eye: 0, -8, 14
// center: 0, 0.3, 0
void puck_game_reset_view_proj_ortho(LWCONTEXT* pLwc,
                                     LWPUCKGAME* puck_game,
                                     float half_height,
                                     float near_z,
                                     float far_z,
                                     float eye_x,
                                     float eye_y,
                                     float eye_z,
                                     float center_x,
                                     float center_y,
                                     float center_z) {
    mat4x4_ortho(pLwc->puck_game_proj,
                 -half_height * pLwc->aspect_ratio,
                 +half_height * pLwc->aspect_ratio,
                 -half_height,
                 +half_height,
                 near_z,
                 far_z);
    pLwc->eye[0] = eye_x;
    pLwc->eye[1] = eye_y;
    pLwc->eye[2] = eye_z;
    pLwc->center[0] = center_x;
    pLwc->center[1] = center_y;
    pLwc->center[2] = center_z;
    pLwc->up[0] = 0;
    pLwc->up[1] = 1;
    pLwc->up[2] = 0;
    mat4x4_look_at(pLwc->puck_game_view, pLwc->eye, pLwc->center, pLwc->up);
}

void puck_game_fire(LWCONTEXT* pLwc, LWPUCKGAME* puck_game, float puck_fire_dx, float puck_fire_dy, float puck_fire_dlen) {
    puck_game_commit_fire(puck_game, &puck_game->remote_fire[LW_PUCK_GAME_PLAYER_TEAM][0], 1, puck_fire_dx, puck_fire_dy, puck_fire_dlen);
    LWPFIRE packet_fire;
    packet_fire.type = LPGP_LWPFIRE;
    packet_fire.battle_id = pLwc->puck_game->battle_id;
    packet_fire.token = pLwc->puck_game->token;
    packet_fire.dx = puck_fire_dx;
    packet_fire.dy = puck_fire_dy;
    packet_fire.dlen = puck_fire_dlen;
    udp_send(pLwc->udp, (const char*)&packet_fire, sizeof(packet_fire));
}

void puck_game_shake_player(LWPUCKGAME* puck_game, LWPUCKGAMEPLAYER* player) {
    player->hp_shake_remain_time = puck_game->hp_shake_time;
}

void puck_game_spawn_tower_damage_text(LWCONTEXT* pLwc, LWPUCKGAME* puck_game, LWPUCKGAMETOWER* tower, int damage) {
    tower->shake_remain_time = puck_game->tower_shake_time;
    mat4x4 proj_view;
    mat4x4_identity(proj_view);
    mat4x4_mul(proj_view, pLwc->puck_game_proj, pLwc->puck_game_view);
    vec2 ui_point;
    vec4 tower_world_point;
    puck_game_tower_pos(tower_world_point, puck_game, tower->owner_player_no);
    calculate_ui_point_from_world_point(pLwc->aspect_ratio, proj_view, tower_world_point, ui_point);
    char damage_str[16];
    sprintf(damage_str, "%d", damage);
    spawn_damage_text(pLwc, ui_point[0], ui_point[1], 0, damage_str, LDTC_UI);
    play_sound(LWS_DAMAGE);
    const int remote = puck_game_remote(pLwc, puck_game);
    if (remote) {
        play_sound(LWS_COLLISION);
    }
}

void puck_game_tower_damage_test(LWCONTEXT* pLwc, LWPUCKGAME* puck_game, LWPUCKGAMEPLAYER* player, LWPUCKGAMETOWER* tower, int damage) {
    puck_game_shake_player(pLwc->puck_game, player);
    puck_game_spawn_tower_damage_text(pLwc, pLwc->puck_game, tower, damage);
}

void puck_game_player_tower_decrease_hp_test(LWPUCKGAME* puck_game) {
    puck_game_tower_damage_test(puck_game->pLwc, puck_game, &puck_game->pg_player[0], &puck_game->tower[0], 1);
    LWCONTEXT* pLwc = (LWCONTEXT*)puck_game->pLwc;
    script_on_target_attack(pLwc->script);
}

void puck_game_target_tower_decrease_hp_test(LWPUCKGAME* puck_game) {
    puck_game_tower_damage_test(puck_game->pLwc, puck_game, &puck_game->pg_target[0], &puck_game->tower[1], 1);
    LWCONTEXT* pLwc = (LWCONTEXT*)puck_game->pLwc;
    script_on_player_attack(pLwc->script);
}

int puck_game_remote(const LWCONTEXT* pLwc, const LWPUCKGAME* puck_game) {
    return puck_game->battle_id != 0;
}

int puck_game_is_tutorial_completed(const LWPUCKGAME* puck_game) {
    LWCONTEXT* pLwc = (LWCONTEXT*)puck_game->pLwc;
    return is_file_exist(pLwc->user_data_path, "tutorial-finished");
}

int puck_game_is_tutorial_stoppable(const LWPUCKGAME* puck_game) {
    return puck_game_is_tutorial_completed(puck_game);
}

void puck_game_roll_world(LWPUCKGAME* puck_game, int dir, int axis, float target) {
    if (puck_game->world_roll_dirty == 0) {
        LOGI("World roll began...");
        puck_game->world_roll_dir = dir;
        puck_game->world_roll_axis = axis;
        puck_game->world_roll_target = target;
        puck_game->world_roll_dirty = 1;
        play_sound(LWS_SWOOSH);
    }
}

void puck_game_roll_to_battle(LWPUCKGAME* puck_game) {
    if (puck_game->world_roll_dirty == 0) {
        puck_game->game_state = LPGS_BATTLE;
        LOGI("World roll to battle began...");
        puck_game->world_roll_target = 0;
        puck_game->world_roll_dirty = 1;
        play_sound(LWS_SWOOSH);
    }
}

void puck_game_roll_to_practice(LWPUCKGAME* puck_game) {
    if (puck_game->world_roll_dirty == 0) {
        puck_game->game_state = LPGS_PRACTICE;
        LOGI("World roll to practice began...");
        puck_game->world_roll_target = 0;
        puck_game->world_roll_dirty = 1;
        play_sound(LWS_SWOOSH);
    }
}

void puck_game_roll_to_tutorial(LWPUCKGAME* puck_game) {
    if (puck_game->world_roll_dirty == 0) {
        puck_game->game_state = LPGS_TUTORIAL;
        LOGI("World roll to tutorial began...");
        puck_game->world_roll_target = 0;
        puck_game->world_roll_dirty = 1;
        play_sound(LWS_SWOOSH);
    }
}

void puck_game_roll_to_main_menu(LWPUCKGAME* puck_game) {
    if (puck_game->world_roll_dirty == 0) {
        puck_game->game_state = LPGS_MAIN_MENU;
        puck_game_roll_world(puck_game, 1, 1, (float)LWDEG2RAD(180));
    }
}

void puck_game_set_static_default_values_client(LWPUCKGAME* puck_game) {
    puck_game->puck_lae = LAE_PUCK_GRAY_KTX;
    puck_game->player_lae = LAE_PUCK_PLAYER_KTX;
    puck_game->target_lae = LAE_PUCK_ENEMY_KTX;
    puck_game->puck_lvt = LVT_PUCK;
    puck_game->player_lvt = LVT_PUCK_PLAYER;
}

void puck_game_follow_cam(LWCONTEXT* pLwc, LWPUCKGAME* puck_game) {
    if (puck_game->follow_cam) {
        const float player_x = puck_game->go[LPGO_PLAYER].pos[0];
        const float cam_x = pLwc->eye[0] + pLwc->eye_x_offset;
        //if (fabsf(player_x - cam_x) > 1.0f) {
            pLwc->eye_x_offset += (player_x - cam_x) * 0.05f;
        //}
        vec3 eye = { pLwc->eye[0] + pLwc->eye_x_offset, pLwc->eye[1], pLwc->eye[2] };
        vec3 center = { pLwc->center[0] + pLwc->eye_x_offset, pLwc->center[1], pLwc->center[2] };
        mat4x4_look_at(pLwc->puck_game_view, eye, center, pLwc->up);
    }
}

void puck_game_update_battle_result_popup(LWCONTEXT* pLwc, LWPUCKGAME* puck_game, LWP_STATE_PHASE battle_phase, int player_no) {
    int new_score = 0;
    switch (battle_phase) {
        case LSP_INIT:
        break;
        case LSP_READY:
        case LSP_STEADY:
        case LSP_GO:
        return;
        case LSP_FINISHED_DRAW: // DRAW == TIMEOUT
        new_score = puck_game->matched2.draw_score;
        break;
        case LSP_FINISHED_VICTORY_P1:
        new_score = player_no == 2 ? puck_game->matched2.defeat_score : puck_game->matched2.victory_score;
        break;
        case LSP_FINISHED_VICTORY_P2:
        new_score = player_no == 2 ? puck_game->matched2.victory_score : puck_game->matched2.defeat_score;
        break;
        case LSP_TUTORIAL:
        break;
        case LSP_MAX_BIT:
        break;
    }
    // show score diff message
    int score_diff = new_score - puck_game->player_score[0];
    if (score_diff
        && puck_game->game_state == LPGS_BATTLE
        && puck_game_state_phase_finished(battle_phase)) {
        char points_acquired_lost[64];
        char points_singular_plural[64];
        if (score_diff > 0) {
            script_get_string(pLwc->L, "STR_POINTS_ACQUIRED", points_acquired_lost, sizeof(points_acquired_lost));
            script_get_string(pLwc->L, score_diff == 1 ? "STR_POINTS_SINGULAR" : "STR_POINTS_PLURAL", points_singular_plural, sizeof(points_singular_plural));
            sprintf(puck_game->score_message, points_acquired_lost, score_diff, points_singular_plural);
        } else if (score_diff < 0) {
            script_get_string(pLwc->L, "STR_POINTS_LOST", points_acquired_lost, sizeof(points_acquired_lost));
            script_get_string(pLwc->L, -score_diff == 1 ? "STR_POINTS_SINGULAR" : "STR_POINTS_PLURAL", points_singular_plural, sizeof(points_singular_plural));
            sprintf(puck_game->score_message, points_acquired_lost, -score_diff, points_singular_plural);
        }
    } else {
        puck_game->score_message[0] = '\0';
    }
}
