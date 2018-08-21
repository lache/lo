#include "puckgame.h"
#include "lwmacro.h"
#include "lwlog.h"
#include "lwtimepoint.h"
#include "numcomp.h"
#include <assert.h>
#include "file.h"
#include "sound.h"
#include "puckgamerecord.h"
#include "lz4.h"

static void call_collision_callback(LWPUCKGAME* puck_game,
                                    const dContact* contact,
                                    void(*on_collision)(LWPUCKGAME*, float, float)) {
    dVector3 zero = { 0,0,0 };
    const dReal* v1 = dGeomGetBody(contact->geom.g1) ? dBodyGetLinearVel(dGeomGetBody(contact->geom.g1)) : zero;
    const dReal* v2 = dGeomGetBody(contact->geom.g2) ? dBodyGetLinearVel(dGeomGetBody(contact->geom.g2)) : zero;
    dVector3 vd;
    dSubtractVectors3(vd, v1, v2);
    const float vdlen = (float)dLENGTH(vd);
    const float depth = (float)contact->geom.depth;

    if (vdlen > 0.5f && depth > LWEPSILON && puck_game->puck_owner_player_no > 0) {
        int idx = puck_game->puck_owner_player_no - 1;
        assert(idx == 0 || idx == 1);
        puck_game->battle_stat[idx].PuckWallHit++;
    }

    if (on_collision) {
        on_collision(puck_game, vdlen, depth);
    }
}

static void testgo_move_callback(dBodyID b) {
    LWPUCKGAMEOBJECT* go = (LWPUCKGAMEOBJECT*)dBodyGetData(b);

    // Position
    const dReal* p = dBodyGetPosition(b);
    go->pos[0] = (float)p[0];
    go->pos[1] = (float)p[1];
    go->pos[2] = (float)p[2];
    // Orientation
    const dReal* r = dBodyGetRotation(b); // dMatrix3
    mat4x4 rot;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            rot[i][j] = (float)r[4 * i + j];
        }
    }
    rot[3][0] = 0;
    rot[3][1] = 0;
    rot[3][2] = 0;
    rot[3][3] = 1;

    mat4x4_transpose(go->rot, rot);
    const dReal* vel = dBodyGetLinearVel(go->body);
    go->speed = (float)dLENGTH(vel);
    if (vel[0]) {
        go->move_rad = atan2f((float)vel[1], (float)vel[0]) + (float)M_PI / 2;
    }
}

static void create_go(LWPUCKGAME* puck_game, LW_PUCK_GAME_OBJECT lpgo, float mass, float radius) {
    LWPUCKGAMEOBJECT* go = &puck_game->go[lpgo];
    go->puck_game = puck_game;
    go->radius = radius;
    assert(go->geom == 0);
    go->geom = dCreateSphere(puck_game->space, radius);
    assert(go->body == 0);
    go->body = dBodyCreate(puck_game->world);
    dMass m;
    dMassSetSphereTotal(&m, mass, radius);
    dBodySetMass(go->body, &m);
    dBodySetData(go->body, go);
    dBodySetMovedCallback(go->body, testgo_move_callback);
    dGeomSetBody(go->geom, go->body);
    go->capsule = 0;
}

static void create_capsule_go(LWPUCKGAME* puck_game, LW_PUCK_GAME_OBJECT lpgo, float mass, float radius, float length) {
    LWPUCKGAMEOBJECT* go = &puck_game->go[lpgo];
    go->puck_game = puck_game;
    go->radius = radius;
    go->length = length;
    assert(go->geom == 0);
    go->geom = dCreateCapsule(puck_game->space, radius, length);
    assert(go->body == 0);
    go->body = dBodyCreate(puck_game->world);
    dMass m;
    dMassSetCapsuleTotal(&m, mass, 2/*z-axis*/, radius, length);
    dBodySetMass(go->body, &m);
    dBodySetData(go->body, go);
    dBodySetMovedCallback(go->body, testgo_move_callback);
    dGeomSetBody(go->geom, go->body);
    go->capsule = 1;
}

static void destroy_go(LWPUCKGAME* puck_game, LW_PUCK_GAME_OBJECT lpgo) {
    LWPUCKGAMEOBJECT* go = &puck_game->go[lpgo];
    assert(go->geom);
    dGeomDestroy(go->geom);
    go->geom = 0;
    assert(go->body);
    dBodyDestroy(go->body);
    go->body = 0;
}

void puck_game_create_tower_geom(LWPUCKGAME* puck_game, int i) {
    LWPUCKGAMETOWER* tower = &puck_game->tower[i];
    assert(tower->geom == 0);
    tower->geom = dCreateCapsule(puck_game->space, puck_game->tower_radius, 10.0f);
    dGeomSetPosition(tower->geom,
                     puck_game->tower_pos * puck_game->tower_pos_multiplier[i][0],
                     puck_game->tower_pos * puck_game->tower_pos_multiplier[i][1],
                     0.0f);
    dGeomSetData(tower->geom, tower);
    // Tower #0(NW), #1(NE) --> player 1
    // Tower #2(SW), #3(SE) --> player 2
    tower->owner_player_no = i < LW_PUCK_GAME_TOWER_COUNT / 2 ? 1 : 2;
}

static void destroy_tower_geom(LWPUCKGAME* puck_game, int i) {
    LWPUCKGAMETOWER* tower = &puck_game->tower[i];
    assert(tower->geom);
    dGeomDestroy(tower->geom);
    tower->geom = 0;
}

static void create_control_joint(LWPUCKGAME* puck_game, LW_PUCK_GAME_OBJECT attach_target, dJointGroupID* joint_group, dJointID* control_joint) {
    assert(*control_joint == 0);
    *joint_group = dJointGroupCreate(0);
    *control_joint = dJointCreateLMotor(puck_game->world, *joint_group);
    dJointSetLMotorNumAxes(*control_joint, 2); // XY plane
    dJointSetLMotorAxis(*control_joint, 0, 0, 1, 0, 0); // x-axis actuator
    dJointSetLMotorAxis(*control_joint, 1, 0, 0, 1, 0); // y-axis actuator
    dJointAttach(*control_joint, puck_game->go[attach_target].body, 0);
    dJointSetLMotorParam(*control_joint, dParamFMax1, puck_game->control_joint_max_force);
    dJointSetLMotorParam(*control_joint, dParamFMax2, puck_game->control_joint_max_force);
}

static void destroy_control_joint(LWPUCKGAME* puck_game, dJointGroupID* joint_group, dJointID* control_joint) {
    assert(joint_group);
    dJointGroupDestroy(*joint_group);
    *joint_group = 0;
    // all joints within joint_group destroyed as a whole
    *control_joint = 0;
}

void puck_game_create_go(LWPUCKGAME* puck_game, int lpgo, float x, float y, float z, float radius) {
    create_go(puck_game, lpgo, puck_game->sphere_mass, radius);
    puck_game_reset_go(puck_game, &puck_game->go[lpgo], x, y, z);
}

void puck_game_create_capsule_go(LWPUCKGAME* puck_game, int lpgo, float x, float y, float z, float radius, float length) {
    create_capsule_go(puck_game, lpgo, puck_game->sphere_mass, radius, length);
    puck_game_reset_go(puck_game, &puck_game->go[lpgo], x, y, z);
}

void puck_game_create_control_joint(LWPUCKGAME* puck_game, int lpgo) {
    if (lpgo == LPGO_TARGET) {
        create_control_joint(puck_game, lpgo, &puck_game->target_control_joint_group[0], &puck_game->target_control_joint[0]);
    } else if (lpgo == LPGO_PLAYER) {
        create_control_joint(puck_game, lpgo, &puck_game->player_control_joint_group[0], &puck_game->player_control_joint[0]);
    } else {
        LOGEP("invalid lpgo");
    }
}

// planes always perpendicular to xy=0 plane have c = 0
//static void octagonal_plane_param(float h, float theta, float* a, float* b, float* d) {
//    float ct = cosf(theta);
//    float st = sinf(theta);
//    *a = -ct;
//    *b = -st;
//    *d = -h * ct;
//}
//
//static dGeomID create_octagonal_plane(dSpaceID space, float h, float theta) {
//    float a, b, d;
//    octagonal_plane_param(h, theta, &a, &b, &d);
//    return dCreatePlane(space, a, b, 0, d);
//}

