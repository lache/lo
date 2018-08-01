#pragma once
#ifdef __cplusplus
extern "C" {;
#endif
typedef struct _LWCONTEXT LWCONTEXT;
void lwc_init_tilemap(const LWCONTEXT* pLwc);
void lwc_render_tilemap(const LWCONTEXT* pLwc);
void lwc_destroy_tilemap();
#ifdef __cplusplus
}
#endif
