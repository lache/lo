#include "logic.h"
#include <czmq.h>
#include "lwcontext.h"
#include "lwtimepoint.h"
#include "lwlog.h"
#include "mq.h"
#include "script.h"
#include "net.h"
#include "dialog.h"
#include "battle.h"
#include "sysmsg.h"
#include "render_font_test.h"
#include "render_ttl.h"
#include "battle_result.h"
#include "file.h"
#include "ps.h"
#include "script.h"
#include "nav.h"
#include "laidoff.h"
#include "lwbutton.h"
#include "puckgameupdate.h"
#include "lwudp.h"
#include "lwtcp.h"
#include "lwtcpclient.h"
#include "lwime.h"
#include "puckgame.h"
#include "sound.h"
#include "lwttl.h"
#include "rmsg.h"
#include "htmlui.h"
#include "remtex.h"

void toggle_font_texture_test_mode(LWCONTEXT* pLwc);
void lw_request_remote_notification_device_token(LWCONTEXT* pLwc);
void lw_start_reward_video(LWCONTEXT* pLwc, int tag);
void lw_start_sign_in(LWCONTEXT* pLwc, int tag);
void lw_start_sign_out(LWCONTEXT* pLwc, int tag);

static const char* server_addr[] = {
        "s.popsongremix.com", // AWS Tokyo
        "222.110.4.119", // private dev
};

static void reinit_mq(LWCONTEXT* pLwc);
void load_field_2_init_runtime_data_async(LWCONTEXT* pLwc, zactor_t* actor);

void change_to_field(LWCONTEXT* pLwc) {
    if (pLwc->field) {
        pLwc->next_game_scene = LGS_FIELD;
    } else {
        LOGE("pLwc->field null");
        load_field_2_init_runtime_data_async(pLwc, pLwc->logic_actor);
    }
}

void change_to_dialog(LWCONTEXT* pLwc) {
    pLwc->next_game_scene = LGS_DIALOG;
}

void change_to_battle(LWCONTEXT* pLwc) {
    pLwc->next_game_scene = LGS_BATTLE;
}

void change_to_font_test(LWCONTEXT* pLwc) {
    pLwc->next_game_scene = LGS_FONT_TEST;
    lwfbo_init(&pLwc->shared_fbo, pLwc->window_width, pLwc->window_height);
    // Render font FBO using render-to-texture
    lwc_render_font_test_fbo(pLwc);
}

void change_to_ttl(LWCONTEXT* pLwc) {
    pLwc->next_game_scene = LGS_TTL;
    lwfbo_init(&pLwc->shared_fbo, pLwc->window_width, pLwc->window_height);
    // Render font FBO using render-to-texture
    lwc_render_ttl_fbo(pLwc, ASSETS_BASE_PATH "html" PATH_SEPARATOR "HTMLPage1.html");
}

void change_to_admin(LWCONTEXT* pLwc) {
    pLwc->next_game_scene = LGS_ADMIN;
}

void change_to_battle_result(LWCONTEXT* pLwc) {
    pLwc->next_game_scene = LGS_BATTLE_RESULT;
}

void change_to_skin(LWCONTEXT* pLwc) {
    pLwc->next_game_scene = LGS_SKIN;
}

void change_to_puck_game(LWCONTEXT* pLwc) {
    if (pLwc->tcp == 0) {
        pLwc->tcp = new_tcp(pLwc,
                            pLwc->user_data_path,
                            &pLwc->tcp_host_addr,
                            tcp_on_connect,
                            parse_recv_packets);
    }
    pLwc->next_game_scene = LGS_PUCK_GAME;
    lwfbo_init(&pLwc->shared_fbo, pLwc->window_width, pLwc->window_height);
}

void change_to_particle_system(LWCONTEXT* pLwc) {
    pLwc->next_game_scene = LGS_PARTICLE_SYSTEM;
}

void change_to_ui(LWCONTEXT* pLwc) {
    pLwc->next_game_scene = LGS_UI;
}

void change_to_splash(LWCONTEXT* pLwc) {
    pLwc->next_game_scene = LGS_SPLASH;
}

void change_to_leaderboard(LWCONTEXT* pLwc) {
    pLwc->next_game_scene = LGS_LEADERBOARD;
}

void change_to_remtex(LWCONTEXT* pLwc) {
    pLwc->next_game_scene = LGS_REMTEX;
}

typedef enum _LW_MSG {
    LM_ZERO,
    LM_LWMSGINITFIELD,
    LM_LWMSGRESETRUNTIMECONTEXT,
    LM_LWMSGRELOADSCRIPT,
    LM_LWMSGSTARTLOGICLOOP,
    LM_LWMSGSTOPLOGICLOOP,
    LM_LWMSGINITSCENE,
    LM_LWMSGUIEVENT,
    LM_LWMSGEVALUATE,
} LW_MSG;

typedef struct _LWMSGINITFIELD {
    LW_MSG type;
    const char* field_filename;
    const char* nav_filename;
    int field_mesh_count;
    LWFIELDMESH field_mesh[4];
    float skin_scale;
    int follow_cam;
} LWMSGINITFIELD;

typedef struct _LWMSGRESETRUNTIMECONTEXT {
    LW_MSG type;
} LWMSGRESETRUNTIMECONTEXT;

typedef struct _LWMSGRELOADSCRIPT {
    LW_MSG type;
} LWMSGRELOADSCRIPT;

typedef struct _LWMSGSTARTLOGICLOOP {
    LW_MSG type;
} LWMSGSTARTLOGICLOOP;

typedef struct _LWMSGSTOPLOGICLOOP {
    LW_MSG type;
} LWMSGSTOPLOGICLOOP;

typedef struct _LWMSGINITSCENE {
    LW_MSG type;
} LWMSGINITSCENE;

typedef struct _LWMSGUIEVENT {
    LW_MSG type;
    char id[LW_UI_IDENTIFIER_LENGTH];
    float w_ratio;
    float h_ratio;
} LWMSGUIEVENT;

typedef struct _LWMSGEVALUATE {
    LW_MSG type;
} LWMSGEVALUATE;

typedef struct _LWMSGREDRAWUIFBO {
    LW_MSG type;
} LWMSGREDRAWUIFBO;

void logic_start_logic_update_job_async(LWCONTEXT* pLwc) {
    if (pLwc == 0 || pLwc->logic_actor == 0) {
        return;
    }
    zmsg_t* msg = zmsg_new();
    LWMSGSTARTLOGICLOOP m = {
            LM_LWMSGSTARTLOGICLOOP,
    };
    zmsg_addmem(msg, &m, sizeof(LWMSGSTARTLOGICLOOP));
    if (zactor_send(pLwc->logic_actor, &msg) < 0) {
        zmsg_destroy(&msg);
        LOGE("Send message to logic worker failed!");
    }
}

void logic_stop_logic_update_job_async(LWCONTEXT* pLwc) {
    if (pLwc == 0 || pLwc->logic_actor == 0) {
        return;
    }
    zmsg_t* msg = zmsg_new();
    LWMSGSTOPLOGICLOOP m = {
            LM_LWMSGSTOPLOGICLOOP,
    };
    zmsg_addmem(msg, &m, sizeof(LWMSGSTOPLOGICLOOP));
    if (zactor_send(pLwc->logic_actor, &msg) < 0) {
        zmsg_destroy(&msg);
        LOGE("Send message to logic worker failed!");
    }
}

