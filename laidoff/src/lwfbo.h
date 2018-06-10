#pragma once

#include "lwgl.h"

typedef struct _LWFBO {
	GLuint fbo;
    // FBO texture width resolution
    int tex_width;
    // FBO texture height resolution
    int tex_height;
    // FBO visible(rendered) width resolution
    int width;
    // FBO visible(rendered) height resolution
    int height;
	GLuint depth_render_buffer;
	GLuint color_tex;
	int dirty;
} LWFBO;
#ifdef __cplusplus
extern "C" {;
#endif
void lwfbo_init(LWFBO* fbo, int width, int height);
void lwfbo_delete(LWFBO* fbo);
int lwfbo_prerender(const LWCONTEXT* pLwc, const LWFBO* fbo);
void lwfbo_postrender(const LWCONTEXT* pLwc, const LWFBO* fbo);
#ifdef __cplusplus
}
#endif