static void calculate_x1_y1_for_octagonal_plane(float h, float* x1, float* y1) {
    /*float ct = cosf((float)(M_PI / 8));
    float st = sinf((float)(M_PI / 8));
    *x1 = h * ct * ct;
    *y1 = h * ct * st;*/
    *x1 = h;
    *y1 = 0;
}

static float calculate_d_for_octagonal_plane(float a, float b, float x1, float y1, float theta) {
    float rct = cosf(theta);
    float rst = sinf(theta);
    // calculate rotated (x1,y1)
    float x2 = rct * x1 - rst * y1;
    float y2 = rst * x1 + rct * y1;
    return a * x2 + b * y2;
}

static void calculate_a_b_for_octagonal_plane(float theta, float* a, float* b) {
    float ct = cosf(theta);
    float st = sinf(theta);
    *a = -ct;
    *b = -st;
}

void puck_game_create_walls(LWPUCKGAME* puck_game) {
    // dCreatePlane(..., a, b, c, d); ==> plane equation: a*x + b*y + c*z = d
    if (puck_game->boundary[LPGB_GROUND] == 0) {
        puck_game->boundary[LPGB_GROUND] = dCreatePlane(puck_game->space, 0, 0, 1, 0);
    }
    if (puck_game->game_map == LPGM_SQUARE) {
        // square map (4 planes)
        if (puck_game->boundary[LPGB_E] == 0) {
            puck_game->boundary[LPGB_E] = dCreatePlane(puck_game->space, -1, 0, 0, -puck_game->world_width_half);
        }
        if (puck_game->boundary[LPGB_W] == 0) {
            puck_game->boundary[LPGB_W] = dCreatePlane(puck_game->space, 1, 0, 0, -puck_game->world_width_half);
        }
        if (puck_game->boundary[LPGB_S] == 0) {
            puck_game->boundary[LPGB_S] = dCreatePlane(puck_game->space, 0, 1, 0, -puck_game->world_height_half);
        }
        if (puck_game->boundary[LPGB_N] == 0) {
            puck_game->boundary[LPGB_N] = dCreatePlane(puck_game->space, 0, -1, 0, -puck_game->world_height_half);
        }
    } else {
        // octagon map (8 planes)
        assert(puck_game->world_width_half == puck_game->world_height_half);
        float oct_x1, oct_y1;
        calculate_x1_y1_for_octagonal_plane(puck_game->world_width_half, &oct_x1, &oct_y1);
        for (int i = 0; i < 8; i++) {
            if (puck_game->boundary[LPGB_EE + i] == 0) {
                float oct_a, oct_b;
                calculate_a_b_for_octagonal_plane((float)(M_PI / 4 * i), &oct_a, &oct_b);
                float oct_d = calculate_d_for_octagonal_plane(oct_a, oct_b, oct_x1, oct_y1, (float)(M_PI / 4 * i));
                puck_game->boundary[LPGB_EE + i] = dCreatePlane(puck_game->space, oct_a, oct_b, 0, oct_d);
            }
        }
    }
    // Set Geom data to LPGB_* values
    for (int i = 0; i < LPGB_COUNT; i++) {
        if (puck_game->boundary[i]) {
            dGeomSetData(puck_game->boundary[i], (void*)(uintptr_t)i);
        }
    }
    if (puck_game->boundary[LPGB_DIAGONAL_1] == 0) {
        //puck_game->boundary[LPGB_DIAGONAL_1] = dCreatePlane(puck_game->space, -1, -1, 0, 0);
    }
    if (puck_game->boundary[LPGB_DIAGONAL_2] == 0) {
        //puck_game->boundary[LPGB_DIAGONAL_2] = dCreatePlane(puck_game->space, +1, +1, 0, 0);
    }
}

void puck_game_destroy_walls(LWPUCKGAME* puck_game) {
    for (int i = 0; i < LPGB_COUNT; i++) {
        if (puck_game->boundary[i]) {
            dGeomDestroy(puck_game->boundary[i]);
            puck_game->boundary[i] = 0;
        }
    }
}

// This function setup all dynamic objects needed for 1vs1 battle.
// It allows a 'partial' creation state.
// (can be called safely even if subset of battle objects exist)
void puck_game_create_all_battle_objects(LWPUCKGAME* puck_game) {
    // create walls
    puck_game_create_walls(puck_game);
    // create tower geoms
    for (int i = 0; i < LW_PUCK_GAME_TOWER_COUNT; i++) {
        if (puck_game->tower[i].geom == 0) {
            puck_game_create_tower_geom(puck_game, i);
        }
    }
    // create game objects (puck, player, target)
    if (puck_game->go[LPGO_PUCK].geom == 0) {
        puck_game_create_go(puck_game, LPGO_PUCK, 0, 0, 0, puck_game->puck_sphere_radius);
    }
    if (puck_game->go[LPGO_PLAYER].geom == 0) {
        if (puck_game->player_capsule) {
            puck_game_create_capsule_go(puck_game, LPGO_PLAYER, 0, 0, 0, puck_game->player_sphere_radius, puck_game->player_capsule_length);
        } else {
            puck_game_create_go(puck_game, LPGO_PLAYER, 0, 0, 0, puck_game->player_sphere_radius);
        }
    }
    if (puck_game->go[LPGO_TARGET].geom == 0) {
        if (puck_game->player_capsule) {
            puck_game_create_capsule_go(puck_game, LPGO_TARGET, 0, 0, 0, puck_game->target_sphere_radius, puck_game->player_capsule_length);
        } else {
            puck_game_create_go(puck_game, LPGO_TARGET, 0, 0, 0, puck_game->target_sphere_radius);
        }
    }
    // Create target control joint
    if (puck_game->target_control_joint_group[0] == 0) {
        puck_game_create_control_joint(puck_game, LPGO_TARGET);
    }
    // Create player control joint
    if (puck_game->player_control_joint_group[0] == 0) {
        puck_game_create_control_joint(puck_game, LPGO_PLAYER);
    }
}

// This function is a reverse of puck_game_create_all_battle_objects().
void puck_game_destroy_all_battle_objects(LWPUCKGAME* puck_game) {
    // destroy walls
    puck_game_destroy_walls(puck_game);
    // destroy tower geoms
    for (int i = 0; i < LW_PUCK_GAME_TOWER_COUNT; i++) {
        if (puck_game->tower[i].geom) {
            destroy_tower_geom(puck_game, i);
        }
    }
    // destroy game objects (puck, player, target)
    if (puck_game->go[LPGO_PUCK].geom) {
        destroy_go(puck_game, LPGO_PUCK);
    }
    if (puck_game->go[LPGO_PLAYER].geom) {
        destroy_go(puck_game, LPGO_PLAYER);
    }
    if (puck_game->go[LPGO_TARGET].geom) {
        destroy_go(puck_game, LPGO_TARGET);
    }
    // destroy target control joint
    if (puck_game->target_control_joint_group[0]) {
        destroy_control_joint(puck_game, &puck_game->target_control_joint_group[0], &puck_game->target_control_joint[0]);
    }
    // destroy player control joint
    if (puck_game->player_control_joint_group[0]) {
        destroy_control_joint(puck_game, &puck_game->player_control_joint_group[0], &puck_game->player_control_joint[0]);
    }
}

LWPUCKGAME* new_puck_game(int update_frequency, LW_PUCK_GAME_MAP gameMap) {
    // Static game data
    LWPUCKGAME* puck_game = malloc(sizeof(LWPUCKGAME));
    memset(puck_game, 0, sizeof(LWPUCKGAME));
    puck_game->game_map = gameMap;
    puck_game_set_static_default_values(puck_game);
    puck_game->prepare_step_wait_tick = 2 * update_frequency;
    
    // ------

    // Initialize OpenDE
    dInitODE2(0);
    puck_game->world = dWorldCreate();
    puck_game->space = dHashSpaceCreate(0);
    // set global physics engine parameters
    dWorldSetGravity(puck_game->world, 0, 0, -9.81f);
    dWorldSetCFM(puck_game->world, 1e-5f);
    // joint group for physical contacts
    puck_game->contact_joint_group = dJointGroupCreate(0);
    // only puck has a red overlay light for indicating ownership update
    puck_game->go[LPGO_PUCK].red_overlay = 1;
    // Puck game runtime reset
    puck_game_reset(puck_game);
    // flag this instance is ready to run simulation
    puck_game->init_ready = 1;
    puck_game->show_top_level_main_menu = 1;
    return puck_game;
}

