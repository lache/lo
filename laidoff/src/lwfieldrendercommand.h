#pragma once

#include "linmath.h"
#include "lwatlasenum.h"
#include "lwvbotype.h"
#include "armature.h"

typedef enum _LW_RENDER_COMMAND_TYPE {
	LRCT_SPAWN,
	LRCT_DESPAWN,
	LRCT_POS,
	LRCT_TURN,
	LRCT_ANIM,
	LRCT_RPARAMS,
	LRCT_BULLETSPAWNHEIGHT,
    LRCT_QUITAPP, // render thread == main thread
    LRCT_LOADTEX,
    LRCT_STARTTEXTINPUTACTIVITY,
    LRCT_REDRAWUIFBO,
} LW_RENDER_COMMAND_TYPE;

typedef struct _LWFIELDRENDERCOMMAND {
	// Command type
	LW_RENDER_COMMAND_TYPE cmdtype;
	// Object key (issued in field.lua)
	int key;
	// Object type
	int objtype;
	// XYZ position
	vec3 pos;
	// XYZ scale
	vec3 scale;
	// Face direction
	float angle;
	// Animation action ID
	int actionid;
	// Animation start time
	double animstarttime;
	// 1 if loop animation mode
	int loop;
	// Texture atlas to render
	LW_ATLAS_ENUM atlas;
	// Skin VBO enum to render
	LW_SKIN_VBO_TYPE skin_vbo;
	// Armature enum to use
	LW_ARMATURE armature;
	// Bullet spawn height
	float bullet_spawn_height;
	// VBO
	LW_VBO_TYPE vbo;
	// Starting anim marker index for scanning events
	int anim_marker_search_begin;
    // native context
    void* native_context;
    int tag;
} LWFIELDRENDERCOMMAND;

double rendercommand_animtime(const LWFIELDRENDERCOMMAND* rcmd, double now);
