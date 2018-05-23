#pragma once

#include "lwgl.h"
#include "lwfanim.h"
#include "linmath.h"

typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWPUCKGAME LWPUCKGAME;

typedef enum _LW_FVBO_TYPE {
    LFT_TOWER,
    LFT_COUNT,
} LW_FVBO_TYPE;

#define LWFVBO_MAX_CELL_COUNT (32)
// Cell fracture objects (multible VBOs in a single file)
typedef struct _LWFVBO
{
	GLuint vertex_buffer;
	int total_vertex_count;
    int total_cell_count;
    int vertex_count_per_cell[LWFVBO_MAX_CELL_COUNT];
} LWFVBO;

typedef struct _LWFVBO_FILE_LAYOUT {
    int total_cell_object_count;
    int* num_vertices; // array length: total_cell_object_count
    float* buffer_data;
} LWFVBO_FILE_LAYOUT;

void load_fvbo(LWCONTEXT* pLwc, const char* filename, LWFVBO* fvbo);
void render_fvbo(const LWCONTEXT* pLwc, const LWPUCKGAME* puck_game, const mat4x4 view, const mat4x4 proj,
                 LW_FVBO_TYPE lft, LW_F_ANIM_TYPE lfat, float x, float y, float z, float scale, float z_rot_angle,
                 float frame_time, int loop, float frames_per_sec);