void puck_game_set_static_default_values(LWPUCKGAME* puck_game) {
    // datasheet begin
    puck_game->world_width = 4.0f;
    puck_game->world_height = 4.0f;
    puck_game->wall_height = 0.8f;
    puck_game->dash_interval = 1.2f;
    puck_game->dash_duration = 0.1f;
    puck_game->dash_shake_time = 0.3f;
    puck_game->hp_shake_time = 0.3f;
    puck_game->jump_force = 35.0f;
    puck_game->jump_interval = 0.5f;
    puck_game->jump_shake_time = 0.5f;
    puck_game->puck_damage_contact_speed_threshold = 1.1f;
    puck_game->sphere_mass = 0.1f;
    puck_game->puck_sphere_radius = 0.12f; //0.16f;
    puck_game->player_sphere_radius = 0.12f; //0.16f;
    puck_game->target_sphere_radius = 0.12f; //0.16f;
    puck_game->total_time = 60.0f;
    puck_game->fire_max_force = 35.0f;
    puck_game->fire_max_vel = 5.0f;
    puck_game->fire_interval = 1.5f;
    puck_game->fire_duration = 0.2f;
    puck_game->fire_shake_time = 0.5f;
    if (puck_game->game_map == LPGM_SQUARE) {
        puck_game->tower_pos = 1.1f;
    } else {
        puck_game->tower_pos = 0.9f;
    }
    puck_game->tower_radius = 0.3f; //0.825f / 2;
    puck_game->tower_mesh_radius = 0.825f / 2; // Check tower.blend file
    puck_game->tower_total_hp = 5;
    puck_game->tower_shake_time = 0.2f;
    puck_game->go_start_pos = 0.6f;
    puck_game->hp = 5;
    puck_game->player_max_move_speed = 1.0f;
    puck_game->player_dash_speed = 6.0f;
    puck_game->boundary_impact_falloff_speed = 10.0f;
    puck_game->boundary_impact_start = 3.0f;
    puck_game->control_joint_max_force = 10.0f;
    puck_game->bounce = 0.9f;
    // tower pos
    int tower_pos_multiplier_index = 0;
    puck_game_set_tower_pos_multiplier(puck_game, tower_pos_multiplier_index, -1, -1);
    puck_game->tower_collapsing_z_rot_angle[tower_pos_multiplier_index] = (float)LWDEG2RAD(180);
    tower_pos_multiplier_index++;
    puck_game_set_tower_pos_multiplier(puck_game, tower_pos_multiplier_index, +1, +1);
    puck_game->tower_collapsing_z_rot_angle[tower_pos_multiplier_index] = (float)LWDEG2RAD(0);
    tower_pos_multiplier_index++;
    if (tower_pos_multiplier_index != LW_PUCK_GAME_TOWER_COUNT) {
        LOGE("Runtime assertion error");
        exit(-1);
    }
    puck_game->follow_cam = 0;
    puck_game->hide_hp_star = 0;
    puck_game->dash_by_direction = 0;
    puck_game->player_capsule = 0;
    // datasheet end
    puck_game_set_secondary_static_default_values(puck_game);
}

void puck_game_set_secondary_static_default_values(LWPUCKGAME* puck_game) {
    puck_game->world_width_half = puck_game->world_width / 2;
    puck_game->world_height_half = puck_game->world_height / 2;
    puck_game->pg_player[0].total_hp = puck_game->hp;
    puck_game->pg_player[0].current_hp = puck_game->hp;
    puck_game->pg_target[0].total_hp = puck_game->hp;
    puck_game->pg_target[0].current_hp = puck_game->hp;
    puck_game->puck_reflect_size = 1.0f;
}

void puck_game_set_tower_pos_multiplier(LWPUCKGAME* puck_game, int index, float mx, float my) {
    puck_game->tower_pos_multiplier[index][0] = mx;
    puck_game->tower_pos_multiplier[index][1] = my;
}

void delete_puck_game(LWPUCKGAME** puck_game) {
    if ((*puck_game) == 0) {
        LOGEP("already deleted puck game (try to re-delete)");
        return;
    }
    puck_game_destroy_all_battle_objects(*puck_game);
    dSpaceDestroy((*puck_game)->space);
    dWorldDestroy((*puck_game)->world);
    if ((*puck_game)->record) {
        free((*puck_game)->record);
    }
    free(*puck_game);
    *puck_game = 0;
}

LWPUCKGAMEDASH* puck_game_single_play_dash_object(LWPUCKGAME* puck_game) {
    return &puck_game->remote_dash[puck_game->player_no == 2 ? 1 : 0][0];
}

LWPUCKGAMEJUMP* puck_game_single_play_jump_object(LWPUCKGAME* puck_game) {
    return &puck_game->remote_jump[puck_game->player_no == 2 ? 1 : 0][0];
}

static void near_puck_go(LWPUCKGAME* puck_game, int player_no, dContact* contact) {
    //LWPUCKGAMEOBJECT* puck = &puck_game->go[LPGO_PUCK];
    //LWPUCKGAMEOBJECT* go = &puck_game->go[player_no == 1 ? LPGO_PLAYER : LPGO_TARGET];
    contact->surface.mode = dContactSoftCFM | dContactBounce;
    contact->surface.mu = 1.9f;

    if (puck_game->puck_owner_player_no == 0) {
        puck_game->puck_owner_player_no = player_no;
        puck_game->puck_reflect_size = 2.0f;
    } else if (puck_game->puck_owner_player_no != player_no) {
        puck_game->puck_reflect_size = 2.0f;
        puck_game->puck_owner_player_no = player_no;
        if (player_no == 1) {
            puck_game->pg_player[0].puck_contacted = 1;
        } else {
            puck_game->pg_target[0].puck_contacted = 1;
        }
    }
    // custom collision callback
    call_collision_callback(puck_game, contact, puck_game->on_puck_player_collision);
}

static void near_puck_player(LWPUCKGAME* puck_game, dContact* contact) {
    near_puck_go(puck_game, 1, contact);
}

static void near_puck_target(LWPUCKGAME* puck_game, dContact* contact) {
    near_puck_go(puck_game, 2, contact);
}

int is_wall_geom(LWPUCKGAME* puck_game, dGeomID maybe_wall_geom) {
    if (puck_game->game_map == LPGM_SQUARE) {
        return puck_game->boundary[LPGB_E] == maybe_wall_geom
            || puck_game->boundary[LPGB_W] == maybe_wall_geom
            || puck_game->boundary[LPGB_S] == maybe_wall_geom
            || puck_game->boundary[LPGB_N] == maybe_wall_geom;
    } else {
        return puck_game->boundary[LPGB_EE] == maybe_wall_geom
            || puck_game->boundary[LPGB_EN] == maybe_wall_geom
            || puck_game->boundary[LPGB_NN] == maybe_wall_geom
            || puck_game->boundary[LPGB_NW] == maybe_wall_geom
            || puck_game->boundary[LPGB_WW] == maybe_wall_geom
            || puck_game->boundary[LPGB_WS] == maybe_wall_geom
            || puck_game->boundary[LPGB_SS] == maybe_wall_geom
            || puck_game->boundary[LPGB_SE] == maybe_wall_geom;
    }
}

LWPUCKGAMETOWER* get_tower_from_geom(LWPUCKGAME* puck_game, dGeomID maybe_tower_geom) {
    for (int i = 0; i < LW_PUCK_GAME_TOWER_COUNT; i++) {
        if (maybe_tower_geom == puck_game->tower[i].geom) {
            return &puck_game->tower[i];
        }
    }
    return 0;
}

