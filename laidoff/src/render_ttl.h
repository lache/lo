#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _LWCONTEXT LWCONTEXT;

void lwc_render_ttl_fbo_body(const LWCONTEXT* pLwc, const char* html_body);
void lwc_render_ttl_fbo(const LWCONTEXT* pLwc, const char* html_path);
void lwc_render_ttl(const LWCONTEXT* pLwc);
int lwc_render_ttl_render(const char* name);
void lwc_render_ttl_enable_render(const char* name, int v);

int lwc_prerender_ttl_fbo(const LWCONTEXT* pLwc);
void lwc_postrender_ttl_fbo(const LWCONTEXT* pLwc);

#ifdef __cplusplus
}
#endif
