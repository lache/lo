#pragma once

#include "lwgl.h"

typedef struct _LWVBO
{
	GLuint vertex_buffer;
	int vertex_count;
} LWVBO;

void lw_load_vbo_data(LWCONTEXT* pLwc, const char* mesh_vbo_data, size_t mesh_size, LWVBO* pVbo, int stride_in_bytes);
void lw_load_vbo(LWCONTEXT* pLwc, const char* filename, LWVBO* pVbo, int stride_in_bytes);
void lw_load_all_vbo(LWCONTEXT* pLwc);
void lazy_glBindBuffer(const LWCONTEXT* pLwc, int lvt);
void lw_setup_vao(LWCONTEXT* pLwc, int lvt);