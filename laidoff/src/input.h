#pragma once
#ifdef __cplusplus
extern "C" {;
#endif
typedef struct _LWCONTEXT LWCONTEXT;

void lw_trigger_touch(LWCONTEXT* pLwc, float nx, float ny, int pointer_id);
void lw_trigger_mouse_press(LWCONTEXT* pLwc, float nx, float ny, int pointer_id);
void lw_trigger_mouse_release(LWCONTEXT* pLwc, float nx, float ny, int pointer_id);
void lw_trigger_mouse_move(LWCONTEXT* pLwc, float nx, float ny, int pointer_id);
void lw_trigger_reset(LWCONTEXT* pLwc);
void lw_trigger_play_sound(LWCONTEXT* pLwc);
void lw_trigger_key_left(LWCONTEXT* pLwc);
void lw_trigger_key_right(LWCONTEXT* pLwc);
void lw_trigger_key_enter(LWCONTEXT* pLwc);
void lw_press_key_left(LWCONTEXT* pLwc);
void lw_press_key_right(LWCONTEXT* pLwc);
void lw_press_key_up(LWCONTEXT* pLwc);
void lw_press_key_down(LWCONTEXT* pLwc);
void lw_press_key_space(LWCONTEXT* pLwc);
void lw_press_key_a(LWCONTEXT* pLwc);
void lw_press_key_z(LWCONTEXT* pLwc);
void lw_press_key_x(LWCONTEXT* pLwc);
void lw_press_key_q(LWCONTEXT* pLwc);
void lw_press_key_w(LWCONTEXT* pLwc);
void lw_release_key_left(LWCONTEXT* pLwc);
void lw_release_key_right(LWCONTEXT* pLwc);
void lw_release_key_up(LWCONTEXT* pLwc);
void lw_release_key_down(LWCONTEXT* pLwc);
void lw_release_key_space(LWCONTEXT* pLwc);
void lw_release_key_a(LWCONTEXT* pLwc);
void lw_release_key_z(LWCONTEXT* pLwc);
void lw_release_key_x(LWCONTEXT* pLwc);
void lw_release_key_q(LWCONTEXT* pLwc);
void lw_go_back(LWCONTEXT* pLwc, void* native_context);
void lw_trigger_scroll(LWCONTEXT* pLwc, float xoffset, float yoffset);
int lw_pinch();
#ifdef __cplusplus
};
#endif