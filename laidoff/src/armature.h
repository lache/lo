#pragma once

#include "linmath.h"
#include "lwmacro.h"

typedef struct _LWARMATURE {
	int count;
	mat4x4* mat;
	int* parent_index;
	int* use_connect;
	char* d;
} LWARMATURE;

typedef enum _LW_ARMATURE {
	LWAR_TRIANGLEARMATURE,
	LWAR_TREEPLANEARMATURE,
	LWAR_HUMANARMATURE,
	LWAR_DETACHPLANEARMATURE,
	LWAR_GUNTOWER_ARMATURE,
	LWAR_TURRET_ARMATURE,
	LWAR_CROSSBOW_ARMATURE,
	LWAR_CATAPULT_ARMATURE,
	LWAR_PYRO_ARMATURE,

	LWAR_COUNT,
} LW_ARMATURE;

static const char* armature_filename[] = {
	ASSETS_BASE_PATH "armature" PATH_SEPARATOR "TriangleArmature.arm",
	ASSETS_BASE_PATH "armature" PATH_SEPARATOR "TreePlaneArmature.arm",
	ASSETS_BASE_PATH "armature" PATH_SEPARATOR "HumanArmature.arm",
	ASSETS_BASE_PATH "armature" PATH_SEPARATOR "DetachPlaneArmature.arm",
	ASSETS_BASE_PATH "armature" PATH_SEPARATOR "guntower-armature.arm",
	ASSETS_BASE_PATH "armature" PATH_SEPARATOR "turret-armature.arm",
	ASSETS_BASE_PATH "armature" PATH_SEPARATOR "crossbow-armature.arm",
	ASSETS_BASE_PATH "armature" PATH_SEPARATOR "catapult-armature.arm",
	ASSETS_BASE_PATH "armature" PATH_SEPARATOR "pyro-armature.arm",
};

int load_armature(const char* filename, LWARMATURE* ar);
void unload_armature(LWARMATURE* ar);