static void near_puck_wall(LWPUCKGAME* puck_game, dGeomID puck_geom, dGeomID wall_geom, const dContact* contact) {
    LW_PUCK_GAME_BOUNDARY boundary = (LW_PUCK_GAME_BOUNDARY)dGeomGetData(wall_geom);
    if (boundary < LPGB_E || boundary > LPGB_SE) {
        LOGE("boundary geom data corrupted");
        return;
    }
    if (puck_game->game_map == LPGM_SQUARE) {
        puck_game->wall_hit_bit |= 1 << (boundary - LPGB_E);
    } else {
        puck_game->wall_hit_bit |= 1 << (boundary - LPGB_EE);
    }
    puck_game->wall_hit_bit_send_buf_1 |= puck_game->wall_hit_bit;
    puck_game->wall_hit_bit_send_buf_2 |= puck_game->wall_hit_bit;
    // custom collision callback
    call_collision_callback(puck_game, contact, puck_game->on_puck_wall_collision);
}

static void near_puck_tower(LWPUCKGAME* puck_game, dGeomID puck_geom, LWPUCKGAMETOWER* tower, dContact* contact, double now) {
    // Puck - tower contacts
    // Set basic contact parameters first
    contact->surface.mode = dContactSoftCFM | dContactBounce;
    contact->surface.mu = 1.9f;
    contact->surface.bounce_vel = 0;
    // custom collision callback
    call_collision_callback(puck_game, contact, puck_game->on_puck_tower_collision);
    // Check puck ownership
    if (puck_game->puck_owner_player_no == tower->owner_player_no) {
        return;
    }
    if (puck_game->puck_owner_player_no == 0) {
        return;
    }
    // Check minimum contact damage speed threshold
    dBodyID puck_body = dGeomGetBody(puck_geom);
    dReal puck_speed = dLENGTH(dBodyGetLinearVel(puck_body));
    if (puck_speed < (dReal)1.0) {
        return;
    }
    // Check last damaged cooltime
    if (now - tower->last_damaged_at > 1.0f && tower->invincible == 0) {
        int* player_hp_ptr = tower->owner_player_no == 1 ? &puck_game->pg_player[0].current_hp : &puck_game->pg_target[0].current_hp;
        if (tower->hp > 0 || *player_hp_ptr > 0) {
            if (tower->hp > 0) {
                tower->hp--;
            }
            if (*player_hp_ptr > 0) {
                const int old_hp = *player_hp_ptr;
                (*player_hp_ptr)--;
                const int new_hp = *player_hp_ptr;
                if (old_hp > 0 && new_hp == 0) {
                    // on_death...
                    tower->collapsing = 1;
                    tower->collapsing_time = 0;
                    puck_game->battle_phase = tower->owner_player_no == 1 ? LSP_FINISHED_VICTORY_P2 : LSP_FINISHED_VICTORY_P1;
                    puck_game->battle_control_ui_alpha = 0;
                    if (puck_game->on_finished) {
                        puck_game->on_finished(puck_game, tower->owner_player_no == 1 ? 2 : 1);
                    }
                }
            }
            tower->shake_remain_time = puck_game->tower_shake_time;
            tower->last_damaged_at = now;

            if (tower->owner_player_no == 1) {
                if (puck_game->on_player_damaged) {
                    puck_game->on_player_damaged(puck_game);
                }
            } else {
                if (puck_game->on_target_damaged) {
                    puck_game->on_target_damaged(puck_game);
                }
            }
        }
    }
}

void puck_game_near_callback(void* data, dGeomID geom1, dGeomID geom2) {
    LWPUCKGAME* puck_game = (LWPUCKGAME*)data;
    // Early pruning
    // LPGB_DIAGONAL_1 should collided only with LPGO_PLAYER
    // LPGB_DIAGONAL_2 should collided only with LPGO_TARGET
    if (puck_game->boundary[LPGB_DIAGONAL_1]) {
        if (geom1 == puck_game->boundary[LPGB_DIAGONAL_1] || geom2 == puck_game->boundary[LPGB_DIAGONAL_1]) {
            if (geom1 == puck_game->go[LPGO_PLAYER].geom || geom2 == puck_game->go[LPGO_PLAYER].geom) {

            } else {
                return;
            }
        }
    }
    if (puck_game->boundary[LPGB_DIAGONAL_2]) {
        if (geom1 == puck_game->boundary[LPGB_DIAGONAL_2] || geom2 == puck_game->boundary[LPGB_DIAGONAL_2]) {
            if (geom1 == puck_game->go[LPGO_TARGET].geom || geom2 == puck_game->go[LPGO_TARGET].geom) {

            } else {
                return;
            }
        }
    }
    const double now = lwtimepoint_now_seconds();
    if (dGeomIsSpace(geom1) || dGeomIsSpace(geom2)) {

        // colliding a space with something :
        dSpaceCollide2(geom1, geom2, data, &puck_game_near_callback);

        // collide all geoms internal to the space(s)
        if (dGeomIsSpace(geom1))
            dSpaceCollide((dSpaceID)geom1, data, &puck_game_near_callback);
        if (dGeomIsSpace(geom2))
            dSpaceCollide((dSpaceID)geom2, data, &puck_game_near_callback);

    } else {

        //if (geom1 == puck_game->boundary[LPGB_DIAGONAL_2] || geom2 == puck_game->boundary[LPGB_DIAGONAL_2]) {
        //    if (geom1 == puck_game->go[LPGO_TARGET].geom || puck_game->go[LPGO_TARGET].geom) {

        //    } else {
        //        return;
        //    }
        //}

        const int max_contacts = 5;
        dContact contact[5];
        // colliding two non-space geoms, so generate contact
        // points between geom1 and geom2
        int num_contact = dCollide(geom1, geom2, max_contacts, &contact->geom, sizeof(dContact));
        // add these contact points to the simulation ...
        LWPUCKGAMETOWER* tower = 0;
        for (int i = 0; i < num_contact; i++) {
            // bounce is the amount of "bouncyness".
            contact[i].surface.bounce = puck_game->bounce;
            // bounce_vel is the minimum incoming velocity to cause a bounce
            contact[i].surface.bounce_vel = 0.1f;
            // constraint force mixing parameter
            contact[i].surface.soft_cfm = 0.001f;

            if (geom1 == puck_game->boundary[LPGB_GROUND] || geom2 == puck_game->boundary[LPGB_GROUND]) {
                // All objects - ground contacts
                contact[i].surface.mode = dContactSoftCFM | dContactRolling;// | dContactFDir1;
                contact[i].surface.rho = 0.001f;
                contact[i].surface.rho2 = 0.001f;
                contact[i].surface.rhoN = 0.001f;
                //contact[i].fdir1[0] = 1.0f;
                //contact[i].fdir1[1] = 0.0f;
                //contact[i].fdir1[2] = 0.0f;
                contact[i].surface.mu = 100.9f;
            } else if ((geom1 == puck_game->go[LPGO_PUCK].geom && geom2 == puck_game->go[LPGO_PLAYER].geom)
                       || (geom1 == puck_game->go[LPGO_PLAYER].geom && geom2 == puck_game->go[LPGO_PUCK].geom)) {
                // Player - puck contacts
                near_puck_player(puck_game, &contact[i]);
            } else if ((geom1 == puck_game->go[LPGO_PUCK].geom && geom2 == puck_game->go[LPGO_TARGET].geom)
                       || (geom1 == puck_game->go[LPGO_TARGET].geom && geom2 == puck_game->go[LPGO_PUCK].geom)) {
                // Target - puck contacts
                near_puck_target(puck_game, &contact[i]);
            } else if (geom1 == puck_game->go[LPGO_PUCK].geom && (tower = get_tower_from_geom(puck_game, geom2))) {
                // Puck - tower contacts
                near_puck_tower(puck_game, geom1, tower, &contact[i], now);
            } else if (geom2 == puck_game->go[LPGO_PUCK].geom && (tower = get_tower_from_geom(puck_game, geom1))) {
                // Puck - tower contacts
                near_puck_tower(puck_game, geom2, tower, &contact[i], now);
            } else if (geom1 == puck_game->go[LPGO_PUCK].geom && is_wall_geom(puck_game, geom2)) {
                // Puck - wall contacts
                contact[i].surface.mode = dContactSoftCFM | dContactBounce;
                contact[i].surface.mu = 0;//1.9f;

                near_puck_wall(puck_game, geom1, geom2, &contact[i]);
            } else if (geom2 == puck_game->go[LPGO_PUCK].geom && is_wall_geom(puck_game, geom1)) {
                // Puck - wall contacts
                contact[i].surface.mode = dContactSoftCFM | dContactBounce;
                contact[i].surface.mu = 0;//1.9f;

                near_puck_wall(puck_game, geom2, geom1, &contact[i]);
            } else {
                // Other contacts
                contact[i].surface.mode = dContactSoftCFM | dContactBounce;
                contact[i].surface.mu = 0;//1.9f;
            }

            dJointID c = dJointCreateContact(puck_game->world, puck_game->contact_joint_group, &contact[i]);
            dBodyID b1 = dGeomGetBody(geom1);
            dBodyID b2 = dGeomGetBody(geom2);
            dJointAttach(c, b1, b2);
        }
    }
}

