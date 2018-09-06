#pragma once
#include "lwgl.h"
#include "lwvbotype.h"
#include "linmath.h"

typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWTCP LWTCP;

typedef struct _LWFIELDMESH {
    LW_VBO_TYPE vbo;
    GLuint tex_id;
    int tex_mip;
} LWFIELDMESH;
#ifdef __cplusplus
extern "C" {;
#endif
void lwc_update(LWCONTEXT* pLwc, double delta_time);
void reset_runtime_context(LWCONTEXT* pLwc);
void reset_time(LWCONTEXT* pLwc);
const char* logic_server_addr(int idx);
void logic_update_default_ui_proj(const int width, const int height, mat4x4 proj);
void logic_update_default_ui_proj_for_htmlui(const int width, const int height, mat4x4 proj);
void reset_runtime_context(LWCONTEXT* pLwc);
void reset_runtime_context_async(LWCONTEXT* pLwc);
void logic_start_logic_update_job(LWCONTEXT* pLwc);
void logic_stop_logic_update_job(LWCONTEXT* pLwc);
void logic_start_logic_update_job_async(LWCONTEXT* pLwc);
void logic_stop_logic_update_job_async(LWCONTEXT* pLwc);
void logic_emit_ui_event_async(LWCONTEXT* pLwc, const char* id, float w_ratio, float h_ratio);
void logic_emit_evalute_with_name_async(LWCONTEXT* pLwc, const char* code, size_t code_len, const char* name);
void logic_emit_evalute_async(LWCONTEXT* pLwc, const char* code, size_t code_len);
void load_field_1_init_runtime_data(LWCONTEXT* pLwc);
void load_field_2_init_runtime_data(LWCONTEXT* pLwc);
void load_field_3_init_runtime_data(LWCONTEXT* pLwc);
void load_field_4_init_runtime_data(LWCONTEXT* pLwc);
void load_field_5_init_runtime_data(LWCONTEXT* pLwc);
void change_to_field(LWCONTEXT* pLwc);
void change_to_dialog(LWCONTEXT* pLwc);
void change_to_battle(LWCONTEXT* pLwc);
void change_to_font_test(LWCONTEXT* pLwc);
void change_to_ttl(LWCONTEXT* pLwc);
void change_to_mocap(LWCONTEXT* pLwc);
void change_to_gazza(LWCONTEXT* pLwc);
void change_to_dynamic_vbo(LWCONTEXT* pLwc);
void change_to_admin(LWCONTEXT* pLwc);
void change_to_battle_result(LWCONTEXT* pLwc);
void change_to_skin(LWCONTEXT* pLwc);
void change_to_puck_game(LWCONTEXT* pLwc);
void change_to_particle_system(LWCONTEXT* pLwc);
void connect_to_server_0(LWCONTEXT* pLwc);
void connect_to_server_1(LWCONTEXT* pLwc);
void change_to_leaderboard(LWCONTEXT* pLwc);
void change_to_puck_game(LWCONTEXT* pLwc);
void request_player_reveal_leaderboard(LWTCP* tcp, int items_in_page);
void request_leaderboard(LWTCP* tcp, int items_in_page, int one_based_page);
void start_nickname_text_input_activity(LWCONTEXT* pLwc);
void start_chat_text_input_activity(LWCONTEXT* pLwc);
void lw_first_page(LWCONTEXT* pLwc);
void lw_prev_page(LWCONTEXT* pLwc);
void lw_next_page(LWCONTEXT* pLwc);
void lw_last_page(LWCONTEXT* pLwc);
void lw_new_tcp_ttl(LWCONTEXT* pLwc);
void lw_new_tcp_ttl_custom(LWCONTEXT* pLwc, const char* host, const char* port_str, int port);
#ifdef __cplusplus
};
#endif
