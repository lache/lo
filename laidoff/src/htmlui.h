#pragma once

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct _LWCONTEXT LWCONTEXT;
	int test_html_ui(LWCONTEXT* pLwc);
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
#ifdef __cplusplus
}
#endif
