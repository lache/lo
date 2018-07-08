#pragma once
#include "linmath.h"
#ifdef __cplusplus
extern "C" {;
#endif
typedef struct _LWCONTEXT LWCONTEXT;
void* htmlui_new(LWCONTEXT* pLwc);
void htmlui_destroy(void** c);
void htmlui_load_render_draw(void* c, const char* html_path);
void htmlui_load_render_draw_body(void* c, const char* html_body);
void htmlui_on_lbutton_down(void* c, float nx, float ny);
void htmlui_on_lbutton_up(void* c, float nx, float ny);
void htmlui_on_over(void* c, float nx, float ny);
void htmlui_set_next_html_path(void* c, const char* html_path);
void htmlui_load_next_html_path(void* c);
void htmlui_set_refresh_html_body(void* c, int v);
void htmlui_load_next_html_body(void* c);
void htmlui_clear_loop(void* c, const char* loop_name);
void htmlui_set_loop_key_value(void* c, const char* loop_name, const char* key, const char* value);
void htmlui_set_online(void* c, int b);
void htmlui_update_country_data(const LWCONTEXT* pLwc, void* c);
void htmlui_load_redraw_fbo(void* c);
void htmlui_on_remtex_gpu_loaded(void* c, unsigned int name_hash);
void htmlui_set_client_size(void* c, int client_width, int client_height);
int htmlui_over_element(void* c, float nx, float ny);
int htmlui_get_refresh_html_body(void* c);
void htmlui_execute_anchor_click(void* c, const char* url);
void htmlui_add_touch_rect(void* c, float x, float y, float z, float width, float height, float extend_width, float extend_height, const mat4x4 view, const mat4x4 proj);
int htmlui_get_touch_rect_count(void* c);
void htmlui_get_touch_rect(void* c, int index, double* start, float* x, float* y, float* z, float* width, float* height, float* extend_width, float* extend_height, mat4x4 view, mat4x4 proj);
void htmlui_render_render_commands(void* c);
void htmlui_update_on_render_thread(void* c);
#ifdef __cplusplus
}
#endif
