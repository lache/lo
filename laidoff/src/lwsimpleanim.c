#include <string.h>

#include "lwsimpleanim.h"

float lwanim_get_1d(const LWANIM1D* a) {
	if (a->t <= 0) {
		return 0;
	}
	else if (a->t <= a->max_t / 2) {
		return a->max_v / (a->max_t / 2) * a->t;
	}
	else if (a->t <= a->max_t) {
		return -(a->max_v) / (a->max_t / 2) * (a->t - a->max_t);
	}

	return 0;
}

void vector_interp(const int dim, const float* v0, const float* v1, const float r, float* out) {
	for (int i = 0; i < dim; i++) {
		out[i] = (1.0f - r) * v0[i] + r * v1[i];
	}
}

void lwanim_get_5d(const LWANIM5D* a, float out[5]) {
	const int dim = 5;
	//const float v = lwanim_get_1d(&a->anim_1d);
	if (a->anim_1d.t <= 0) {
		memcpy(out, a->v0, sizeof(float) * dim);
		return;
	}
	else if (a->anim_1d.t <= a->anim_1d.max_t / 2) {
		vector_interp(dim, a->v0, a->v1, a->anim_1d.t / a->anim_1d.max_t, out);
		return;
	}
	else if (a->anim_1d.t <= a->anim_1d.max_t) {
		vector_interp(dim, a->v1, a->v2, 1.0f - a->anim_1d.t / a->anim_1d.max_t, out);
		return;
	}

	memcpy(out, a->v2, sizeof(float) * dim);
}