void load_field_1_init_runtime_data_async(LWCONTEXT* pLwc, zactor_t* actor) {
    pLwc->next_game_scene = LGS_FIELD;
    // should load stage texture here
    const int lae = LAE_3D_APT_TEX_MIP_KTX;
    lw_load_tex(pLwc, lae);
    zmsg_t* msg = zmsg_new();
    LWMSGINITFIELD m = {
            LM_LWMSGINITFIELD,
            ASSETS_BASE_PATH "field" PATH_SEPARATOR "testfield.field",
            ASSETS_BASE_PATH "nav" PATH_SEPARATOR "apt.nav",
            1, {
                    { LVT_APT, pLwc->tex_atlas[lae], 1, },
            },
            0.9f,
            0,
    };
    zmsg_addmem(msg, &m, sizeof(LWMSGINITFIELD));
    if (zactor_send(actor, &msg) < 0) {
        zmsg_destroy(&msg);
        LOGE("Send message to logic worker failed!");
    }
}

void load_field_1_init_runtime_data(LWCONTEXT* pLwc) {
    load_field_1_init_runtime_data_async(pLwc, pLwc->logic_actor);
}

void load_field_2_init_runtime_data_async(LWCONTEXT* pLwc, zactor_t* actor) {
    pLwc->next_game_scene = LGS_FIELD;
    // should load stage texture here
    const int lae = LAE_3D_FLOOR_TEX_KTX;
    lw_load_tex(pLwc, lae);
    zmsg_t* msg = zmsg_new();
    LWMSGINITFIELD m = {
            LM_LWMSGINITFIELD,
            ASSETS_BASE_PATH "field" PATH_SEPARATOR "testfield.field",
            ASSETS_BASE_PATH "nav" PATH_SEPARATOR "test.nav",
            1, {
                    { LVT_FLOOR, pLwc->tex_atlas[lae], 0, },
            },
            0.35f,
            1,
    };
    zmsg_addmem(msg, &m, sizeof(LWMSGINITFIELD));
    if (zactor_send(actor, &msg) < 0) {
        zmsg_destroy(&msg);
        LOGE("Send message to logic worker failed!");
    }
}

void load_field_2_init_runtime_data(LWCONTEXT* pLwc) {
    load_field_2_init_runtime_data_async(pLwc, pLwc->logic_actor);
}

void load_field_3_init_runtime_data_async(LWCONTEXT* pLwc, zactor_t* actor) {
    pLwc->next_game_scene = LGS_FIELD;
    // should load stage texture here
    const int lae = LAE_3D_FLOOR2_TEX_KTX;
    lw_load_tex(pLwc, lae);
    zmsg_t* msg = zmsg_new();
    LWMSGINITFIELD m = {
            LM_LWMSGINITFIELD,
            ASSETS_BASE_PATH "field" PATH_SEPARATOR "testfield2.field",
            ASSETS_BASE_PATH "nav" PATH_SEPARATOR "test2.nav",
            1, {
                    { LVT_FLOOR2, pLwc->tex_atlas[lae], 0, },
            },
            0.35f,
            1,
    };
    zmsg_addmem(msg, &m, sizeof(LWMSGINITFIELD));
    if (zactor_send(actor, &msg) < 0) {
        zmsg_destroy(&msg);
        LOGE("Send message to logic worker failed!");
    }
}

void load_field_3_init_runtime_data(LWCONTEXT* pLwc) {
    load_field_3_init_runtime_data_async(pLwc, pLwc->logic_actor);
}

void load_field_4_init_runtime_data_async(LWCONTEXT* pLwc, zactor_t* actor) {
    pLwc->next_game_scene = LGS_FIELD;
    // should load stage texture here
    const int lae = LAE_3D_ROOM_TEX_KTX;
    lw_load_tex(pLwc, lae);
    zmsg_t* msg = zmsg_new();
    LWMSGINITFIELD m = {
            LM_LWMSGINITFIELD,
            ASSETS_BASE_PATH "field" PATH_SEPARATOR "room.field",
            ASSETS_BASE_PATH "nav" PATH_SEPARATOR "room.nav",
            1, {
                    { LVT_ROOM, pLwc->tex_atlas[lae], 0, },
            },
            0.35f,
            1,
    };
    zmsg_addmem(msg, &m, sizeof(LWMSGINITFIELD));
    if (zactor_send(actor, &msg) < 0) {
        zmsg_destroy(&msg);
        LOGE("Send message to logic worker failed!");
    }
}

void load_field_4_init_runtime_data(LWCONTEXT* pLwc) {
    load_field_4_init_runtime_data_async(pLwc, pLwc->logic_actor);
}

void load_field_5_init_runtime_data_async(LWCONTEXT* pLwc, zactor_t* actor) {
    pLwc->next_game_scene = LGS_FIELD;
    // should load stage texture here
    const int lae1 = LAE_3D_BATTLEGROUND_FLOOR_BAKE_TEX_KTX;
    lw_load_tex(pLwc, lae1);
    const int lae2 = LAE_3D_BATTLEGROUND_WALL_BAKE_TEX_KTX;
    lw_load_tex(pLwc, lae1);
    zmsg_t* msg = zmsg_new();
    LWMSGINITFIELD m = {
            LM_LWMSGINITFIELD,
            ASSETS_BASE_PATH "field" PATH_SEPARATOR "battleground.field",
            ASSETS_BASE_PATH "nav" PATH_SEPARATOR "battleground.nav",
            2, {
                    { LVT_BATTLEGROUND_FLOOR, pLwc->tex_atlas[lae1], 0, },
                    { LVT_BATTLEGROUND_WALL, pLwc->tex_atlas[lae2], 0, },
            },
            0.35f,
            1,
    };
    zmsg_addmem(msg, &m, sizeof(LWMSGINITFIELD));
    if (zactor_send(actor, &msg) < 0) {
        zmsg_destroy(&msg);
        LOGE("Send message to logic worker failed!");
    }
}

void load_field_5_init_runtime_data(LWCONTEXT* pLwc) {
    load_field_5_init_runtime_data_async(pLwc, pLwc->logic_actor);
}

void load_field_6_init_runtime_data_async(LWCONTEXT* pLwc, zactor_t* actor) {
    pLwc->next_game_scene = LGS_FIELD;
    zmsg_t* msg = zmsg_new();
    // should load stage texture here
    const int lae = LAE_SPIRAL_KTX;
    lw_load_tex(pLwc, lae);
    LWMSGINITFIELD m = {
            LM_LWMSGINITFIELD,
            ASSETS_BASE_PATH "field" PATH_SEPARATOR "spiral.field",
            ASSETS_BASE_PATH "nav" PATH_SEPARATOR "spiral.nav",
            1,{
                    { LVT_SPIRAL, pLwc->tex_atlas[lae], 0, },
            },
            0.35f,
            1,
    };
    zmsg_addmem(msg, &m, sizeof(LWMSGINITFIELD));
    if (zactor_send(actor, &msg) < 0) {
        zmsg_destroy(&msg);
        LOGE("Send message to logic worker failed!");
    }
}