void puck_game_push(LWPUCKGAME* puck_game) {
    //puck_game->push = 1;

    dBodySetLinearVel(puck_game->go[LPGO_PUCK].body, 1, 1, 0);
}

float puck_game_dash_gauge_ratio(LWPUCKGAME* puck_game, const LWPUCKGAMEDASH* dash) {
    return LWMIN(1.0f, puck_game_dash_elapsed_since_last(puck_game, dash) / puck_game->dash_interval);
}

float puck_game_dash_elapsed_since_last(const LWPUCKGAME* puck_game, const LWPUCKGAMEDASH* dash) {
    return puck_game->time - dash->last_time;
}

int puck_game_dash_can_cast(const LWPUCKGAME* puck_game, const LWPUCKGAMEDASH* dash) {
    return puck_game_dash_elapsed_since_last(puck_game, dash) >= puck_game->dash_interval;
}

float puck_game_jump_cooltime(LWPUCKGAME* puck_game) {
    LWPUCKGAMEJUMP* jump = puck_game_single_play_jump_object(puck_game);
    return puck_game->time - jump->last_time;
}

int puck_game_jumping(LWPUCKGAMEJUMP* jump) {
    return jump->remain_time > 0;
}

int puck_game_dashing(LWPUCKGAMEDASH* dash) {
    return dash->remain_time > 0;
}

void puck_game_commit_jump(LWPUCKGAME* puck_game, LWPUCKGAMEJUMP* jump, int player_no) {
    jump->remain_time = puck_game->jump_interval;
    jump->last_time = puck_game->time;
}

void puck_game_commit_dash(LWPUCKGAME* puck_game, LWPUCKGAMEDASH* dash, float dx, float dy, int player_no) {
    assert(player_no == 1 || player_no == 2);
    puck_game->battle_stat[player_no - 1].Dash++;
    dash->remain_time = puck_game->dash_duration;
    dash->dir_x = dx;
    dash->dir_y = dy;
    dash->last_time = puck_game->time;
    if (puck_game->on_player_dash) {
        puck_game->on_player_dash(puck_game);
    }
}

void puck_game_commit_dash_to_puck(LWPUCKGAME* puck_game, LWPUCKGAMEDASH* dash, int player_no) {
    float dx = puck_game->go[LPGO_PUCK].pos[0] - puck_game->go[player_no == 1 ? LPGO_PLAYER : LPGO_TARGET].pos[0];
    float dy = puck_game->go[LPGO_PUCK].pos[1] - puck_game->go[player_no == 1 ? LPGO_PLAYER : LPGO_TARGET].pos[1];
    const float ddlen = sqrtf(dx * dx + dy * dy);
    dx /= ddlen;
    dy /= ddlen;
    puck_game_commit_dash(puck_game, dash, dx, dy, player_no);
}

float puck_game_elapsed_time(int update_tick, int update_frequency) {
    return update_tick * 1.0f / update_frequency;
}

float puck_game_remain_time(float total_time, int update_tick, int update_frequency) {
    return LWMAX(0, total_time - puck_game_elapsed_time(update_tick, update_frequency));
}

int puck_game_remain_time_floor(float total_time, int update_tick, int update_frequency) {
    return (int)floorf(puck_game_remain_time(total_time, update_tick, update_frequency));
}

void puck_game_commit_fire(LWPUCKGAME* puck_game, LWPUCKGAMEFIRE* fire, int player_no, float puck_fire_dx, float puck_fire_dy, float puck_fire_dlen) {
    fire->remain_time = puck_game->fire_duration;
    fire->last_time = puck_game->time;
    fire->dir_x = puck_fire_dx;
    fire->dir_y = puck_fire_dy;
    fire->dir_len = puck_fire_dlen;
}

void update_puck_reflect_size(LWPUCKGAME* puck_game, float delta_time) {
    if (puck_game->puck_reflect_size > 1.0f) {
        puck_game->puck_reflect_size = LWMAX(1.0f, puck_game->puck_reflect_size - (float)delta_time * 2);
    }
}

void update_puck_ownership(LWPUCKGAME* puck_game) {
    const float speed = puck_game->go[LPGO_PUCK].speed;
    const float red_overlay_ratio = LWMIN(1.0f, speed / puck_game->puck_damage_contact_speed_threshold);
    if (puck_game->puck_owner_player_no != 0 && red_overlay_ratio < 0.5f) {
        puck_game->puck_owner_player_no = 0;
        //puck_game->puck_reflect_size = 2.0f;
    }
}

void puck_game_reset_go(LWPUCKGAME* puck_game, LWPUCKGAMEOBJECT* go, float x, float y, float z) {
    // reset physics engine values
    if (go->body) {
        dBodySetPosition(go->body, x, y, z);
        dMatrix3 rot_identity = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
        };
        dBodySetRotation(go->body, rot_identity);
        dBodySetLinearVel(go->body, 0, 0, 0);
        dBodySetAngularVel(go->body, 0, 0, 0);
        dBodySetForce(go->body, 0, 0, 0);
        dBodySetTorque(go->body, 0, 0, 0);
    }
    // reset cached values
    go->move_rad = 0;
    go->pos[0] = x;
    go->pos[1] = y;
    go->pos[2] = z;
    mat4x4_identity(go->rot);
    go->speed = 0;
    go->wall_hit_count = 0;
}

void puck_game_remote_state_reset(LWPUCKGAME* puck_game, LWPSTATE* state) {
    state->bf.puck_owner_player_no = 0;
    state->bf.player_current_hp = puck_game->hp;
    state->bf.player_total_hp = puck_game->hp;
    state->bf.target_current_hp = puck_game->hp;
    state->bf.target_total_hp = puck_game->hp;
    state->bf.player_pull = 0;
    state->bf.target_pull = 0;
}

void puck_game_tower_pos(vec4 p_out, const LWPUCKGAME* puck_game, int owner_player_no) {
    if (owner_player_no != 1 && owner_player_no != 2) {
        LOGEP("invalid owner_player_no: %d", owner_player_no);
        return;
    }
    p_out[0] = puck_game->tower_pos * puck_game->tower_pos_multiplier[owner_player_no - 1][0];
    p_out[1] = puck_game->tower_pos * puck_game->tower_pos_multiplier[owner_player_no - 1][1];
    p_out[2] = 0.0f;
    p_out[3] = 1.0f;
}

void puck_game_set_dash_disabled(LWPUCKGAME* puck_game, int index, int v) {
    // should reset all dash data to prevent already committed dash to be shown
    if (v == 1) {
        memset(&puck_game->remote_dash[index][0], 0, sizeof(puck_game->remote_dash[index][0]));
    }
    puck_game->remote_dash[index][0].disabled = v;
}

void puck_game_set_bogus_disabled(LWPUCKGAME* puck_game, int v) {
    puck_game->bogus_disabled = v;
}

