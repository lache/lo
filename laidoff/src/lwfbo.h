#pragma once

#include "lwgl.h"

typedef struct _LWFBO {
	GLuint fbo;
	int width;
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
int lwfbo_prerender(const LWFBO* fbo);
void lwfbo_postrender(const LWFBO* fbo);
#ifdef __cplusplus
}
#endif
