#pragma once

#include "lwgl.h"
#include "linmath.h"
#include "lwdamagetext.h"
#include "lwbattlecreature.h"
#include "lwenemy.h"
#include "lwshader.h"
#include "lwvbo.h"
#include "lwvbotype.h"
#include "lwmacro.h"
#include "lwatlasenum.h"
#include "lwbuttoncommand.h"
#include "lwprogrammedtex.h"
#include "lwbattlestate.h"
#include "lwgamescene.h"
#include "lwanim.h"
#include "lwtouchproc.h"
//#include "lwbox2dcollider.h"
//#include "lwfieldobject.h"
#include "lwfbo.h"
#include "lwtrail.h"
#include "armature.h"
#include "playersm.h"
#include "lwdeltatime.h"
#include "lwfieldrendercommand.h"
#include "lwbutton.h"
#include "construct.h"
#include "puckgamepacket.h"
#include "lwuniqueid.h"
#include "jsmn.h"
#include "lwdirpad.h"
#include "lwhostaddr.h"
#include "lwfvbo.h"
#include "lwfanim.h"
#include "lwcountry.h"
#include "lwchatringbuffer.h"
#define MAX_RENDER_QUEUE_CAPACITY (512)

// Vertex attributes: Coordinates (3xf) + Normal (3xf) + UV (2xf) + S9 (2xf)
// See Also: LWVERTEX
static const GLsizei lwvertex_stride_in_bytes = (GLsizei)(sizeof(float) * (3 + 3 + 2 + 2));
LwStaticAssert(sizeof(LWVERTEX) == (GLsizei)(sizeof(float) * (3 + 3 + 2 + 2)), "LWVERTEX size error");

// Color Vertex attributes: Coordinates (3xf) + Normal (3xf) + Color (3xf)
// See Also: LWCOLORVERTEX
static const GLsizei lwcolorvertex_stride_in_bytes = (GLsizei)(sizeof(float) * (3 + 3 + 3));
LwStaticAssert(sizeof(LWCOLORVERTEX) == (GLsizei)(sizeof(float) * (3 + 3 + 3)), "LWCOLORVERTEX size error");

// Skin Vertex attributes: Coordinates (3xf) + Normal (3xf) + UV (2xf) + Bone Weight (4xf) + Bone Matrix (4xi)
// See Also: LWSKINVERTEX
static const GLsizei skin_stride_in_bytes = (GLsizei)(sizeof(float) * (3 + 3 + 2 + 4) + sizeof(int) * 4);
LwStaticAssert(sizeof(LWSKINVERTEX) == (GLsizei)(sizeof(float) * (3 + 3 + 2 + 4) + sizeof(int) * 4), "LWSKINVERTEX size error");

// Fan Vertex attributes: Coordinates (3xf)
// See Also: LWFANVERTEX
static const GLsizei fan_stride_in_bytes = (GLsizei)(sizeof(float) * 3);
LwStaticAssert(sizeof(LWFANVERTEX) == (GLsizei)(sizeof(float) * 3), "LWFANVERTEX size error");

// Line Vertex attributes: Coordinates (2xf)
// See Also: LWLINEVERTEX
static const GLsizei line_stride_in_bytes = (GLsizei)(sizeof(float) * 2);
LwStaticAssert(sizeof(LWLINEVERTEX) == (GLsizei)(sizeof(float) * 2), "LWLINEVERTEX size error");

// Morph Vertex attributes: Coordinates_0 (3xf) + Coordinates_1 (3xf) + UV (2xf)
// See Also: LWMORPHVERTEX
static const GLsizei lwmorphvertex_stride_in_bytes = (GLsizei)(sizeof(float) * (3 + 3 + 2));
LwStaticAssert(sizeof(LWMORPHVERTEX) == (GLsizei)(sizeof(float) * (3 + 3 + 2)), "LWMORPHVERTEX size error");

