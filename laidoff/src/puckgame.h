#pragma once
#include "linmath.h"
#include "puckgamepacket.h"
#include <ode/ode.h>
#include "pcg_basic.h"

typedef struct _LWPUCKGAME LWPUCKGAME;
typedef struct _LWPUCKGAMERECORD LWPUCKGAMERECORD;

typedef enum _LW_PUCK_GAME_BOUNDARY {
	LPGB_GROUND,
    // Square stage walls
    LPGB_E,
    LPGB_W,
    LPGB_S, 
    LPGB_N,
    // Octagon stage walls
    LPGB_EE,
    LPGB_EN,
    LPGB_NN,
    LPGB_NW,
    LPGB_WW,
    LPGB_WS,
    LPGB_SS,
    LPGB_SE,
    // Diagonal walls
    LPGB_DIAGONAL_1,
    LPGB_DIAGONAL_2,
    LPGB_COUNT
} LW_PUCK_GAME_BOUNDARY;

typedef enum _LW_PUCK_GAME_OBJECT {
	LPGO_PUCK,
	LPGO_PLAYER,
	LPGO_TARGET,
	LPGO_COUNT,
} LW_PUCK_GAME_OBJECT;

typedef enum _LW_PUCK_GAME_STATE {
    LPGS_MAIN_MENU,
    LPGS_PRACTICE,
    LPGS_TUTORIAL,
    LPGS_SEARCHING,
    LPGS_BATTLE_PREPARE, // READY...STEADY...
    LPGS_BATTLE, // ...GO!
} LW_PUCK_GAME_STATE;

typedef struct _LWPUCKGAMEOBJECT {
	dGeomID geom;
	dBodyID body;
	float pos[3]; // read-only
	float move_rad; // angle on x-y plane
	float radius;
    float length; // only valid if capsule
	mat4x4 rot;
	LWPUCKGAME* puck_game;
	int wall_hit_count;
	float speed;
	int red_overlay;
    int capsule;
} LWPUCKGAMEOBJECT;

typedef struct _LWPUCKGAMEJUMP {
    float last_time;
    float remain_time;
    float shake_remain_time;
} LWPUCKGAMEJUMP;

typedef struct _LWPUCKGAMEDASH {
	float last_time;
	float remain_time;
	float dir_x;
	float dir_y;
	float shake_remain_time;
    int disabled;
} LWPUCKGAMEDASH;

typedef struct _LWPUCKGAMEFIRE {
    float last_time;
    float remain_time;
    float dir_x;
    float dir_y;
    float dir_len;
    float shake_remain_time;
} LWPUCKGAMEFIRE;

typedef struct _LWPUCKGAMEPLAYER {
	int total_hp;
	int current_hp;
	dBodyID last_contact_puck_body;
	int puck_contacted;
	float hp_shake_remain_time;
} LWPUCKGAMEPLAYER;

typedef struct _LWREMOTEPLAYERCONTROL {
	int dir_pad_dragging;
	float dx;
	float dy;
    float dlen;
	int pull_puck;
} LWREMOTEPLAYERCONTROL;

typedef struct _LWPUCKGAMETOWER {
    dGeomID geom;
    int hp;
    int owner_player_no;
    double last_damaged_at;
    float shake_remain_time;
    int collapsing; // 0 if normal state, 1 if collapsing state
    float collapsing_time; // time elapsed from starting collapsing animation
    int invincible;
} LWPUCKGAMETOWER;

#define LW_PUCK_GAME_TOWER_COUNT (2)
#define LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM (2)
#define LW_PUCK_GAME_PLAYER_TEAM (0)
#define LW_PUCK_GAME_TARGET_TEAM (1)

typedef enum _LW_PUCK_GAME_CONTROL_FLAGS {
    LPGCF_HIDE_TIMER = 1 << 0,
    LPGCF_HIDE_LEFT_DIR_PAD = 1 << 1,
    LPGCF_HIDE_DASH_BUTTON = 1 << 2,
    LPGCF_HIDE_PULL_BUTTON = 1 << 3,
} LW_PUCK_GAME_CONTROL_FLAGS;