void load_field_6_init_runtime_data(LWCONTEXT* pLwc) {
    load_field_6_init_runtime_data_async(pLwc, pLwc->logic_actor);
}

void load_scene_async(LWCONTEXT* pLwc, zactor_t* actor, void next_game_scene_changer(LWCONTEXT*)) {
    zmsg_t* msg = zmsg_new();
    LWMSGINITSCENE m = {
            LM_LWMSGINITSCENE,
    };
    zmsg_addmem(msg, &m, sizeof(LWMSGINITSCENE));
    if (zactor_send(actor, &msg) < 0) {
        zmsg_destroy(&msg);
        LOGE("Send message to logic worker failed!");
    }
    next_game_scene_changer(pLwc);
}

void logic_emit_ui_event_async(LWCONTEXT* pLwc, const char* id, float w_ratio, float h_ratio) {
    if (strlen(id) > LW_UI_IDENTIFIER_LENGTH - 1) {
        LOGEP("id('%s') length exceeds (max %d)", id, LW_UI_IDENTIFIER_LENGTH - 1);
        return;
    }
    zmsg_t* msg = zmsg_new();
    LWMSGUIEVENT m;
    m.type = LM_LWMSGUIEVENT;
    strcpy(m.id, id);
    m.w_ratio = w_ratio;
    m.h_ratio = h_ratio;
    zmsg_addmem(msg, &m, sizeof(LWMSGUIEVENT));
    zactor_t* actor = pLwc->logic_actor;
    if (zactor_send(actor, &msg) < 0) {
        zmsg_destroy(&msg);
        LOGE("Send message to logic worker failed!");
    }
}

void reset_runtime_context_async(LWCONTEXT* pLwc) {
    zmsg_t* msg = zmsg_new();
    LWMSGRESETRUNTIMECONTEXT m = {
            LM_LWMSGRESETRUNTIMECONTEXT,
    };
    zmsg_addmem(msg, &m, sizeof(LWMSGRESETRUNTIMECONTEXT));
    if (zactor_send(pLwc->logic_actor, &msg) < 0) {
        zmsg_destroy(&msg);
        LOGE("Send message to logic worker failed!");
    }
}

void logic_emit_evalute_with_name_async(LWCONTEXT* pLwc, const char* code, size_t code_len, const char* name) {
    if (code == 0 || code_len <= 0) {
        LOGE("Invalid script to evaluate");
        return;
    }
    zmsg_t* msg = zmsg_new();
    LWMSGEVALUATE m;
    m.type = LM_LWMSGEVALUATE;
    zmsg_addmem(msg, &m, sizeof(LWMSGEVALUATE));
    zmsg_addmem(msg, code, code_len);
    zmsg_addmem(msg, name, strlen(name) + 1); // should include trailing NULL
    zactor_t* actor = pLwc->logic_actor;
    if (zactor_send(actor, &msg) < 0) {
        zmsg_destroy(&msg);
        LOGE("Send message to logic worker failed!");
    }
}

void logic_emit_evalute_async(LWCONTEXT* pLwc, const char* code, size_t code_len) {
    logic_emit_evalute_with_name_async(pLwc, code, code_len, SCRIPT_UNNAMED_SCRIPT_NAME);
}

static void reinit_mq(LWCONTEXT* pLwc) {

    //mq_interrupt();

    deinit_mq(pLwc->mq);

    pLwc->mq = init_mq(server_addr[pLwc->server_index], pLwc->def_sys_msg);
}

void connect_to_server_0(LWCONTEXT* pLwc) {
    pLwc->server_index = 0;

    reinit_mq(pLwc);
}

void connect_to_server_1(LWCONTEXT* pLwc) {
    pLwc->server_index = 1;

    reinit_mq(pLwc);
}

void reset_time(LWCONTEXT* pLwc) {
    pLwc->player_skin_time = 0;
}

void set_creature_data(LWBATTLECREATURE* c, const char* name, int lv, int hp, int max_hp, int mp, int max_mp,
                       int turn_token, int exp, int max_exp) {
    c->valid = 1;
    //strcpy(c->name, name);
    c->lv = lv;
    c->hp = hp;
    c->max_hp = max_hp;
    c->mp = mp;
    c->max_mp = max_hp;
    c->selected_r = 0.5f;
    c->selected_g = 0.5f;
    c->selected_b = 0.5f;
    c->selected_a = 0.5f;
    c->turn_token = turn_token;
    c->exp = exp;
    c->max_exp = max_exp;
}

void reset_battle_context(LWCONTEXT* pLwc) {
    pLwc->battle_fov_deg_0 = 49.134f;
    pLwc->battle_fov_mag_deg_0 = 35.134f;
    pLwc->battle_fov_deg = pLwc->battle_fov_deg_0;
    pLwc->selected_enemy_slot = -1;

    const int enemy_count = LWMIN(pLwc->field_event_id, MAX_ENEMY_SLOT);
    // Fill enemy slots with test enemies
    for (int i = 0; i < enemy_count; i++) {
        pLwc->enemy[i] = ENEMY_DATA_LIST[i];

        for (int j = 0; j < MAX_SKILL_PER_CREATURE; j++) {
            pLwc->enemy[i].c.skill[j] = &SKILL_DATA_LIST[j];
        }
    }
    // Invalidate empty enemy slot
    for (int i = enemy_count; i < MAX_ENEMY_SLOT; i++) {
        pLwc->enemy[i].valid = 0;
    }

    memset(pLwc->player, 0, sizeof(pLwc->player));

    for (int i = 0; i < MAX_PLAYER_SLOT; i++) {
        pLwc->player[i] = ENEMY_DATA_LIST[LET_TEST_PLAYER_1 + i].c;
        pLwc->player[i].valid = 0;
    }

    set_creature_data(
        &pLwc->player[0],
        "Player",
        1,
        50,
        50,
        30,
        30,
        1,
        5,
        38
    );
    pLwc->player[0].stat = ENEMY_DATA_LIST[0].c.stat;
    pLwc->player[0].skill[0] = &SKILL_DATA_LIST[0];
    pLwc->player[0].skill[1] = &SKILL_DATA_LIST[1];
    pLwc->player[0].skill[2] = &SKILL_DATA_LIST[2];
    pLwc->player[0].skill[3] = &SKILL_DATA_LIST[3];
    pLwc->player[0].skill[4] = &SKILL_DATA_LIST[4];
    pLwc->player[0].skill[5] = &SKILL_DATA_LIST[5];

    set_creature_data(
        &pLwc->player[1],
        u8"Screenwriter",
        2,
        25,
        50,
        30,
        30,
        2,
        1,
        30
    );
    pLwc->player[1].stat = ENEMY_DATA_LIST[0].c.stat;
    pLwc->player[1].skill[0] = &SKILL_DATA_LIST[0];
    pLwc->player[1].skill[1] = &SKILL_DATA_LIST[2];

    set_creature_data(
        &pLwc->player[2],
        u8"Nail Clipper",
        3,
        46,
        46,
        15,
        55,
        3,
        0,
        50
    );
    pLwc->player[2].stat = ENEMY_DATA_LIST[0].c.stat;
    pLwc->player[2].skill[0] = &SKILL_DATA_LIST[0];
    pLwc->player[2].skill[1] = &SKILL_DATA_LIST[3];

    set_creature_data(
        &pLwc->player[3],
        u8"Great Writer",
        8,
        105,
        105,
        0,
        20,
        4,
        45,
        56
    );
    pLwc->player[3].stat = ENEMY_DATA_LIST[0].c.stat;
    pLwc->player[3].skill[0] = &SKILL_DATA_LIST[0];
    pLwc->player[3].skill[1] = &SKILL_DATA_LIST[4];

    for (int i = 0; i < MAX_PLAYER_SLOT; i++) {
        pLwc->player[i].selected = 0;
    }

    pLwc->player[0].selected = 1;

    pLwc->player_turn_creature_index = 0;

    pLwc->battle_state = LBS_SELECT_COMMAND;

    pLwc->selected_command_slot = 0;
}