#define MAX_ANIM_COUNT (10)
#define ANIM_FPS (60)
#define MAX_TOUCHPROC_COUNT (10)

extern const vec4 EXP_COLOR;

/* test7.fnt */
extern const char* tex_font_atlas_filename[2];
/* neo.fnt */
//extern const char* tex_font_atlas_filename[9];

#define MAX_TEX_FONT_ATLAS (ARRAY_SIZE(tex_font_atlas_filename))

#define MAX_FIELD_OBJECT (128)
#define MAX_BOX_COLLIDER MAX_FIELD_OBJECT
#define MAX_TRAIL (16)
#define SET_COLOR_RGBA_FLOAT(v, r, g, b, a) \
	v[0] = r; \
	v[1] = g; \
	v[2] = b; \
	v[3] = a;
#define SET_COLOR_RGBA_FLOAT_ARRAY(v, rgba) \
	v[0] = rgba[0]; \
	v[1] = rgba[1]; \
	v[2] = rgba[2]; \
	v[3] = rgba[3];
#define MAX_DAMAGE_TEXT (16)
#define MAX_PLAYER_SLOT (4)
#define MAX_ENEMY_SLOT (5)
#define VERTEX_BUFFER_COUNT LVT_COUNT
#define SKIN_VERTEX_BUFFER_COUNT LSVT_COUNT
#define FAN_VERTEX_BUFFER_COUNT LFVT_COUNT
#define PS_VERTEX_BUFFER_COUNT LPVT_COUNT
#define PS0_VERTEX_BUFFER_COUNT LP0VT_COUNT
#define LINE_VERTEX_BUFFER_COUNT (1)
#define MAX_DELTA_TIME_HISTORY (60)

