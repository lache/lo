#pragma once

#include "linmath.h"

typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWPUCKGAME LWPUCKGAME;

void lwc_render_puck_game(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj);
void mult_world_roll(float aspect_ratio, mat4x4 model, int axis, int dir, float angle);
void calculate_world_right_top_end_ui_point(const LWCONTEXT* pLwc, const LWPUCKGAME* puck_game, vec2 world_right_top_end_ui_point);
