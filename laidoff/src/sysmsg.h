#pragma once
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _LWCONTEXT LWCONTEXT;

void *init_sys_msg();

void deinit_sys_msg(void *_sm);

void show_sys_msg(void *_sm, const char *msg);

void update_sys_msg(void *_sm, float delta_time);

void render_sys_msg(const LWCONTEXT *pLwc, void *_sm);

#ifdef __cplusplus
}
#endif