#ifdef __cplusplus
extern "C" {;
#endif
typedef struct _LWPUCKGAME LWPUCKGAME;
typedef struct _LWUDP LWUDP;
typedef struct _LWTCP LWTCP;
typedef struct _LWPSCONTEXT LWPSCONTEXT;
typedef struct _LWTTL LWTTL;
typedef struct _LWCHAT LWCHAT;
typedef struct _LWSG LWSG;

typedef enum _LW_CONTROL_FLAGS {
    LCF_PUCK_GAME_RIGHT_DIR_PAD = 1 << 0,
    LCF_PUCK_GAME_DASH = 1 << 1,
    LCF_PUCK_GAME_JUMP = 1 << 2,
    LCF_PUCK_GAME_PULL = 1 << 3,
} LW_CONTROL_FLAGS;

typedef struct _LWCONTEXT {
	// Window instance
	struct GLFWwindow* window;
	// Window width
	int window_width;
	// Window height
	int window_height;
    // Window aspect ratio
    float window_aspect_ratio;
    // Window right top UI coordinate (calculated from aspect ratio)
    float window_rt_x;
    float window_rt_y;
    // Screen width
    int viewport_width;
    // Screen height
    int viewport_height;
    // Screen aspect ratio
    float viewport_aspect_ratio;
    // Right Top UI coordinate (calculated from aspect ratio)
    float viewport_rt_x;
    float viewport_rt_y;
    // Vertex shader array
    GLuint vertex_shader[LWVS_COUNT];
    // Fragment shader array
    GLuint frag_shader[LWFS_COUNT];
	// Shader program array
	LWSHADER shader[MAX_SHADER];
	// General VBO
	LWVBO vertex_buffer[VERTEX_BUFFER_COUNT];
	// Skinned mesh VBO
	LWVBO skin_vertex_buffer[SKIN_VERTEX_BUFFER_COUNT];
	// Aim sector(fan) VBO
	LWVBO fan_vertex_buffer[FAN_VERTEX_BUFFER_COUNT];
    // FVBO
    LWFVBO fvertex_buffer[LFT_COUNT];
    // Sea route VBO
    LWVBO sea_route_vbo;
    // Sea route VBO
    LWVBO morph_vertex_buffer[LMVT_COUNT];
    // FANIM (FVBO Anim)
    LWFANIM fanim[LFAT_COUNT];
	// General mesh VAO
	GLuint vao[VERTEX_BUFFER_COUNT];
    int vao_ready[VERTEX_BUFFER_COUNT];
    // General fracture VAO
    GLuint fvao[LFT_COUNT];
	// Skinned mesh VAO
	GLuint skin_vao[SKIN_VERTEX_BUFFER_COUNT];
	// Aim sector(fan) VAO
	GLuint fan_vao[FAN_VERTEX_BUFFER_COUNT];
	// Particle system VAO (EMITTER2)
	GLuint ps_vao[PS_VERTEX_BUFFER_COUNT];
	// Particle system VAO (EMITTER)
	GLuint ps0_vao[PS0_VERTEX_BUFFER_COUNT];
    // Line VAO
    GLuint line_vao[1];
    // Morph VAO
    GLuint morph_vao[LMVT_COUNT];
	// General texture atlas
	GLuint tex_atlas[MAX_TEX_ATLAS];
    int tex_atlas_ready[MAX_TEX_ATLAS];
	// Width for text atlas array
	int tex_atlas_width[MAX_TEX_ATLAS];
	// Height for text atlas array
	int tex_atlas_height[MAX_TEX_ATLAS];
	// Font texture atlas
	GLuint tex_font_atlas[MAX_TEX_FONT_ATLAS];
	// Hashes for tex atlas resource
	unsigned long tex_atlas_hash[MAX_TEX_ATLAS];
	// Textures created from code during runtime
	GLuint tex_programmed[MAX_TEX_PROGRAMMED];
	// Delta time tracker for logic update loop
	LWDELTATIME* update_dt;
	// Delta time tracker for rendering loop
	LWDELTATIME* render_dt;
	// Default projection matrix for UI
	mat4x4 proj;
	// Current scene
	LW_GAME_SCENE game_scene;
	// The scene going to be visible at the next tick
	LW_GAME_SCENE next_game_scene;
	// Logic updated count since app started
	int update_count;
	// Rendered count since app started
	int render_count;
	// last keypad number (debug)
	int kp;
	// Elapsed time since app started
	double app_time;
	// Elapsed time since enter the current scene
	double scene_time;
	// ???
	int tex_atlas_index;
	// ???
	LWSPRITE* sprite_data;
	// Font instance
	void* pFnt;
	// Dialog text data
	char* dialog;
	// Total dialog text data's length in bytes
	int dialog_bytelen;
	// Current page's current character index of dialog text
	int render_char;
	// Last time when the last character of dialog text is rendered
	double last_render_char_incr_time;
	// Current page's start index of dialog text
	int dialog_start_index;
	// 1 if next dialog input received, 0 if otherwise
	int dialog_move_next;
	// Dialog background image index
	int dialog_bg_tex_index;
	// Dialog portrait image index
	int dialog_portrait_tex_index;
	// 1 if player move left key is down, 0 if otherwise
	int player_move_left;
	// 1 if player move right key is down, 0 if otherwise
	int player_move_right;
	// 1 if player move up key is down, 0 if otherwise
	int player_move_up;
	// 1 if player move down key is down, 0 if otherwise
	int player_move_down;
	// 1 if player space key is down, 0 if otherwise
	int player_space;
	// Current player x coordinate
	float player_pos_x;
	// Current player y coordinate
	float player_pos_y;
	// Last valid player x coordinate move delta (value when the dir pad is released)
	float player_pos_last_moved_dx;
	// Last valid player y coordinate move delta (value when the dir pad is released)
	float player_pos_last_moved_dy;
	// Current player action
	const LWANIMACTION* player_action;
	//float player_aim_theta;
	LWPLAYERSTATEDATA player_state_data;
	// Current field event ID
	int field_event_id;
	// Next field event ID
	int next_field_event_id;
	// Battle: Normal attack effect instance array
	LWTRAIL trail[MAX_TRAIL];
	// Battle: Current FOV
	float battle_fov_deg;
	// Battle: FOV when normal state
	float battle_fov_deg_0;
	// Battle: FOV when camera is zoomed(magnified)
	float battle_fov_mag_deg_0;
	// Battle: Camera zooming x coordinate
	float battle_cam_center_x;
	// Battle: Current command slot index
	int selected_command_slot;
	// Battle: Current enemy slot index
	int selected_enemy_slot;
	// Battle: State
	LW_BATTLE_STATE battle_state;
	// Battle: Command banner
	LWANIM1D command_banner_anim;
	// Battle: Damage text instance array
	LWDAMAGETEXT damage_text[MAX_DAMAGE_TEXT];
	// Battle: Player slots
	LWBATTLECREATURE player[MAX_PLAYER_SLOT];
	// Battle: Enemy slots
	LWENEMY enemy[MAX_ENEMY_SLOT];
	// Battle: Current player turn
	int player_turn_creature_index;
	// Battle: Current enemy turn
	int enemy_turn_creature_index;
	// Battle: Delay before executing the enemy action (mimics thinking time of human)
	float enemy_turn_command_wait_time;
	// Battle: Wall UV animation (v coordinate)
	float battle_wall_tex_v;
	// Battle: Projection amtrix
	mat4x4 battle_proj;
	// Battle: View matrix
	mat4x4 battle_view;
	// Battle: Turn change UI animator
	LWANIM1D center_image_anim;
	// Battle: Turn change UI image
	LW_ATLAS_ENUM center_image;
	// Font: Render font debug UI
	int font_texture_texture_mode;
	// Shared full screen FBO
	LWFBO shared_fbo;
	// Input: Last mouse press x coordinate
	float last_mouse_press_x;
	// Input: Last mouse press y coordinate
	float last_mouse_press_y;
	// Input: Last mouse move x coordinate
	float last_mouse_move_x;
	// Input: Last mouse move y coordinate
	float last_mouse_move_y;
	// Input: Last mouse move delta x
	float last_mouse_move_delta_x;
	// Input: Last mouse move delta y
	float last_mouse_move_delta_y;
	// Admin button command handler array
	LWBUTTONCOMMAND admin_button_command[6 * 6];
	// lua VM instance
	void* L;
	void* script;
	// Skin: Armature data
	LWARMATURE armature[LWAR_COUNT];
	// Skin: Anim action data
	LWANIMACTION action[LWAC_COUNT];
	// Abstract time for player animation
	float player_skin_time;
	// Abstract time for test player animation
	float test_player_skin_time;
	// 1 if player animation action is looped, 0 if otherwise
	int player_action_loop;
	// Field instance
	struct _LWFIELD* field;
	// 1 if FPS camera mode enabled, 0 if otherwise
	int fps_mode;
	// 1 if field mesh hidden, 0 if otherwise
	int hide_field;
	// Default system message instance
	void* def_sys_msg;
	// Message queue instance
	void* mq;
	// Server index for online play
	int server_index;
	// 1 if aim sector ray test is executed for every logic frame, 0 if ray test disabled
	int ray_test;
	// 1 if application quit is requested
	int quit_request;
	// CZMQ actor for logic thread
	void* logic_actor;
	// CZMQ actor for script watch thread (win32 dev only)
	void* scriptwatch_actor;
	// Particle system buffer (rose)
	GLuint particle_buffer;
	// Particle system buffer (explosion)
	GLuint particle_buffer2;
    // Logic update frequency (in Hz)
    int update_frequency;
	// Logic update interval (in seconds)
	double update_interval;
	// 1 if safe to render, 0 if otherwise
	/*volatile*/ int safe_to_start_render;
	// 1 if rendering in progress, 0 if otherwise
	/*volatile*/ int rendering;
	// Asynchronous render command queue
	LWFIELDRENDERCOMMAND render_command[MAX_RENDER_QUEUE_CAPACITY];
	// Render command message send count
	int rmsg_send_count;
	// Render command message recv(handle) count
	int rmsg_recv_count;
	// ZeroMQ logic loop
	void* logic_loop;
	// ZeroMQ logic update timer job
	int logic_update_job;
	// Last now (seconds)
	double last_now;
	// Button list
	LWBUTTONLIST button_list;
    char pressed_button_id[LW_UI_IDENTIFIER_LENGTH];
	// Construct
	LWCONSTRUCT construct;
	// Puck game sub-context
	LWPUCKGAME* puck_game;
	// UDP context
	LWUDP* udp;
	// TCP context (ball rumble)
	LWTCP* tcp;
    // TCP context (transport tycoon lee)
    LWTCP* tcp_ttl;
	// Puck game remote(server) state
	LWPSTATE puck_game_state;
	// Puck game remote state last received time (sec)
	double puck_game_state_last_received;
	// Last state packet reception interval (msec)
	double puck_game_state_last_received_interval;
    // Puck game remote(server) state
    //LWPSTATE puck_game_state2;
    // Puck game remote state last received time (sec)
    double puck_game_state2_last_received;
    // Last state packet reception interval (msec)
    double puck_game_state2_last_received_interval;
	const char* internal_data_path;
    const char* user_data_path;
	mat4x4 puck_game_view;
	mat4x4 puck_game_proj;
	LWHOSTADDR tcp_host_addr;
	LWHOSTADDR udp_host_addr;
    LWHOSTADDR tcp_ttl_host_addr;
    LWHOSTADDR sea_udp_host_addr;
    LWHOSTADDR sea_tcp_host_addr;
	int last_text_input_seq;
    LWDIRPAD left_dir_pad;
    LWDIRPAD right_dir_pad;
    LWPLEADERBOARD last_leaderboard;
    int control_flags;
    LWATLASSPRITEARRAY atlas_conf[LAC_COUNT];
	void* android_native_activity;
    char device_model[64];
    int lowend_device;
    int argc;
    char** argv;
    int show_stat;
    int viewport_x;
    int viewport_y;
    LW_VBO_TYPE puck_game_stage_lvt;
    LW_ATLAS_ENUM puck_game_stage_lae;
    vec3 eye;
    vec3 center;
    vec3 up;
    float eye_x_offset;
	void* htmlui; // LWHTMLUI
    LWPSCONTEXT* ps_context; // LWPSCONTEXT
    LWTTL* ttl;
    LWCOUNTRYARRAY country_array;
    int country_page;
    void* remtex;
    LWCHATRINGBUFFER chat_ring_buffer;
    int show_chat_window;
    int focus_chat_input;
    LWSG* sg;
} LWCONTEXT;

double lwcontext_delta_time(const LWCONTEXT* pLwc);
int lwcontext_safe_to_start_render(const LWCONTEXT* pLwc);
void lwcontext_set_safe_to_start_render(LWCONTEXT* pLwc, int v);
int lwcontext_rendering(const LWCONTEXT* pLwc);
void lwcontext_set_rendering(LWCONTEXT* pLwc, int v);
void* lwcontext_mq(LWCONTEXT* pLwc);
LWFIELD* lwcontext_field(LWCONTEXT* pLwc);
void lwcontext_inc_rmsg_send(LWCONTEXT* pLwc);
void lwcontext_inc_rmsg_recv(LWCONTEXT* pLwc);
void lwcontext_set_custom_puck_game_stage(LWCONTEXT* pLwc, LW_VBO_TYPE lvt, LW_ATLAS_ENUM lae);
void lwcontext_set_update_frequency(LWCONTEXT* pLwc, int hz);
float lwcontext_update_interval(LWCONTEXT* pLwc);
int lwcontext_update_frequency(LWCONTEXT* pLwc);
void lwcontext_rt_corner(const float aspect_ratio, float* x, float* y);
#ifdef __cplusplus
};
#endif
