#pragma once
#ifdef __cplusplus
extern "C" {;
#endif
typedef struct _LWCONTEXT LWCONTEXT;
void lwc_init_dynamic_vbo(LWCONTEXT* pLwc);
void lwc_render_dynamic_vbo(const LWCONTEXT* pLwc);
#ifdef __cplusplus
}
#endif