void puck_game_control_bogus(LWPUCKGAME* puck_game, const LWPUCKGAMEBOGUSPARAM* bogus_param) {
    float ideal_target_dx = puck_game->go[LPGO_PUCK].pos[0] - puck_game->go[LPGO_TARGET].pos[0];
    float ideal_target_dy = puck_game->go[LPGO_PUCK].pos[1] - puck_game->go[LPGO_TARGET].pos[1];
    puck_game->target_dx[0] = (1.0f - bogus_param->target_follow_agility) * puck_game->target_dx[0] + bogus_param->target_follow_agility * ideal_target_dx;
    puck_game->target_dy[0] = (1.0f - bogus_param->target_follow_agility) * puck_game->target_dy[0] + bogus_param->target_follow_agility * ideal_target_dy;
    // normalize target dx, dy
    float target_dlen = sqrtf(puck_game->target_dx[0] * puck_game->target_dx[0] + puck_game->target_dy[0] * puck_game->target_dy[0]);
    puck_game->target_dx[0] /= target_dlen;
    puck_game->target_dy[0] /= target_dlen;

    float ideal_target_dx2 = ideal_target_dx * ideal_target_dx;
    float ideal_target_dy2 = ideal_target_dy * ideal_target_dy;
    float ideal_target_dlen = sqrtf(ideal_target_dx2 + ideal_target_dy2);

    float ideal_target_dlen_ratio = 1.0f;
    if (ideal_target_dlen < 0.5f) {
        ideal_target_dlen_ratio = 0.5f;
    }
    puck_game->target_dlen_ratio[0] = (1.0f - bogus_param->target_follow_agility) * puck_game->target_dlen_ratio[0] + bogus_param->target_follow_agility * ideal_target_dlen_ratio;

    puck_game->remote_control[LW_PUCK_GAME_TARGET_TEAM][0].dir_pad_dragging = 1;
    puck_game->remote_control[LW_PUCK_GAME_TARGET_TEAM][0].dx = puck_game->target_dx[0];
    puck_game->remote_control[LW_PUCK_GAME_TARGET_TEAM][0].dy = puck_game->target_dy[0];
    puck_game->remote_control[LW_PUCK_GAME_TARGET_TEAM][0].dlen = puck_game->target_dlen_ratio[0];
    puck_game->remote_control[LW_PUCK_GAME_TARGET_TEAM][0].pull_puck = 0;

    LOGIx("[BOGUS] dx=%.2f, dy=%.2f, dlen=xxxx, dlen_max=xxxx, dlen_ratio=%.2f",
          puck_game->target_dx[0],
          puck_game->target_dy[0],
          puck_game->target_dlen_ratio[0]);

    // dash
    int bogus_player_no = puck_game->player_no == 2 ? 1 : 2;
    LWPUCKGAMEDASH* dash = &puck_game->remote_dash[bogus_player_no - 1][0];
    if (dash->disabled == 0 && ideal_target_dlen < bogus_param->dash_detect_radius) {
        if (numcomp_float_random_01_local(&puck_game->bogus_rng) < bogus_param->dash_frequency) {
            const float dash_cooltime_aware_lag = numcomp_float_random_range_local(&puck_game->bogus_rng, bogus_param->dash_cooltime_lag_min, bogus_param->dash_cooltime_lag_max);
            if (puck_game_dash_elapsed_since_last(puck_game, dash) >= puck_game->dash_interval + dash_cooltime_aware_lag) {
                puck_game_dash(puck_game, dash, bogus_player_no);
            }
        }
    }
}

void puck_game_update_remote_player(LWPUCKGAME* puck_game, float delta_time, int i) {
    dJointID pcj[2] = {
        puck_game->player_control_joint[0],
        puck_game->target_control_joint[0],
    };
    LW_PUCK_GAME_OBJECT control_enum[2] = {
        LPGO_PLAYER,
        LPGO_TARGET,
    };

    if (pcj[i]) {
        if (puck_game->remote_control[i][0].dir_pad_dragging) {
            float dx, dy, dlen;
            dx = puck_game->remote_control[i][0].dx;
            dy = puck_game->remote_control[i][0].dy;
            dlen = puck_game->remote_control[i][0].dlen;
            if (dlen > 1.0f) {
                dlen = 1.0f;
            }
            dJointEnable(pcj[i]);
            dJointSetLMotorParam(pcj[i], dParamVel1, puck_game->player_max_move_speed * dx * dlen);
            dJointSetLMotorParam(pcj[i], dParamVel2, puck_game->player_max_move_speed * dy * dlen);
        } else {
            dJointSetLMotorParam(pcj[i], dParamVel1, 0);
            dJointSetLMotorParam(pcj[i], dParamVel2, 0);
        }
        // Move direction fixed while dashing
        if (puck_game->remote_dash[i][0].remain_time > 0) {
            float dx, dy;
            dx = puck_game->remote_dash[i][0].dir_x;
            dy = puck_game->remote_dash[i][0].dir_y;
            dJointSetLMotorParam(pcj[i], dParamVel1, puck_game->player_dash_speed * dx);
            dJointSetLMotorParam(pcj[i], dParamVel2, puck_game->player_dash_speed * dy);
            puck_game->remote_dash[i][0].remain_time = LWMAX(0,
                                                             puck_game->remote_dash[i][0].remain_time - delta_time);
        }
    }
    // Jump
    if (puck_game->remote_jump[i][0].remain_time > 0) {
        puck_game->remote_jump[i][0].remain_time = 0;
        dBodyAddForce(puck_game->go[control_enum[i]].body, 0, 0, puck_game->jump_force);
    }
    // Pull
    if (puck_game->remote_control[i][0].pull_puck && puck_game->go[LPGO_PUCK].body) {
        const dReal *puck_pos = dBodyGetPosition(puck_game->go[LPGO_PUCK].body);
        const dReal *player_pos = dBodyGetPosition(puck_game->go[control_enum[i]].body);
        const dVector3 f = {
            player_pos[0] - puck_pos[0],
            player_pos[1] - puck_pos[1],
            player_pos[2] - puck_pos[2]
        };
        const dReal flen = dLENGTH(f);
        const dReal power = 0.1f;
        const dReal scale = power / flen;
        dBodyAddForce(puck_game->go[LPGO_PUCK].body, f[0] * scale, f[1] * scale, f[2] * scale);
    }
    // Fire
    if (puck_game->remote_fire[i][0].remain_time > 0) {
        // [1] Player Control Joint Version
        /*dJointSetLMotorParam(pcj, dParamVel1, puck_game->fire.dir_x * puck_game->fire_max_force * puck_game->fire.dir_len);
        dJointSetLMotorParam(pcj, dParamVel2, puck_game->fire.dir_y * puck_game->fire_max_force * puck_game->fire.dir_len);
        puck_game->fire.remain_time = LWMAX(0, puck_game->fire.remain_time - (float)delta_time);*/

        // [2] Impulse Force Version
        if (pcj[i]) {
            dJointDisable(pcj[i]);
        }
        dBodySetLinearVel(puck_game->go[control_enum[i]].body, 0, 0, 0);
        dBodyAddForce(puck_game->go[control_enum[i]].body,
                      puck_game->remote_fire[i][0].dir_x * puck_game->fire_max_force *
                      puck_game->remote_fire[i][0].dir_len,
                      puck_game->remote_fire[i][0].dir_y * puck_game->fire_max_force *
                      puck_game->remote_fire[i][0].dir_len,
                      0);
        puck_game->remote_fire[i][0].remain_time = 0;
    }
}

int puck_game_dash(LWPUCKGAME* puck_game, LWPUCKGAMEDASH* dash, int player_no) {
    // Check params
    if (!puck_game || !dash) {
        return -1;
    }
    // Check already effective dash
    if (puck_game_dashing(dash)) {
        return -2;
    }
    // Check effective move input
    //float dx, dy, dlen;
    /*if (!lw_get_normalized_dir_pad_input(pLwc, &dx, &dy, &dlen)) {
    return;
    }*/

    // Check cooltime
    if (puck_game_dash_can_cast(puck_game, dash) == 0) {
        dash->shake_remain_time = puck_game->dash_shake_time;
        return -3;
    }

    // Start dash!
    if (puck_game->dash_by_direction) {
        puck_game_commit_dash(puck_game, dash, puck_game->remote_control[LW_PUCK_GAME_PLAYER_TEAM][0].dx, puck_game->remote_control[LW_PUCK_GAME_PLAYER_TEAM][0].dy, player_no);
    } else {
        puck_game_commit_dash_to_puck(puck_game, dash, player_no);
    }
    return 0;
}

void puck_game_set_searching_str(LWPUCKGAME* puck_game, const char* str) {
    strcpy(puck_game->searching_str, str);
}

void puck_game_set_tutorial_guide_str(LWPUCKGAME* puck_game, const char* str) {
    strcpy(puck_game->tutorial_guide_str, str);
}

static void set_gameobject_state(const LWPUCKGAMEOBJECT* go, const LWPUCKGAMERECORDFRAMEGAMEOBJECT* record_go) {
    dBodySetPosition(go->body, record_go->pos[0], record_go->pos[1], record_go->pos[2]);
    dBodySetLinearVel(go->body, record_go->linvel[0], record_go->linvel[1], record_go->linvel[2]);
    dBodySetQuaternion(go->body, record_go->quat);
    dBodySetAngularVel(go->body, record_go->angularvel[0], record_go->angularvel[1], record_go->angularvel[2]);
}

