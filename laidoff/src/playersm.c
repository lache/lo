#include "playersm.h"
#include "lwmacro.h"
#include "lwlog.h"
#include "field.h"
#include "mq.h"
#include "laidoff.h"

static int s_verbose = 0;

static void s_fire_bullet(LWPLAYERSTATEDATA* data);

typedef LW_PLAYER_STATE STATE_FUNC(LWPLAYERSTATEDATA* data);

static LW_PLAYER_STATE s_do_state_idle(LWPLAYERSTATEDATA* data) {
    data->skin_time += data->delta_time;
    // Start moving
    if (data->dir) {
        return LPS_MOVE;
    }
    // Start aiming
    if (data->atk) {
        return LPS_AIM;
    }
    return LPS_IDLE;
}

static LW_PLAYER_STATE s_do_state_move(LWPLAYERSTATEDATA* data) {
    data->skin_time += data->delta_time;
    // Stop moving
    if (!data->dir) {
        return LPS_IDLE;
    }
    // Start aiming
    if (data->atk) {
        return LPS_AIM;
    }
    return LPS_MOVE;
}

static LW_PLAYER_STATE s_do_state_aim(LWPLAYERSTATEDATA* data) {
    data->skin_time += data->delta_time;
    // Fire!
    if (!data->atk) {
        return LPS_FIRE;
    }
    // Increase aiming precision
    data->aim_theta = LWMAX((float)LWDEG2RAD(5), data->aim_theta + data->aim_theta_speed * data->delta_time);
    return LPS_AIM;
}

static LW_PLAYER_STATE s_do_state_fire(LWPLAYERSTATEDATA* data) {
    data->skin_time += data->delta_time;
    // Start moving
    if (data->dir) {
        return LPS_MOVE;
    }
    // Start aiming
    if (data->atk) {
        return LPS_AIM;
    }
    // Finished fire anim
    if (data->animfin) {
        return LPS_UNAIM;
    }
    return LPS_FIRE;
}

static LW_PLAYER_STATE s_do_state_unaim(LWPLAYERSTATEDATA* data) {
    data->skin_time += data->delta_time;
    // Start moving
    if (data->dir) {
        return LPS_IDLE;
    }
    // Start aiming
    if (data->atk) {
        return LPS_AIM;
    }
    // Finished unaim anim
    if (data->animfin) {
        return LPS_IDLE;
    }
    return LPS_UNAIM;
}

static STATE_FUNC* const s_state[LPS_COUNT] = {
    s_do_state_idle,
    s_do_state_move,
    s_do_state_aim,
    s_do_state_fire,
    s_do_state_unaim,
};

static void s_on_enter_aim(LWPLAYERSTATEDATA* data);
static void s_on_exit_aim(LWPLAYERSTATEDATA* data);
static void s_on_enter_idle(LWPLAYERSTATEDATA* data);

static void s_do_idle_to_move(LWPLAYERSTATEDATA* data) {
    if (s_verbose) { LOGI("s_do_idle_to_move"); }
    data->skin_time = 0;
}

static void s_do_move_to_idle(LWPLAYERSTATEDATA* data) {
    if (s_verbose) { LOGI("s_do_move_to_idle"); }
    data->skin_time = 0;
    s_on_enter_idle(data);
}

static void s_do_move_to_aim(LWPLAYERSTATEDATA* data) {
    if (s_verbose) { LOGI("s_do_move_to_aim"); }
    data->skin_time = 0;
    s_on_enter_aim(data);
}

static void s_do_idle_to_aim(LWPLAYERSTATEDATA* data) {
    if (s_verbose) { LOGI("s_do_idle_to_aim"); }
    data->skin_time = 0;
    s_on_enter_aim(data);
}

static void s_do_aim_to_idle(LWPLAYERSTATEDATA* data) {
    if (s_verbose) { LOGI("s_do_aim_to_idle"); }
    data->skin_time = 0;
    s_on_exit_aim(data);
    s_on_enter_idle(data);
}

static void s_do_aim_to_fire(LWPLAYERSTATEDATA* data) {
    if (s_verbose) { LOGI("s_do_aim_to_fire"); }
    data->skin_time = 0;
    if (s_verbose) { LOGI("Fire!"); }
    s_fire_bullet(data);
    s_on_exit_aim(data);
}

static void s_do_fire_to_unaim(LWPLAYERSTATEDATA* data) {
    if (s_verbose) { LOGI("s_do_fire_to_unaim"); }
    data->skin_time = 0;
}

static void s_do_fire_to_move(LWPLAYERSTATEDATA* data) {
    if (s_verbose) { LOGI("s_do_fire_to_move"); }
    data->skin_time = 0;
}