void reset_field_context(LWCONTEXT* pLwc) {
    // Reset random generator seed
    field_reset_deterministic_seed(pLwc->field);
    // Nav instance shortcut
    LWNAV* nav = field_nav(pLwc->field);
    // Despawn all field objects
    despawn_all_field_object(pLwc->field);
    // Spawn all initial field objects
    spawn_all_field_object(pLwc);
    // Player initially should fall from the sky for stable ray checking
    set_field_player_position(pLwc->field, 0, 0, 10);
    // Create all field cube colliders
    field_create_field_box_collider(pLwc->field);
    // Destroy all script colliders
    field_destroy_all_script_colliders(pLwc->field);
    // Reset player position
    pLwc->player_pos_x = 0;
    pLwc->player_pos_y = 0;
    // Reset nav context
    reset_nav_context(nav);
    // Delete all pending render messages
    delete_all_rmsgs(pLwc);
    // Clear render command
    memset(pLwc->render_command, 0, sizeof(pLwc->render_command));
}

void toggle_ray_test(LWCONTEXT* pLwc) {
    pLwc->ray_test = !pLwc->ray_test;

    field_enable_ray_test(pLwc->field, pLwc->ray_test);
}

void toggle_network_poll(LWCONTEXT* pLwc) {
    field_set_network(pLwc->field, !field_network(pLwc->field));
}

void toggle_stat(LWCONTEXT* pLwc) {
    pLwc->show_stat = !pLwc->show_stat;
}

void start_nickname_text_input_activity(LWCONTEXT* pLwc) {
    lw_start_text_input_activity(pLwc, LITI_NICKNAME);
}

void start_reward_video(LWCONTEXT* pLwc) {
    lw_start_reward_video(pLwc, 0);
}

void start_sign_in(LWCONTEXT* pLwc) {
    lw_start_sign_in(pLwc, 0);
}

void start_sign_out(LWCONTEXT* pLwc) {
    lw_start_sign_out(pLwc, 0);
}

void start_tcp_addr_text_input_activity(LWCONTEXT* pLwc) {
    lw_start_text_input_activity(pLwc, LITI_SERVER_ADDR);
}

void request_leaderboard(LWTCP* tcp, int items_in_page, int one_based_page) {
    tcp_send_get_leaderboard(tcp,
                             300,
                             (one_based_page - 1) * items_in_page,
                             items_in_page,
                             0);
}

void request_player_reveal_leaderboard(LWTCP* tcp, int items_in_page) {
    tcp_send_get_leaderboard_reveal_player(tcp,
                                           300,
                                           &tcp->user_id,
                                           items_in_page);
}

void start_request_push_token(LWCONTEXT* pLwc) {
    lw_request_remote_notification_device_token(pLwc);
}

void reset_runtime_context(LWCONTEXT* pLwc) {
    // Stop new frame of rendering
    lwcontext_set_safe_to_start_render(pLwc, 0);
    // Busy wait for current frame of rendering to be completed
    while (lwcontext_rendering(pLwc)) {}
    // Init lua
    init_lua(pLwc);
    // Reset time
    reset_time(pLwc);
    // Reset sprite data pointer
    pLwc->sprite_data = SPRITE_DATA[0];
    // Reset game scene
    //pLwc->next_game_scene = LGS_FIELD;
    // Reset debug font rendering indicator
    pLwc->font_texture_texture_mode = 0;
    // Reset battle context
    reset_battle_context(pLwc);
    // Reset field context
    if (pLwc->field) {
        reset_field_context(pLwc);
    }
    // Make render-to-texture flag dirty
    pLwc->shared_fbo.dirty = 1;
    // Register admin button commands
    const LWBUTTONCOMMAND handler_array[] = {
        { LWU("Field"), change_to_field },
        { LWU("Dialog"), change_to_dialog },
        { LWU("Battle"), change_to_battle },
        { LWU("Font"), change_to_font_test },
        { LWU("TTL"), change_to_ttl },
        { LWU("Runtime Reset"), reset_runtime_context_async },
        { LWU("Font Debug"), toggle_font_texture_test_mode },
        { LWU("UDP"), net_rtt_test },
        { LWU("Skin"), change_to_skin },
        { LWU("Puck Game"), change_to_puck_game },
        { LWU("Particles"), change_to_particle_system },
        { LWU("Field 1"), load_field_1_init_runtime_data },
        { LWU("Field 2"), load_field_2_init_runtime_data },
        { LWU("Field 3"), load_field_3_init_runtime_data },
        { LWU("Field 4"), load_field_4_init_runtime_data },
        { LWU("Field 5"), load_field_5_init_runtime_data },
        { LWU("Field 6"), load_field_6_init_runtime_data },
        { LWU("UI"), change_to_ui },
        { LWU("Splash"), change_to_splash },
        { LWU("Server #0"), connect_to_server_0 },
        { LWU("Server #1"), connect_to_server_1 },
        { LWU("Ray Test Toggle"), toggle_ray_test },
        { LWU("Network Toggle"), toggle_network_poll },
        { LWU("Nickname"), start_nickname_text_input_activity },
        { LWU("Push Token"), start_request_push_token },
        { LWU("TCP Address"), start_tcp_addr_text_input_activity },
        { LWU("Stat Toggle"), toggle_stat },
        { LWU("Reward Video"), start_reward_video },
        { LWU("Sign In"), start_sign_in },
        { LWU("Sign Out"), start_sign_out },
        { LWU("Remtex"), change_to_remtex },
    };
    assert(ARRAY_SIZE(handler_array) <= ARRAY_SIZE(pLwc->admin_button_command));
    for (int i = 0; i < ARRAY_SIZE(handler_array); i++) {
        pLwc->admin_button_command[i].name = handler_array[i].name;
        pLwc->admin_button_command[i].command_handler = handler_array[i].command_handler;
    }
    puck_game_reset(pLwc->puck_game);
    puck_game_remote_state_reset(pLwc->puck_game, &pLwc->puck_game_state);
    // Run script for testing script error logging function (no effects on system)
    script_run_file(pLwc, ASSETS_BASE_PATH "l" PATH_SEPARATOR "error_test.lua");
    // Run post init script
    script_run_file(pLwc, ASSETS_BASE_PATH "l" PATH_SEPARATOR "post_init.lua");
    // Start rendering
    lwcontext_set_safe_to_start_render(pLwc, 1);
}