static void puck_game_restore_state_bitfield(LWPUCKGAME* puck_game, LWPUCKGAMEPLAYER* player, LWPUCKGAMEPLAYER* target, int* wall_hit_bit, const LWPSTATEBITFIELD* bf) {
    player->current_hp = (int)bf->player_current_hp;
    player->total_hp = (int)bf->player_total_hp;
    target->current_hp = (int)bf->target_current_hp;
    target->total_hp = (int)bf->target_total_hp;
    puck_game->puck_owner_player_no = (int)bf->puck_owner_player_no;
    puck_game->battle_phase = (LWP_STATE_PHASE)bf->phase;
    puck_game->remote_control[LW_PUCK_GAME_PLAYER_TEAM][0].pull_puck = (int)bf->player_pull;
    puck_game->remote_control[LW_PUCK_GAME_TARGET_TEAM][0].pull_puck = (int)bf->target_pull;
    *wall_hit_bit = (int)bf->wall_hit_bit;
}

void puck_game_update_tick(LWPUCKGAME* puck_game, int update_frequency) {
    // update puck game time
    puck_game->time = puck_game_elapsed_time(puck_game->update_tick, update_frequency);
    // reset per-frame caches
    puck_game->pg_player[0].puck_contacted = 0;
    puck_game->pg_target[0].puck_contacted = 0;
    // reset wall hit bits
    puck_game->wall_hit_bit = 0;
    // check timeout condition
    if (puck_game_remain_time(puck_game->total_time, puck_game->update_tick, update_frequency) <= 0
        && puck_game->game_state != LPGS_TUTORIAL) {
        const int hp_diff = puck_game->pg_player[0].current_hp - puck_game->pg_target[0].current_hp;
        puck_game->battle_phase = hp_diff > 0 ? LSP_FINISHED_VICTORY_P1 : hp_diff < 0 ? LSP_FINISHED_VICTORY_P2 : LSP_FINISHED_DRAW;
        puck_game->battle_control_ui_alpha = 0;
    }
    // transition from READY to STEADY to GO
    if (puck_game->battle_phase == LSP_READY) {
        puck_game->prepare_step_waited_tick++;
        if (puck_game->prepare_step_waited_tick >= puck_game->prepare_step_wait_tick) {
            if (puck_game->battle_phase != LSP_STEADY) {
                if (puck_game->on_steady) {
                    puck_game->on_steady(puck_game);
                }
            }
            puck_game->battle_phase = LSP_STEADY;
            puck_game->prepare_step_waited_tick = 0;
            puck_game->battle_control_ui_alpha = 0.2f;
        }
    } else if (puck_game->battle_phase == LSP_STEADY) {
        puck_game->prepare_step_waited_tick++;
        if (puck_game->prepare_step_waited_tick >= puck_game->prepare_step_wait_tick) {
            if (puck_game->battle_phase != LSP_GO) {
                if (puck_game->on_go) {
                    puck_game->on_go(puck_game);
                }
            }
            puck_game->battle_phase = LSP_GO;
            puck_game->prepare_step_waited_tick = 0;
            puck_game->battle_control_ui_alpha = 1.0f;
        }
    }
    // stepping physics only if battling
    if (puck_game_state_phase_battling(puck_game->battle_phase)
        || puck_game->battle_phase == LSP_TUTORIAL) {
        // real battle or record play?
        if (puck_game->on_new_record_frame) {
            // real battle - record this game
            puck_game->on_new_record_frame(puck_game, puck_game->record, puck_game->update_tick);
        } else if (puck_game->record_replay_mode && puck_game->record) {
            // record play - overwrite all states
            if (puck_game->update_tick < puck_game->record->num_frame) {
                const LWPUCKGAMERECORDFRAME* f = &puck_game->record->frames[puck_game->update_tick];
                set_gameobject_state(&puck_game->go[0], &f->go[0]);
                set_gameobject_state(&puck_game->go[1], &f->go[1]);
                set_gameobject_state(&puck_game->go[2], &f->go[2]);
                LWPUCKGAMEPLAYER* player = &puck_game->pg_player[0];
                LWPUCKGAMEPLAYER* target = &puck_game->pg_target[0];
                int* wall_hit_bit = &puck_game->wall_hit_bit_send_buf_1;
                puck_game_restore_state_bitfield(puck_game, player, target, wall_hit_bit, &f->bf);
            } else {
                LOGEP("puck_game->update_tick >= puck_game->record->num_frame!");
            }
        } else if (puck_game->game_state == LPGS_TUTORIAL) {
            // do nothing on tutorial
        } else if (puck_game->game_state == LPGS_PRACTICE) {
            // do nothing on practice
        } else {
            // WTF?
            LOGEP("Unknown case");
        }
        puck_game->update_tick++;
        dSpaceCollide(puck_game->space, puck_game, puck_game_near_callback);
        dWorldQuickStep(puck_game->world, 1.0f / 60);
        dJointGroupEmpty(puck_game->contact_joint_group);
    }
    if (puck_game->go[LPGO_PLAYER].capsule) {
        dQuaternion q = { 1, 0, 0, 0 };
        dBodySetQuaternion(puck_game->go[LPGO_PLAYER].body, q);
        dBodySetAngularVel(puck_game->go[LPGO_PLAYER].body, 0, 0, 0);
    }
    if (puck_game->go[LPGO_TARGET].capsule) {
        dQuaternion q = { 1, 0, 0, 0 };
        dBodySetQuaternion(puck_game->go[LPGO_TARGET].body, q);
        dBodySetAngularVel(puck_game->go[LPGO_TARGET].body, 0, 0, 0);
    }
    // update last contact puck body
    if (puck_game->pg_player[0].puck_contacted == 0) {
        puck_game->pg_player[0].last_contact_puck_body = 0;
    }
    if (puck_game->pg_target[0].puck_contacted == 0) {
        puck_game->pg_target[0].last_contact_puck_body = 0;
    }
    // update battle stat stat (max puck speed)
    const float puck_speed_int = puck_game->go[LPGO_PUCK].speed;
    if (puck_game->puck_owner_player_no == 1) {
        puck_game->battle_stat[0].MaxPuckSpeed = LWMAX(puck_speed_int, puck_game->battle_stat[0].MaxPuckSpeed);
    } else if (puck_game->puck_owner_player_no == 2) {
        puck_game->battle_stat[1].MaxPuckSpeed = LWMAX(puck_speed_int, puck_game->battle_stat[1].MaxPuckSpeed);
    }
    // update battle stat stat (travel distance)
    puck_game->battle_stat[0].TravelDistance += puck_game->go[LPGO_PLAYER].speed / update_frequency;
    puck_game->battle_stat[1].TravelDistance += puck_game->go[LPGO_TARGET].speed / update_frequency;
}

void puck_game_set_tower_invincible(LWPUCKGAME* puck_game, int tower_index, int v) {
    puck_game->tower[tower_index].invincible = v;
}

