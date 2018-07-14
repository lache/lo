#pragma once
#ifdef __cplusplus
extern "C" {;
#endif
#define SCRIPT_UNNAMED_SCRIPT_NAME "<Unnamed Script>"
void init_lua(LWCONTEXT* pLwc);
void spawn_all_field_object(LWCONTEXT* pLwc);
int script_run_file_ex(LWCONTEXT* pLwc, const char* filename, int pop_result);
void script_run_file(LWCONTEXT* pLwc, const char* filename);
void script_set_context(LWCONTEXT* pLwc);
LWCONTEXT* script_context();
const char* script_prefix_path();
void script_update(LWCONTEXT* pLwc);
void script_cleanup_all_coros(LWCONTEXT* pLwc);
int script_emit_anim_marker(void* L, int key, const char* name);
int script_emit_near(void* L, int key1, int key2);
int script_emit_logic_frame_finish(void* L, float dt);
int script_emit_ui_event(void* L, const char* id, float w_ratio, float h_ratio);
void script_get_string(void* L, const char* id, char* ret, int ret_max_len);
void script_on_near_puck_player(void* _script, int dashing);
void script_on_player_attack(void* _script);
void script_on_target_attack(void* _script);
void script_evaluate_with_name(void* L, const char* code, size_t code_len, const char* name);
void script_evaluate_with_name_async(LWCONTEXT* pLwc, const char* code, size_t code_len, const char* name);
void script_evaluate(void* L, const char* code, size_t code_len);
void script_evaluate_async(LWCONTEXT* pLwc, const char* code, size_t code_len);
const char* script_full_asset_path(const char* asset_type, const char* asset_name);
int script_http_header(void* L, char* header, size_t header_max_len);
void script_emit_single_string_arg(void* L, const char* func_name, const char* str_arg, char* output);
#ifdef __cplusplus
};
#endif
