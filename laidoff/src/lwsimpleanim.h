#pragma once

typedef struct _LWANIM1D {
	float t;
	float max_t;
	float max_v;
} LWANIM1D;

float lwanim_get_1d(const LWANIM1D* a);

typedef struct _LWANIM5D {
	float v0[5];
	float v1[5];
	float v2[5];
	LWANIM1D anim_1d;
} LWANIM5D;

void lwanim_get_5d(const LWANIM5D* a, float out[5]);