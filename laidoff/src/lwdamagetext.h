#pragma once

#include "lwtextblock.h"
#include "linmath.h"

typedef struct _LWCONTEXT LWCONTEXT;

typedef enum _LW_DAMAGE_TEXT_COORD {
	LDTC_3D,
	LDTC_UI,
} LW_DAMAGE_TEXT_COORD;

typedef struct _LWDAMAGETEXT
{
	int valid;
	float x, y, z;
	float age;
	float max_age;
	char text[32];
	LWTEXTBLOCK text_block;
	LW_DAMAGE_TEXT_COORD coord;
} LWDAMAGETEXT;

int spawn_damage_text(LWCONTEXT* pLwc, float x, float y, float z, const char *text, LW_DAMAGE_TEXT_COORD coord);
int spawn_exp_text(LWCONTEXT* pLwc, float x, float y, float z, const char *text, LW_DAMAGE_TEXT_COORD coord);
void update_damage_text(LWCONTEXT* pLwc);
void render_damage_text(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, const mat4x4 ui_proj, float ui_alpha);