static void update_battle_wall(LWCONTEXT* pLwc) {
    const float delta_time = (float)deltatime_delta_time(pLwc->update_dt);

    pLwc->battle_wall_tex_v += delta_time / 34;
    pLwc->battle_wall_tex_v = fmodf(pLwc->battle_wall_tex_v, 1.0f);
}

void logic_update_default_ui_proj(const int width, const int height, mat4x4 proj) {
    float aspect_ratio = (float)width / height;
    LOGIx("logic_update_default_ui_proj(): width: %d height: %d ratio: %f", width, height, aspect_ratio);
    if (aspect_ratio == 0) {
        LOGEP("Aspect ratio is zero! Set to 1.0f.");
        aspect_ratio = 1.0f;
    }
    if (aspect_ratio > 1) {
        mat4x4_ortho(proj, -aspect_ratio, aspect_ratio, -1.0f, 1.0f, 1.0f, -1.0f);
    } else {
        mat4x4_ortho(proj, -1.0f, 1.0f, -1.0f / aspect_ratio, 1.0f / aspect_ratio, 1.0f, -1.0f);
    }
}

void logic_update_default_ui_proj_for_htmlui(const int width, const int height, mat4x4 proj) {
    LOGIx("logic_update_default_ui_proj_for_htmlui(): width: %d height: %d", width, height);
    mat4x4_ortho(proj, 0, (float)width, 0, (float)height, 1.0f, -1.0f);
}

int lw_write_tcp_addr(LWCONTEXT* pLwc, const char* tcp_addr) {
#if LW_PLATFORM_ANDROID || LW_PLATFORM_IOS
    FILE* f;
    char path[1024] = { 0, };
    concat_path(path, pLwc->internal_data_path, LW_CONF_FILE_NAME);
    f = fopen(path, "w");
    if (f == 0) {
        // no cached user id exists
        LOGEP("error for writing path %s...", path);
        return -1;
    }
    char conf[1024] = { 0, };
    sprintf(conf, "{\n"
            "  \"ClientTcpHost\": \"%s\",\n"
            "  \"ConnPort\": \"19856\",\n"
            "  \"ClientTcpTtlHost\": \"%s\",\n"
            "  \"TtlConnPort\": \"3000\",\n"
            "  \"SeaUdpHost\": \"%s\",\n"
            "  \"SeaUdpPort\": \"3100\",\n"
            "  \"SeaTcpHost\": \"%s\",\n"
            "  \"SeaTcpPort\": \"3200\"\n"
            "}\n", tcp_addr, tcp_addr, tcp_addr, tcp_addr);
    fputs(conf, f);
    fclose(f);
#else
    LOGEP("NOT IMPLEMENTED YET");
#endif
    return 0;
}

void lwc_update(LWCONTEXT* pLwc, double delta_time) {
    if (pLwc->update_dt) {
        deltatime_tick_delta(pLwc->update_dt, delta_time);
    }

    //const float delta_time = (float)deltatime_delta_time(pLwc->update_dt);

    if (pLwc->next_game_scene == LGS_INVALID && pLwc->game_scene == LGS_INVALID) {
        // Default game scene
        pLwc->next_game_scene = LGS_FIELD;
    }

    if (pLwc->next_game_scene != LGS_INVALID && pLwc->next_game_scene != pLwc->game_scene) {

        pLwc->game_scene = pLwc->next_game_scene;
        pLwc->next_game_scene = LGS_INVALID;
        pLwc->scene_time = 0;

        if (pLwc->game_scene == LGS_BATTLE) {
            reset_battle_context(pLwc);
        }
    }

    // accumulate time since app startup
    pLwc->app_time += delta_time;
    pLwc->scene_time += delta_time;

    if (pLwc->field && field_network(pLwc->field)) {
        mq_poll(pLwc, pLwc->def_sys_msg, pLwc->mq, pLwc->field);
    }

    if (pLwc->game_scene == LGS_DIALOG) {
        update_dialog(pLwc);
    }

    if (pLwc->game_scene == LGS_PUCK_GAME) {
        update_puck_game(pLwc, pLwc->puck_game, delta_time);
        update_damage_text(pLwc);
    }

    if (pLwc->udp) {
        udp_update(pLwc, pLwc->udp);
    }

    if (pLwc->game_scene != LGS_PUCK_GAME) {
        remtex_update(pLwc->remtex, delta_time);
    }
    
    if (pLwc->tcp) {
        tcp_update(pLwc->tcp);
    }

    if (pLwc->tcp_ttl) {
        tcp_update(pLwc->tcp_ttl);
    }

    //****//
    // fix delta time
    //lwcontext_delta_time(pLwc) = 1.0f / 60;
    //****//

    if (pLwc->game_scene == LGS_BATTLE) {
        update_battle(pLwc);
    }


    if (pLwc->game_scene == LGS_FIELD) {
        move_player(pLwc);
        resolve_player_event_collision(pLwc);
    }

    if (pLwc->game_scene == LGS_BATTLE) {
        update_attack_trail(pLwc);

        update_damage_text(pLwc);

        update_battle_wall(pLwc);
    }
    update_sys_msg(pLwc->def_sys_msg, (float)delta_time);

    if (pLwc->battle_state == LBS_START_PLAYER_WIN || pLwc->battle_state == LBS_PLAYER_WIN_IN_PROGRESS) {
        update_battle_result(pLwc);
    }

    if (pLwc->game_scene == LGS_FIELD) {
        update_field(pLwc, pLwc->field);
    }

    if (pLwc->game_scene == LGS_PARTICLE_SYSTEM) {
        ps_test_update(pLwc, pLwc->ps_context);
    }

    if (pLwc->game_scene == LGS_FIELD || pLwc->game_scene == LGS_PUCK_GAME) {
        script_update(pLwc);
    }

    if (pLwc->game_scene == LGS_TTL) {
        lwttl_update(pLwc->ttl, pLwc, (float)delta_time);
    }

    // Touch start point will follow if the distance
    // between touch start point and current touch point
    // aparted by some amount...
    dir_pad_follow_start_position(&pLwc->left_dir_pad);

    script_emit_logic_frame_finish(pLwc->L, (float)delta_time);

    // Check for a new native user text input
    if (pLwc->last_text_input_seq != lw_get_text_input_seq()) {
        switch (lw_text_input_tag()) {
        case LITI_NICKNAME:
        {
            //char nicknameMsg[256];
            //sprintf(nicknameMsg, "Changing nickname to %s...", lw_get_text_input());
            //show_sys_msg(pLwc->def_sys_msg, nicknameMsg);
            tcp_send_setnickname(pLwc->tcp, &pLwc->tcp->user_id, lw_get_text_input());
            break;
        }
        case LITI_SERVER_ADDR:
        {
            lw_write_tcp_addr(pLwc, lw_get_text_input());
            break;
        }
        default:
            break;
        }
        pLwc->last_text_input_seq = lw_get_text_input_seq();
    }
    pLwc->update_count++;
}

void init_lwc_runtime_data(LWCONTEXT* pLwc) {

    reset_runtime_context(pLwc);

    pLwc->dialog = create_string_from_file(ASSETS_BASE_PATH "d" PATH_SEPARATOR "d1.txt");
    if (pLwc->dialog) {
        pLwc->dialog_bytelen = (int)strlen(pLwc->dialog);
    } else {
        LOGE("dialog loading failed.");
    }
}

