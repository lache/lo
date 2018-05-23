#pragma once
#include "lwatlasenum.h"
#include "lwvbotype.h"
#include "armature.h"
#include "lwanim.h"

typedef struct _LWCONSTRUCTPREVIEW {
	LW_ATLAS_ENUM atlas;
	LW_SKIN_VBO_TYPE skin_vbo;
	LW_ARMATURE armature;
	LW_ACTION anim_action_id;
} LWCONSTRUCTPREVIEW;

typedef struct _LWCONSTRUCT {
	int preview_enable;
	LWCONSTRUCTPREVIEW preview;
} LWCONSTRUCT;

void construct_set_preview_enable(LWCONSTRUCT* construct, int enable);
void construct_set_preview(LWCONSTRUCT* construct, LW_ATLAS_ENUM atlas, LW_SKIN_VBO_TYPE skin_vbo, LW_ARMATURE armature, LW_ACTION anim_action_id);
