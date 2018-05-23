#pragma once

#include "lwgl.h"

typedef struct _LWFIELDOBJECT {
	int valid;
	float x, y, z;
	float sx, sy;
	float rot_z;
	enum _LW_VBO_TYPE lvt;
	GLuint tex_id;
	float alpha_multiplier;
	int skin;
} LWFIELDOBJECT;
