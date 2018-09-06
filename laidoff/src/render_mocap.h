#pragma once
#ifdef __cplusplus
extern "C" {;
#endif
typedef struct _LWCONTEXT LWCONTEXT;
void lwc_render_mocap_fbo_body(const LWCONTEXT* pLwc, const char* html_body);
void lwc_render_mocap(const LWCONTEXT* pLwc);
#ifdef __cplusplus
}
#endif