static int loop_logic_update(zloop_t* loop, int timer_id, void* args) {
    LWCONTEXT* pLwc = args;
    if (pLwc->quit_request) {
        //zloop_timer_end(loop, timer_id);
        // End the reactor
        return -1;
    }
    double now = lwtimepoint_now_seconds();
    lwc_update(pLwc, now - pLwc->last_now);
    pLwc->last_now = now;
    return 0;
}

static int loop_pipe_reader(zloop_t* loop, zsock_t* pipe, void* args) {
    LWCONTEXT* pLwc = args;
    zmsg_t* msg = zmsg_recv(pipe);
    LOGIPx(LWLOGPOS "Message received through pipe");
    zframe_t* f = zmsg_first(msg);
    int rc = 0;
    while (f) {
        byte* d = zframe_data(f);
        size_t s = zframe_size(f);
        if (d && s == sizeof(LWMSGINITFIELD) && *(int*)d == LM_LWMSGINITFIELD) {
            // Stop new frame of rendering
            lwcontext_set_safe_to_start_render(pLwc, 0);
            // Busy wait for current frame of rendering to be completed
            while (lwcontext_rendering(pLwc)) {}

            LWMSGINITFIELD* m = (LWMSGINITFIELD*)d;
            init_field(pLwc,
                       m->field_filename,
                       m->nav_filename,
                       m->field_mesh_count,
                       m->field_mesh,
                       m->skin_scale,
                       m->follow_cam);

            init_lwc_runtime_data(pLwc);
        } else if (d && s == sizeof(LWMSGINITSCENE) && *(int*)d == LM_LWMSGINITSCENE) {
            // Stop new frame of rendering
            lwcontext_set_safe_to_start_render(pLwc, 0);
            // Busy wait for current frame of rendering to be completed
            while (lwcontext_rendering(pLwc)) {}

            init_lwc_runtime_data(pLwc);
        } else if (d && s == sizeof(LWMSGRESETRUNTIMECONTEXT) && *(int*)d == LM_LWMSGRESETRUNTIMECONTEXT) {
            // Stop new frame of rendering
            lwcontext_set_safe_to_start_render(pLwc, 0);
            // Busy wait for current frame of rendering to be completed
            while (lwcontext_rendering(pLwc)) {}

            reset_runtime_context(pLwc);
        } else if (d && s == sizeof(LWMSGRELOADSCRIPT) && *(int*)d == LM_LWMSGRELOADSCRIPT) {
            // TODO: Reload all scripts
        } else if (d && s == sizeof(LWMSGRELOADSCRIPT) && *(int*)d == LM_LWMSGSTARTLOGICLOOP) {
            logic_start_logic_update_job(pLwc);
        } else if (d && s == sizeof(LWMSGRELOADSCRIPT) && *(int*)d == LM_LWMSGSTOPLOGICLOOP) {
            logic_stop_logic_update_job(pLwc);
        } else if (d && s == sizeof(LWMSGUIEVENT) && *(int*)d == LM_LWMSGUIEVENT) {
            LWMSGUIEVENT* m = (LWMSGUIEVENT*)d;
            if (pLwc->puck_game->world_roll_dirty == 0) {
                LOGIx("UI Event: %s (w_ratio=%.2f, h_ratio=%.2f)", m->id, m->w_ratio, m->h_ratio);
                script_emit_ui_event(pLwc->L, m->id, m->w_ratio, m->h_ratio);
                play_sound(LWS_CLICK);
            } else {
                LOGI("UI Event '%s' suppressed since state is dirty", m->id);
            }
        } else if (d && s == sizeof(LWMSGEVALUATE) && *(int*)d == LM_LWMSGEVALUATE) {
            // Lua script code in the next frame.
            f = zmsg_next(msg);
            byte* code = zframe_data(f);
            size_t code_len = zframe_size(f);
            f = zmsg_next(msg);
            byte* name = zframe_data(f);
            //size_t code_len = zframe_size(f);
            script_evaluate_with_name(pLwc->L, code, code_len, name);
        } else if (strncmp((const char*)d, "$TERM", 5) == 0) {
            // ZFRAME spec?
            rc = -1;
        } else {
            abort();
        }
        f = zmsg_next(msg);
    }
    zmsg_destroy(&msg);
    return rc;
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

static int str2int(const char* str, int len) {
    int i;
    int ret = 0;
    for (i = 0; i < len; ++i) {
        ret = ret * 10 + (str[i] - '0');
    }
    return ret;
}

static void parse_countries(LWCONTEXT* pLwc, LWCOUNTRYARRAY* country_array, const char* conf_path) {
    jsmn_parser conf_parser;
    jsmn_init(&conf_parser);
    jsmntok_t* conf_token = malloc(sizeof(jsmntok_t) * LW_MAX_CONF_TOKEN);
    LOGI("sizeof(char) == %zu", sizeof(char));
    char *conf_str = create_string_from_file(conf_path);
    if (conf_str) {
        int token_count = jsmn_parse(&conf_parser, conf_str, strlen(conf_str), conf_token, LW_MAX_CONF_TOKEN);
        jsmntok_t* t = conf_token;
        if (token_count < 1 || t[0].type != JSMN_ARRAY) {
            LOGE("Conf file broken...");
            fflush(stdout);
            exit(-1);
        }
        LOGI("countries file: %s", conf_path);
        
        int entry_count = t[0].size;
        LWCOUNTRY* country = (LWCOUNTRY*)calloc(entry_count, sizeof(LWCOUNTRY));
        
        int string_count = 0;
        int entry_index = -1;
        for (int i = 1; i < token_count; i++) {
            if (t[i].type == JSMN_STRING) {
                LOGIx("countries: %.*s", t[i].end - t[i].start, conf_str + t[i].start);
                int string_mod = string_count % 3;
                if (string_mod == 0) {
                    entry_index++;
                    strncpy(country[entry_index].code, conf_str + t[i].start, t[i].end - t[i].start);
                    country[entry_index].code[t[i].end - t[i].start] = 0;
                } else if (string_mod == 1) {
                    // korean
                    strncpy(country[entry_index].name, conf_str + t[i].start, t[i].end - t[i].start);
                    country[entry_index].name[t[i].end - t[i].start] = 0;
                } else if (string_mod == 2) {
                    // english
                }
                string_count++;
            }
        }
        country_array->count = entry_count;
        country_array->first = country;
        //free(atlas_sprite);
        conf_str = 0;
        release_string(conf_str);
    } else {
        LOGE("Atlas conf file %s not found!", conf_path);
        fflush(stdout);
        exit(-2);
    }
    free(conf_token);
}

static void parse_atlas_conf(LWCONTEXT* pLwc, LWATLASSPRITEARRAY* atlas_array, const char* conf_path, LW_ATLAS_ENUM first_lae, LW_ATLAS_ENUM first_alpha_lae) {
    jsmn_parser conf_parser;
    jsmn_init(&conf_parser);
    jsmntok_t* conf_token = malloc(sizeof(jsmntok_t) * LW_MAX_CONF_TOKEN);
    LOGI("sizeof(char) == %zu", sizeof(char));
    char *conf_str = create_string_from_file(conf_path);
    if (conf_str) {
        int token_count = jsmn_parse(&conf_parser, conf_str, strlen(conf_str), conf_token, LW_MAX_CONF_TOKEN);
        jsmntok_t* t = conf_token;
        if (token_count < 1 || t[0].type != JSMN_OBJECT) {
            LOGE("Conf file broken...");
            fflush(stdout);
            exit(-1);
        }
        LOGI("atlas file: %s", conf_path);
        if (jsoneq(conf_str, &t[1], "textures") == 0) {
            // 'crunch' generated output json
            if (token_count < 2 || t[2].type != JSMN_ARRAY) {
                LOGE("Conf file broken...");
                fflush(stdout);
                exit(-1);
            }
            int entry_count = 0;
            for (int i = 1; i < token_count; i++) {
                if (jsoneq(conf_str, &t[i], "images") == 0 && t[i + 1].type == JSMN_ARRAY) {
                    entry_count += t[i + 1].size;
                }
            }
            LWATLASSPRITE* atlas_sprite = (LWATLASSPRITE*)calloc(entry_count, sizeof(LWATLASSPRITE));
            int entry_index = -1;
            int atlas_index = -1;
            for (int i = 1; i < token_count; i++) {
                if (jsoneq(conf_str, &t[i], "name") == 0) {
                    atlas_index++;
                } else if (jsoneq(conf_str, &t[i], "n") == 0) {
                    LOGIx("n: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                    entry_index++;
                    strncpy(atlas_sprite[entry_index].name, conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
                    atlas_sprite[entry_index].atlas_index = atlas_index;
                } else if (jsoneq(conf_str, &t[i], "x") == 0) {
                    LOGIx("x: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                    atlas_sprite[entry_index].x = str2int(conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
                } else if (jsoneq(conf_str, &t[i], "y") == 0) {
                    LOGIx("y: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                    atlas_sprite[entry_index].y = str2int(conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
                } else if (jsoneq(conf_str, &t[i], "w") == 0) {
                    LOGIx("w: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                    atlas_sprite[entry_index].width = str2int(conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
                } else if (jsoneq(conf_str, &t[i], "h") == 0) {
                    LOGIx("h: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                    atlas_sprite[entry_index].height = str2int(conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
                }
            }
            char* period = strrchr(conf_path, '.');
            char* separator = strrchr(conf_path, PATH_SEPARATOR[0]);
            size_t atlas_name_len = period - (separator + 1);
            strncpy(atlas_array->atlas_name, separator + 1, atlas_name_len);
            atlas_array->atlas_name[atlas_name_len] = 0;
            atlas_array->count = entry_count;
            atlas_array->first = atlas_sprite;
            atlas_array->first_lae = first_lae;
            atlas_array->first_alpha_lae = first_alpha_lae;
            //free(atlas_sprite);
            conf_str = 0;
        } else {
            // 'simple (legacy)' output json
            int entry_count = 0;
            for (int i = 1; i < token_count; i++) {
                if (jsoneq(conf_str, &t[i], "name") == 0) {
                    entry_count++;
                }
            }
            LWATLASSPRITE* atlas_sprite = (LWATLASSPRITE*)calloc(entry_count, sizeof(LWATLASSPRITE));
            int entry_index = -1;
            for (int i = 1; i < token_count; i++) {
                if (jsoneq(conf_str, &t[i], "name") == 0) {
                    LOGIx("name: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                    entry_index++;
                    strncpy(atlas_sprite[entry_index].name, conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
                } else if (jsoneq(conf_str, &t[i], "x") == 0) {
                    LOGIx("x: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                    atlas_sprite[entry_index].x = str2int(conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
                } else if (jsoneq(conf_str, &t[i], "y") == 0) {
                    LOGIx("y: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                    atlas_sprite[entry_index].y = str2int(conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
                } else if (jsoneq(conf_str, &t[i], "width") == 0) {
                    LOGIx("width: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                    atlas_sprite[entry_index].width = str2int(conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
                } else if (jsoneq(conf_str, &t[i], "height") == 0) {
                    LOGIx("height: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                    atlas_sprite[entry_index].height = str2int(conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
                }
            }
            char* period = strrchr(conf_path, '.');
            char* separator = strrchr(conf_path, PATH_SEPARATOR[0]);
            size_t atlas_name_len = period - (separator + 1);
            strncpy(atlas_array->atlas_name, separator + 1, atlas_name_len);
            atlas_array->atlas_name[atlas_name_len] = 0;
            atlas_array->count = entry_count;
            atlas_array->first = atlas_sprite;
            atlas_array->first_lae = first_lae;
            atlas_array->first_alpha_lae = first_alpha_lae;
            //free(atlas_sprite);
            conf_str = 0;
        }
        release_string(conf_str);
    } else {
        LOGE("Atlas conf file %s not found!", conf_path);
        fflush(stdout);
        atlas_array->count = 0;
        atlas_array->first = 0;
        exit(-2);
    }
    free(conf_token);
}

static void parse_atlas(LWCONTEXT* pLwc) {
    for (int i = 0; i < LAC_COUNT; i++) {
        parse_atlas_conf(pLwc,
                         &pLwc->atlas_conf[i],
                         atlas_conf_filename[i],
                         atlas_first_lae[i],
                         atlas_first_alpha_lae[i]);
    }
}

static void parse_conf(LWCONTEXT* pLwc) {
    const char *conf_path = ASSETS_BASE_PATH "conf" PATH_SEPARATOR LW_CONF_FILE_NAME;
    jsmn_parser conf_parser;
    jsmn_init(&conf_parser);
    jsmntok_t* conf_token = malloc(sizeof(jsmntok_t) * LW_MAX_CONF_TOKEN);
    LOGI("sizeof(char) == %zu", sizeof(char));
    // check user saved (customized) conf.json first
    FILE* f;
    char path[1024] = { 0, };
    concat_path(path, pLwc->internal_data_path, LW_CONF_FILE_NAME);
    f = fopen(path, "r");
    char *conf_str;
    if (f) {
        fseek(f, 0, SEEK_END);
        const int f_size = (int)ftell(f);
        fseek(f, 0, SEEK_SET);
        char* d = (char*)malloc(f_size + 1);
        const size_t last_byte = fread(d, 1, f_size, f);
        d[last_byte] = '\0';
        fclose(f);
        LOGI("parse_conf (user saved): %s (%d bytes) loaded to memory.", path, (int)last_byte);
        conf_str = d;
    } else {
        conf_str = create_string_from_file(conf_path);
    }
    if (conf_str) {
        int token_count = jsmn_parse(&conf_parser, conf_str, strlen(conf_str), conf_token, LW_MAX_CONF_TOKEN);
        jsmntok_t* t = conf_token;
        if (token_count < 1 || t[0].type != JSMN_OBJECT) {
            LOGE("Conf file broken...");
            fflush(stdout);
            exit(-1);
        }
        LOGI("Conf file: %s", conf_path);
        for (int i = 1; i < token_count; i++) {
            if (jsoneq(conf_str, &t[i], "ClientTcpHost") == 0) {
                LOGI("ClientTcpHost: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                strncpy(pLwc->tcp_host_addr.host, conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            } else if (jsoneq(conf_str, &t[i], "ConnPort") == 0) {
                LOGI("ConnPort: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                strncpy(pLwc->tcp_host_addr.port_str, conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
                pLwc->tcp_host_addr.port = atoi(pLwc->tcp_host_addr.port_str);
            } else if (jsoneq(conf_str, &t[i], "ClientTcpTtlHost") == 0) {
                LOGI("ClientTcpTtlHost: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                strncpy(pLwc->tcp_ttl_host_addr.host, conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            } else if (jsoneq(conf_str, &t[i], "TtlConnPort") == 0) {
                LOGI("TtlConnPort: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                strncpy(pLwc->tcp_ttl_host_addr.port_str, conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
                pLwc->tcp_ttl_host_addr.port = atoi(pLwc->tcp_ttl_host_addr.port_str);
            } else if (jsoneq(conf_str, &t[i], "SeaUdpHost") == 0) {
                LOGI("SeaUdpHost: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                strncpy(pLwc->sea_udp_host_addr.host, conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            } else if (jsoneq(conf_str, &t[i], "SeaUdpPort") == 0) {
                LOGI("SeaUdpPort: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                strncpy(pLwc->sea_udp_host_addr.port_str, conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
                pLwc->sea_udp_host_addr.port = atoi(pLwc->sea_udp_host_addr.port_str);
            } else if (jsoneq(conf_str, &t[i], "SeaTcpHost") == 0) {
                LOGI("SeaTcpHost: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                strncpy(pLwc->sea_tcp_host_addr.host, conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            } else if (jsoneq(conf_str, &t[i], "SeaTcpPort") == 0) {
                LOGI("SeaTcpPort: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                strncpy(pLwc->sea_tcp_host_addr.port_str, conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
                pLwc->sea_tcp_host_addr.port = atoi(pLwc->sea_tcp_host_addr.port_str);
            }
        }
        release_string(conf_str);
        conf_str = 0;
    } else {
        LOGE("Conf file not found!");
        fflush(stdout);
        exit(-2);
    }
    free(conf_token);
}

static void s_logic_worker(zsock_t *pipe, void *args) {
    LWCONTEXT* pLwc = args;
    
    parse_conf(pLwc);
    parse_atlas(pLwc);
    parse_countries(pLwc, &pLwc->country_array, ASSETS_BASE_PATH "ttldata" PATH_SEPARATOR "countries.json");
    
    // Send 'worker ready' signal to parent thread
    zsock_signal(pipe, 0);
    LWTIMEPOINT last_time;
    lwtimepoint_now(&last_time);

    // Create a new udp instance here since
    // WSAStartup should be called within
    // a thread which opens sockets.
    pLwc->udp = 0;//new_udp();
    LWUDP* sea_udp = new_udp();
    lwttl_set_sea_udp(pLwc->ttl, sea_udp);
    udp_update_addr_host(sea_udp,
                         pLwc->sea_udp_host_addr.host,
                         pLwc->sea_udp_host_addr.port,
                         pLwc->sea_udp_host_addr.port_str);
    // load previous ttl data
    lwttl_read_last_state(pLwc->ttl, pLwc);
    // Ball Rumble TCP connection: no connection made at this point. Lazy connect.
    /*pLwc->tcp = new_tcp(pLwc,
                        pLwc->user_data_path,
                        &pLwc->tcp_host_addr,
                        tcp_on_connect,
                        parse_recv_packets);*/
    
    zloop_t* loop = zloop_new();
    pLwc->logic_loop = loop;
    logic_start_logic_update_job(pLwc);
    zloop_reader(loop, pipe, loop_pipe_reader, pLwc);
    // Start the reactor loop
    zloop_start_noalloc(loop);
    // Reactor loop finished.
    // Send 'worker finished' signal to parent thread
    zsock_signal(pipe, 0);
}

void logic_start_logic_update_job(LWCONTEXT* pLwc) {
    if (pLwc == 0 || pLwc->logic_loop == 0) {
        return;
    }
    if (pLwc->logic_update_job) {
        logic_stop_logic_update_job(pLwc);
    }
    pLwc->last_now = lwtimepoint_now_seconds();
    pLwc->logic_update_job = zloop_timer(pLwc->logic_loop, (size_t)(pLwc->update_interval * 1000), 0, loop_logic_update, pLwc);
}

void logic_stop_logic_update_job(LWCONTEXT* pLwc) {
    if (pLwc == 0 || pLwc->logic_loop == 0) {
        return;
    }
    zloop_timer_end(pLwc->logic_loop, pLwc->logic_update_job);
    pLwc->logic_update_job = 0;
}

void lwc_start_logic_thread(LWCONTEXT* pLwc) {
    // Start logic thread
    pLwc->logic_actor = zactor_new(s_logic_worker, pLwc);
    // Three entry points - Choose one
    // [1] Field Scene
    // [2] Ball Rumble Scene
    // [3] TTL Scene
    /*[1]*///load_field_2_init_runtime_data_async(pLwc, pLwc->logic_actor);
    /*[2]*/load_scene_async(pLwc, pLwc->logic_actor, change_to_puck_game);
    /*[3]*///load_scene_async(pLwc, pLwc->logic_actor, change_to_ttl);
}

const char* logic_server_addr(int idx) {
    return server_addr[idx];
}

void toggle_font_texture_test_mode(LWCONTEXT* pLwc) {
    pLwc->font_texture_texture_mode = !pLwc->font_texture_texture_mode;
}

static void lw_htmlui_redraw_ui_fbo_async(LWCONTEXT* pLwc) {
    rmsg_redraw_ui_fbo(pLwc);
}

void lw_first_page(LWCONTEXT* pLwc) {
    pLwc->country_page = 0;
    htmlui_update_country_data(pLwc, pLwc->htmlui);
    lw_htmlui_redraw_ui_fbo_async(pLwc);
}

void lw_prev_page(LWCONTEXT* pLwc) {
    pLwc->country_page = LWMAX(0, pLwc->country_page - 1);
    htmlui_update_country_data(pLwc, pLwc->htmlui);
    lw_htmlui_redraw_ui_fbo_async(pLwc);
}

void lw_next_page(LWCONTEXT* pLwc) {
    pLwc->country_page = LWMIN(pLwc->country_page + 1, (pLwc->country_array.count - 1) / 20);
    htmlui_update_country_data(pLwc, pLwc->htmlui);
    lw_htmlui_redraw_ui_fbo_async(pLwc);
}

void lw_last_page(LWCONTEXT* pLwc) {
    pLwc->country_page = (pLwc->country_array.count - 1) / 20;
    htmlui_update_country_data(pLwc, pLwc->htmlui);
    lw_htmlui_redraw_ui_fbo_async(pLwc);
}

void lw_new_tcp_ttl(LWCONTEXT* pLwc) {
    if (pLwc->tcp_ttl == 0) {
        pLwc->tcp_ttl = new_tcp(pLwc,
                                pLwc->user_data_path,
                                &pLwc->tcp_ttl_host_addr,
                                tcp_ttl_on_connect,
                                parse_recv_packets);
    } else {
        LOGEP("tcp_ttl already set!");
    }
}