typedef struct _LWPUCKGAMEBOGUSPARAM {
    float target_follow_agility;
    float dash_detect_radius;
    float dash_frequency;
    float dash_cooltime_lag_min;
    float dash_cooltime_lag_max;
} LWPUCKGAMEBOGUSPARAM;

typedef struct _LWPUCKGAME {
	// ---- Static game data begin
	float world_width;
    float world_height;
    float wall_height;
	float dash_interval;
	float dash_duration;
	float dash_shake_time;
	float hp_shake_time;
    float jump_force;
    float jump_interval;
    float jump_shake_time;
	float puck_damage_contact_speed_threshold;
    float sphere_mass;
    float puck_sphere_radius;
    float player_sphere_radius;
    float target_sphere_radius;
    float player_capsule_length;
	float total_time;
    float fire_max_force;
    float fire_max_vel;
    float fire_interval;
    float fire_duration;
    float fire_shake_time;
    float tower_pos;
    float tower_radius;
    float tower_mesh_radius;
    int tower_total_hp;
    float tower_shake_time;
    float go_start_pos;
    int hp;
    float player_max_move_speed;
    float player_dash_speed;
    float boundary_impact_falloff_speed;
    float boundary_impact_start;
    float control_joint_max_force;
    float bounce;
    int follow_cam;
    int hide_hp_star;
    int dash_by_direction;
    int player_capsule;
    int puck_lae;
    int player_lae;
    int target_lae;
    int puck_lvt;
    int player_lvt;
    // ---- Static game data end
    int prepare_step_wait_tick; // wait ticks for each 'ready....' and 'steady....' battle phases
    float world_width_half;
    float world_height_half;
    float tower_pos_multiplier[LW_PUCK_GAME_TOWER_COUNT][2]; // '2' is for x- and y-axis.
    float tower_collapsing_z_rot_angle[LW_PUCK_GAME_TOWER_COUNT];
	dWorldID world;
	dSpaceID space;
	dGeomID boundary[LPGB_COUNT];
    float boundary_impact[LPGB_COUNT];
    int boundary_impact_player_no[LPGB_COUNT];
    LWPUCKGAMETOWER tower[LW_PUCK_GAME_TOWER_COUNT];
	LWPUCKGAMEOBJECT go[LPGO_COUNT];
	dJointGroupID contact_joint_group;
	dJointGroupID player_control_joint_group[LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM];
	dJointGroupID target_control_joint_group[LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM];
	dJointGroupID puck_pull_control_joint_group;
	dJointID player_control_joint[LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM]; // player 1
	dJointID target_control_joint[LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM]; // player 2
	dJointID puck_pull_control_joint;
	int push;
	float time;
	LWPUCKGAMEPLAYER pg_player[LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM];
	LWPUCKGAMEPLAYER pg_target[LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM];
	float last_remote_dx;
	float last_remote_dy;
	void(*on_player_damaged)(LWPUCKGAME*);
	void(*on_target_damaged)(LWPUCKGAME*);
    void(*on_puck_wall_collision)(LWPUCKGAME*, float, float);
    void(*on_puck_tower_collision)(LWPUCKGAME*, float, float);
    void(*on_puck_player_collision)(LWPUCKGAME*, float, float);
	void(*on_player_dash)(LWPUCKGAME*);
    void(*on_ready)(LWPUCKGAME*);
    void(*on_steady)(LWPUCKGAME*);
    void(*on_go)(LWPUCKGAME*);
	void(*on_finished)(LWPUCKGAME*, int);
    void(*on_new_record_frame)(const LWPUCKGAME*, LWPUCKGAMERECORD*, unsigned short);
	void* server;
	int battle_id;
	unsigned int token;
	unsigned int player_no;
	unsigned int c1_token;
	unsigned int c2_token;
    unsigned int c3_token;
    unsigned int c4_token;
    LWREMOTEPLAYERCONTROL remote_control[2][LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM]; // '2': 0 - PLAYER TEAM, 1 - TARGET TEAM
	LWPUCKGAMEDASH remote_dash[2][LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM]; // '2': 0 - PLAYER TEAM, 1 - TARGET TEAM
    LWPUCKGAMEJUMP remote_jump[2][LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM]; // '2': 0 - PLAYER TEAM, 1 - TARGET TEAM
    LWPUCKGAMEFIRE remote_fire[2][LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM]; // '2': 0 - PLAYER TEAM, 1 - TARGET TEAM
	int init_ready;
    LWP_STATE_PHASE battle_phase;
	int update_tick;
	char nickname[LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM][LW_NICKNAME_MAX_LEN];
	char target_nickname[LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM][LW_NICKNAME_MAX_LEN];
    unsigned int id1[4];
    unsigned int id2[4];
    unsigned int id3[4];
    unsigned int id4[4];
    int puck_owner_player_no;
    float puck_reflect_size;
    int world_roll_axis; // 0: x-axis, 1: y-axis, 2: z-axis
    int world_roll_dir; // 0: positive, 1: negative
    float world_roll;
    float world_roll_target;
    float world_roll_target_follow_ratio;
    int world_roll_dirty;
    void* pLwc; // opaque pointer to LWCONTEXT
    float target_dx[LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM];
    float target_dy[LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM];
    float target_dlen_ratio[LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM];
    float battle_ui_alpha;
    float battle_control_ui_alpha;
    float main_menu_ui_alpha;
    LW_PUCK_GAME_STATE game_state;
    char searching_str[256];
    int prepare_step_waited_tick;
    int wall_hit_bit; // LWP_STATE_WALL_HIT_BIT (ORing) - reset at every tick
    int wall_hit_bit_send_buf_1; // reset at every UDP state sync send (P1)
    int wall_hit_bit_send_buf_2; // reset at every UDP state sync send (P2)
    LWPBATTLERESULT_STAT battle_stat[2];
    int player_score[LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM];
    int player_rank[LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM];
    int target_score[LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM];
    int control_flags;
    char tutorial_guide_str[512];
    int bogus_disabled;
    int bogus_opponent;
    pcg32_random_t bogus_rng;
    LWPMATCHED2 matched2;
    char score_message[64];
    float last_udp_frame_sync_sent;
    int show_top_level_main_menu;
    int show_html_ui;
    LW_PUCK_GAME_MAP game_map;
    LWPUCKGAMERECORD* record;
    int record_replay_mode;
} LWPUCKGAME;

