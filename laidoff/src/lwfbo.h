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