void puck_game_reset_battle_state(LWPUCKGAME* puck_game) {
    // reset physics engine random seed again
    dRandSetSeed(0);
    puck_game->update_tick = 0;
    puck_game->prepare_step_waited_tick = 0;
    if (puck_game->battle_phase != LSP_READY) {
        if (puck_game->on_ready) {
            puck_game->on_ready(puck_game);
        }
    }
    puck_game->battle_phase = LSP_READY;
    // recreate all battle objects if not exists
    puck_game_create_all_battle_objects(puck_game);
    for (int i = 0; i < LW_PUCK_GAME_TOWER_COUNT; i++) {
        puck_game->tower[i].hp = puck_game->tower_total_hp;
        puck_game->tower[i].collapsing = 0;
    }
    puck_game_reset_go(puck_game, &puck_game->go[LPGO_PUCK], 0.0f, 0.0f, puck_game->go[LPGO_PUCK].radius);
    puck_game_reset_go(puck_game, &puck_game->go[LPGO_PLAYER], -puck_game->go_start_pos, -puck_game->go_start_pos, puck_game->go[LPGO_PUCK].radius);
    puck_game_reset_go(puck_game, &puck_game->go[LPGO_TARGET], +puck_game->go_start_pos, +puck_game->go_start_pos, puck_game->go[LPGO_PUCK].radius);
    puck_game->pg_player[0].total_hp = puck_game->hp;
    puck_game->pg_player[0].current_hp = puck_game->hp;
    puck_game->pg_target[0].total_hp = puck_game->hp;
    puck_game->pg_target[0].current_hp = puck_game->hp;
    memset(puck_game->remote_control, 0, sizeof(puck_game->remote_control));
    puck_game->control_flags &= ~LPGCF_HIDE_TIMER;
    puck_game->control_flags &= ~LPGCF_HIDE_PULL_BUTTON;
    puck_game->control_flags &= ~LPGCF_HIDE_DASH_BUTTON;
    // reset bogus control variables
    puck_game->target_dx[0] = 0;
    puck_game->target_dy[0] = 0;
    puck_game->target_dlen_ratio[0] = 0;
    // re-seed bogus rng
    pcg32_srandom_r(&puck_game->bogus_rng, 0x01041685967ULL, 0x027855157ULL);
    // reset dash state
    memset(puck_game->remote_dash, 0, sizeof(puck_game->remote_dash));
}

void puck_game_reset_tutorial_state(LWPUCKGAME* puck_game) {
    puck_game->update_tick = 0;
    puck_game->prepare_step_waited_tick = 0;
    puck_game->battle_phase = LSP_TUTORIAL;
    for (int i = 0; i < LW_PUCK_GAME_TOWER_COUNT; i++) {
        puck_game->tower[i].hp = puck_game->tower_total_hp;
        puck_game->tower[i].collapsing = 0;
    }
    // tutorial starts with an empty scene
    puck_game_destroy_all_battle_objects(puck_game);
    puck_game->pg_player[0].total_hp = puck_game->hp;
    puck_game->pg_player[0].current_hp = puck_game->hp;
    puck_game->pg_target[0].total_hp = puck_game->hp;
    puck_game->pg_target[0].current_hp = puck_game->hp;
    memset(puck_game->remote_control, 0, sizeof(puck_game->remote_control));
    puck_game->control_flags |= LPGCF_HIDE_TIMER;
    // reset dash state
    memset(puck_game->remote_dash, 0, sizeof(puck_game->remote_dash));
}

void puck_game_reset(LWPUCKGAME* puck_game) {
    puck_game_reset_battle_state(puck_game);
    puck_game->game_state = LPGS_MAIN_MENU;
    puck_game->world_roll = (float)LWDEG2RAD(180);
    puck_game->world_roll_dir = 1;
    puck_game->world_roll_axis = 1;
    puck_game->world_roll_target = puck_game->world_roll;
    puck_game->world_roll_target_follow_ratio = 0.075f;
}

LWPUCKGAMEPLAYER* puck_game_player(LWPUCKGAME* puck_game, int index) {
    return &puck_game->pg_player[index];
}

LWPUCKGAMEPLAYER* puck_game_target(LWPUCKGAME* puck_game, int index) {
    return &puck_game->pg_target[index];
}

void puck_game_set_show_top_level_main_menu(LWPUCKGAME* puck_game, int show) {
    puck_game->show_top_level_main_menu = show;
}

void puck_game_set_show_htmlui(LWPUCKGAME* puck_game, int show) {
    puck_game->show_html_ui = show;
}

int puck_game_leaderboard_items_in_page(float aspect_ratio) {
    if (aspect_ratio > 1) {
        return LW_LEADERBOARD_ITEMS_IN_PAGE;
    } else {
        return LW_LEADERBOARD_ITEMS_IN_PAGE - 3;
    }
}

static void fill_record_gameobject(LWPUCKGAMERECORDFRAMEGAMEOBJECT* record_frame_go,
                            const LWPUCKGAME* puck_game,
                            const LWPUCKGAMEOBJECT* go) {
    dCopyVector3(record_frame_go->pos, dBodyGetPosition(go->body));
    dCopyVector3(record_frame_go->linvel, dBodyGetLinearVel(go->body));
    dCopyVector4(record_frame_go->quat, dBodyGetQuaternion(go->body));
    dCopyVector3(record_frame_go->angularvel, dBodyGetAngularVel(go->body));
}

void puck_game_fill_state_bitfield(LWPSTATEBITFIELD* bf, const LWPUCKGAME* puck_game, const LWPUCKGAMEPLAYER* player, const LWPUCKGAMEPLAYER* target, const int* wall_hit_bit) {
    bf->player_current_hp = (unsigned int)player->current_hp;
    bf->player_total_hp = (unsigned int)player->total_hp;
    bf->target_current_hp = (unsigned int)target->current_hp;
    bf->target_total_hp = (unsigned int)target->total_hp;
    bf->puck_owner_player_no = (unsigned int)puck_game->puck_owner_player_no;
    bf->phase = (unsigned int)puck_game->battle_phase;
    bf->player_pull = (unsigned int)puck_game->remote_control[LW_PUCK_GAME_PLAYER_TEAM][0].pull_puck;
    bf->target_pull = (unsigned int)puck_game->remote_control[LW_PUCK_GAME_TARGET_TEAM][0].pull_puck;
    bf->wall_hit_bit = (unsigned int)*wall_hit_bit;
}

void puck_game_on_new_record_frame(const LWPUCKGAME* puck_game, LWPUCKGAMERECORD* record, unsigned short update_tick) {
    if (puck_game == 0 || record == 0) {
        LOGEP("puck game record: required pointer null");
        return;
    }
    if (record->num_frame != update_tick) {
        LOGEP("puck game record: num frame skipped?");
        return;
    }
    if (record->num_frame >= ARRAY_SIZE(record->frames)) {
        LOGEP("puck game record: maximum capacity exceeded");
        return;
    }
    LWPUCKGAMERECORDFRAME* f = &record->frames[record->num_frame];
    f->frame = record->num_frame;
    fill_record_gameobject(&f->go[0], puck_game, &puck_game->go[LPGO_PUCK]);
    fill_record_gameobject(&f->go[1], puck_game, &puck_game->go[LPGO_PLAYER]);
    fill_record_gameobject(&f->go[2], puck_game, &puck_game->go[LPGO_TARGET]);
    memcpy(&f->control[0], &puck_game->remote_control[0][0], sizeof(LWPUCKGAMERECORDFRAMECONTROL));
    memcpy(&f->control[1], &puck_game->remote_control[1][0], sizeof(LWPUCKGAMERECORDFRAMECONTROL));
    const LWPUCKGAMEPLAYER* player = &puck_game->pg_player[0];
    const LWPUCKGAMEPLAYER* target = &puck_game->pg_target[0];
    const int* wall_hit_bit = &puck_game->wall_hit_bit_send_buf_1;
    puck_game_fill_state_bitfield(&f->bf, puck_game, player, target, wall_hit_bit);
    f->dash_elapsed_since_last[0] = puck_game_dash_elapsed_since_last(puck_game, &puck_game->remote_dash[0][0]);
    f->dash_elapsed_since_last[1] = puck_game_dash_elapsed_since_last(puck_game, &puck_game->remote_dash[1][0]);
    record->num_frame++;
}

void puck_game_on_finalize_record(const LWPUCKGAME* puck_game, const LWPUCKGAMERECORD* record) {
    const size_t compressed_bound_size = LZ4_COMPRESSBOUND(sizeof(LWPUCKGAMERECORD));
    char* compressed = malloc(compressed_bound_size);
    int compressed_size = LZ4_compress_default((char*)record, compressed, sizeof(LWPUCKGAMERECORD), compressed_bound_size);
    if (compressed_size > 0) {
        char dt[128];
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        strftime(dt, sizeof(dt) - 1, "%Y_%m_%d_%H_%M_%S", t);
        char filename[256];
        snprintf(filename, sizeof(filename), "%s-%05d.dat", dt, puck_game->battle_id);
        FILE* f = fopen(filename, "wb");
        fwrite(compressed, compressed_size, 1, f);
        fclose(f);
        LOGI("puck game record: record file '%s' written (%d bytes)", filename, compressed_size);
    } else {
        LOGEP("puck game record: finalized failed! (compressed size == %d)", compressed_size);
    }
    free(compressed);
}
