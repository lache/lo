#pragma once

#include "lwcontext.h"
#include "lwvbotype.h"
#include "lwatlasenum.h"
#include "lwatlassprite.h"

#define MAX_COMMAND_SLOT (6)

#ifdef __cplusplus
extern "C" {;
#endif

typedef struct _LWCONTEXT LWCONTEXT;
typedef struct GLFWwindow GLFWwindow;

LWCONTEXT* lw_init_initial_size(int width, int height);
LWCONTEXT* lw_init(void);
void lw_deinit(LWCONTEXT* pLwc);
void lw_set_window_size(LWCONTEXT* pLwc, int w, int h);
void lw_set_viewport_size(LWCONTEXT* pLwc, int w, int h);
void lw_set_window(LWCONTEXT* pLwc, struct GLFWwindow* window);
GLFWwindow* lw_get_window(const LWCONTEXT* pLwc);
void lwc_prerender_mutable_context(LWCONTEXT* pLwc);
void lwc_render(const LWCONTEXT* pLwc);
int lw_get_update_count(LWCONTEXT* pLwc);
int lw_get_render_count(LWCONTEXT* pLwc);
void lw_on_destroy(LWCONTEXT* pLwc);
void lw_clear_color();
void lw_set_kp(LWCONTEXT* pLwc, int kp);

#ifndef __cplusplus
enum _LW_ATLAS_ENUM;
enum _LW_ATLAS_SPRITE;
#define LWENUM enum
#else
#define LWENUM
#endif

void set_tex_filter(int min_filter, int mag_filter);

void bind_all_fvertex_attrib(const LWCONTEXT* pLwc, int fvbo_index);
void bind_all_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index);
void bind_all_color_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index);
void bind_all_vertex_attrib_font(const LWCONTEXT* pLwc, int vbo_index, int shader_index);
void bind_all_vertex_attrib_etc1_with_alpha(const LWCONTEXT* pLwc, int vbo_index);
void bind_all_skin_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index);
void bind_all_fan_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index);
void bind_all_ps_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index);
void bind_all_ps0_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index);
void bind_all_line_vertex_attrib(const LWCONTEXT* pLwc);
void bind_all_morph_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index);

void set_texture_parameter_values(const LWCONTEXT* pLwc, float x, float y, float w, float h, float atlas_w, float atlas_h, int shader_index);
int get_tex_index_by_hash_key(const LWCONTEXT* pLwc, const char* hash_key);
void set_texture_parameter(const LWCONTEXT* pLwc, LWENUM _LW_ATLAS_ENUM lae, LWENUM _LW_ATLAS_SPRITE las);

void lw_app_quit(LWCONTEXT* pLwc, void* native_context);
unsigned long hash(const unsigned char *str);
void reset_battle_context(LWCONTEXT* pLwc);
void lwc_start_logic_thread(LWCONTEXT* pLwc);
void delete_all_rmsgs(LWCONTEXT* pLwc);
void lw_start_text_input_activity(LWCONTEXT* pLwc, int tag); // Native text input activity
void lw_set_device_model(LWCONTEXT* pLwc, const char* model);
void lw_flag_logic_actor_to_quit_and_wait(LWCONTEXT* pLwc);
void set_vertex_attrib_pointer(const LWCONTEXT* pLwc, int shader_index);
void set_color_vertex_attrib_pointer(const LWCONTEXT* pLwc, int shader_index);
void load_png_pkm_sw_decoding(LWCONTEXT* pLwc, int i);
void set_package_version(const char* package_version);
void load_pkm_hw_decoding(const char *tex_atlas_filename);

extern const float default_uv_offset[2];
extern const float default_uv_scale[2];
extern const float default_flip_y_uv_scale[2];

#ifdef __cplusplus
};
#endif