LWPUCKGAME* new_puck_game(int update_frequency, LW_PUCK_GAME_MAP gameMap);
void puck_game_set_static_default_values(LWPUCKGAME* puck_game);
void puck_game_set_secondary_static_default_values(LWPUCKGAME* puck_game);
void delete_puck_game(LWPUCKGAME** puck_game);
void puck_game_push(LWPUCKGAME* puck_game);
float puck_game_dash_gauge_ratio(LWPUCKGAME* puck_game, const LWPUCKGAMEDASH* dash);
float puck_game_dash_elapsed_since_last(const LWPUCKGAME* puck_game, const LWPUCKGAMEDASH* dash);
float puck_game_jump_cooltime(LWPUCKGAME* puck_game);
int puck_game_dashing(LWPUCKGAMEDASH* dash);
int puck_game_jumping(LWPUCKGAMEJUMP* jump);
void puck_game_near_callback(void *data, dGeomID o1, dGeomID o2);
void puck_game_commit_jump(LWPUCKGAME* puck_game, LWPUCKGAMEJUMP* jump, int player_no);
void puck_game_commit_dash(LWPUCKGAME* puck_game, LWPUCKGAMEDASH* dash, float dx, float dy, int player_no);
void puck_game_commit_dash_to_puck(LWPUCKGAME* puck_game, LWPUCKGAMEDASH* dash, int player_no);
float puck_game_remain_time(float total_time, int update_tick, int update_frequency);
float puck_game_elapsed_time(int update_tick, int update_frequency);
int puck_game_remain_time_floor(float total_time, int update_tick, int update_frequency);
void puck_game_commit_fire(LWPUCKGAME* puck_game, LWPUCKGAMEFIRE* fire, int player_no, float puck_fire_dx, float puck_fire_dy, float puck_fire_dlen);
void update_puck_ownership(LWPUCKGAME* puck_game);
void update_puck_reflect_size(LWPUCKGAME* puck_game, float delta_time);
void puck_game_remote_state_reset(LWPUCKGAME* puck_game, LWPSTATE* state);
void puck_game_tower_pos(vec4 p_out, const LWPUCKGAME* puck_game, int owner_player_no);
void puck_game_control_bogus(LWPUCKGAME* puck_game, const LWPUCKGAMEBOGUSPARAM* bogus_param);
void puck_game_update_remote_player(LWPUCKGAME* puck_game, float delta_time, int i);
LWPUCKGAMEDASH* puck_game_single_play_dash_object(LWPUCKGAME* puck_game);
LWPUCKGAMEJUMP* puck_game_single_play_jump_object(LWPUCKGAME* puck_game);
int puck_game_dash(LWPUCKGAME* puck_game, LWPUCKGAMEDASH* dash, int player_no);
int puck_game_dash_can_cast(const LWPUCKGAME* puck_game, const LWPUCKGAMEDASH* dash);
void puck_game_set_searching_str(LWPUCKGAME* puck_game, const char* str);
void puck_game_set_tutorial_guide_str(LWPUCKGAME* puck_game, const char* str);
void puck_game_update_tick(LWPUCKGAME* puck_game, int update_frequency);
void puck_game_create_go(LWPUCKGAME* puck_game, int lpgo, float x, float y, float z, float radius);
void puck_game_create_capsule_go(LWPUCKGAME* puck_game, int lpgo, float x, float y, float z, float radius, float length);
void puck_game_create_control_joint(LWPUCKGAME* puck_game, int lpgo);
void puck_game_reset_go(LWPUCKGAME* puck_game, LWPUCKGAMEOBJECT* go, float x, float y, float z);
void puck_game_create_tower_geom(LWPUCKGAME* puck_game, int i);
void puck_game_set_tower_invincible(LWPUCKGAME* puck_game, int tower_index, int v);
void puck_game_set_dash_disabled(LWPUCKGAME* puck_game, int index, int v);
void puck_game_set_bogus_disabled(LWPUCKGAME* puck_game, int v);
void puck_game_create_all_battle_objects(LWPUCKGAME* puck_game);
void puck_game_destroy_all_battle_objects(LWPUCKGAME* puck_game);
void puck_game_reset_battle_state(LWPUCKGAME* puck_game);
void puck_game_reset_tutorial_state(LWPUCKGAME* puck_game);
void puck_game_reset(LWPUCKGAME* puck_game);
void puck_game_set_tower_pos_multiplier(LWPUCKGAME* puck_game, int index, float mx, float my);
void puck_game_create_walls(LWPUCKGAME* puck_game);
void puck_game_destroy_walls(LWPUCKGAME* puck_game);
LWPUCKGAMEPLAYER* puck_game_player(LWPUCKGAME* puck_game, int index);
LWPUCKGAMEPLAYER* puck_game_target(LWPUCKGAME* puck_game, int index);
void puck_game_set_show_top_level_main_menu(LWPUCKGAME* puck_game, int show);
void puck_game_set_show_htmlui(LWPUCKGAME* puck_game, int show);
int puck_game_leaderboard_items_in_page(float aspect_ratio);
void puck_game_on_new_record_frame(const LWPUCKGAME* puck_game, LWPUCKGAMERECORD* record, unsigned short update_tick);
void puck_game_fill_state_bitfield(LWPSTATEBITFIELD* bf, const LWPUCKGAME* puck_game, const LWPUCKGAMEPLAYER* player, const LWPUCKGAMEPLAYER* target, const int* wall_hit_bit);
void puck_game_on_finalize_record(const LWPUCKGAME* puck_game, const LWPUCKGAMERECORD* record);
