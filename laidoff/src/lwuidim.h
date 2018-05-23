#pragma once
#include "lwuialign.h"

typedef struct _LWUIDIM {
	LW_UI_ALIGN align;
	float x;
	float y;
	float w;
	float h;
} LWUIDIM;

void lwuidim_convert_align(const LWUIDIM* d, const LW_UI_ALIGN align, LWUIDIM* r);
int lwuidim_contains(const LWUIDIM* d, float x, float y);