static void s_do_fire_to_aim(LWPLAYERSTATEDATA* data) {
    if (s_verbose) { LOGI("s_do_fire_to_aim"); }
    data->skin_time = data->aim_last_skin_time; // Start with aim finished pose
    s_on_enter_aim(data);
}

static void s_do_unaim_to_idle(LWPLAYERSTATEDATA* data) {
    if (s_verbose) { LOGI("s_do_unaim_to_idle"); }
    data->skin_time = 0;
    s_on_enter_idle(data);
}

static void s_do_unaim_to_aim(LWPLAYERSTATEDATA* data) {
    if (s_verbose) { LOGI("s_do_unaim_to_aim"); }
    data->skin_time = 0;//
    s_on_enter_aim(data);
}


typedef void TRANSITION_FUNC(LWPLAYERSTATEDATA* data);

static TRANSITION_FUNC* const s_transition[LPS_COUNT][LPS_COUNT] = {
    //          IDLE                    MOVE                    AIM                     FIRE                UNAIM
    /* IDLE */  { 0,                    s_do_idle_to_move,      s_do_idle_to_aim,       0,                  0,                  },
    /* MOVE */  { s_do_move_to_idle,    0,                      s_do_move_to_aim,       0,                  0,                  },
    /* AIM */   { s_do_aim_to_idle,     0,                      0,                      s_do_aim_to_fire,   0,                  },
    /* FIRE */  { 0,                    s_do_fire_to_move,      s_do_fire_to_aim,       0,                  s_do_fire_to_unaim  },
    /* UNAIM*/  { s_do_unaim_to_idle,   0,                      s_do_unaim_to_aim,      0,                  0,                  },
};

LW_PLAYER_STATE run_state(LW_PLAYER_STATE cur_state, LWPLAYERSTATEDATA* data) {
    LW_PLAYER_STATE new_state = s_state[cur_state](data);
    TRANSITION_FUNC* transition = s_transition[cur_state][new_state];
    if (transition) {
        transition(data);
        // TODO Should we call 'mq_publish_now()' here?
        if (data->mq) {
            mq_send_action(data->mq, get_anim_by_state(new_state, 0));
        }
    }
    return new_state;
}

static const struct {
    LW_ACTION action;
    int loop;
} S_ANIM_BY_STATE[LPS_COUNT] = {
    { LWAC_HUMANACTION_IDLE,        1 },
    { LWAC_HUMANACTION_WALKPOLISH,  1 },
    { LWAC_HUMANACTION_STAND_AIM,   0 },
    { LWAC_HUMANACTION_STAND_FIRE,  0 },
    { LWAC_HUMANACTION_STAND_UNAIM, 0 },
};

LW_ACTION get_anim_by_state(LW_PLAYER_STATE cur_state, int* loop) {
    if (loop) {
        *loop = S_ANIM_BY_STATE[cur_state].loop;
    }
    return S_ANIM_BY_STATE[cur_state].action;
}

static void s_on_enter_aim(LWPLAYERSTATEDATA* data) {
    field_set_aim_sector_ray(data->field, 1);
    data->aim_theta = (float)(M_PI / 4);
    data->aim_theta_speed = -(float)LWDEG2RAD(90);
}

static void s_on_exit_aim(LWPLAYERSTATEDATA* data) {
}

static void s_on_enter_idle(LWPLAYERSTATEDATA* data) {
    field_set_aim_sector_ray(data->field, 0);
}

static void s_fire_bullet(LWPLAYERSTATEDATA* data) {
    float pos[3];
    get_field_player_geom_position(data->field, pos + 0, pos + 1, pos + 2);
    float speed = 70.0f;
    
    // random range around rot_z: [rot_z - data->aim_theta / 2, rot_z + data->aim_theta / 2)
    const double rd = field_random_double(data->field);
    //LOGI("rd = %f", rd);
    const float aimed_rot_z = (float)(data->rot_z - data->aim_theta / 2 + data->aim_theta * rd);
    
    float vel[3] = { speed * cosf(aimed_rot_z), speed * sinf(aimed_rot_z), 0 };
    if (field_network(data->field)) {
        mq_send_fire(data->mq, pos, vel);
    } else {
        field_spawn_sphere(data->field, pos, vel, mq_bullet_counter(data->mq));

        /*spawn_field_object(data->field, pos[0], pos[1], 1, 1, LVT_PUMP, data->pLwc->tex_programmed[LPT_SOLID_RED],
            1, 1, 0.5f, 0, 1);*/
    }
}
