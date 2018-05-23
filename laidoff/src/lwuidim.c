#include "lwuidim.h"
#include "lwuialign.h"

void convert_to_left_top(const LWUIDIM* d, LWUIDIM* r) {
	if (d->align == LTBA_LEFT_TOP) {
		*r = *d;
		return;
	}

	const LW_UI_ALIGN oalign = d->align;

	r->align = LTBA_LEFT_TOP;
	r->w = d->w;
	r->h = d->h;

	float left_top_x = d->x;
	float left_top_y = d->y;

	// Horizontal axis

	if (oalign == LTBA_CENTER_BOTTOM
		|| oalign == LTBA_CENTER_CENTER
		|| oalign == LTBA_CENTER_TOP) {
		left_top_x -= r->w / 2;
	}

	if (oalign == LTBA_LEFT_BOTTOM
		|| oalign == LTBA_LEFT_CENTER
		|| oalign == LTBA_LEFT_TOP) {
		// do nothing on left_top_x
	}

	if (oalign == LTBA_RIGHT_BOTTOM
		|| oalign == LTBA_RIGHT_CENTER
		|| oalign == LTBA_RIGHT_TOP) {
		left_top_x -= r->w;
	}

	// Vertical axis

	if (oalign == LTBA_CENTER_BOTTOM
		|| oalign == LTBA_LEFT_BOTTOM
		|| oalign == LTBA_RIGHT_BOTTOM) {
		left_top_y += r->h;
	}

	if (oalign == LTBA_CENTER_CENTER
		|| oalign == LTBA_LEFT_CENTER
		|| oalign == LTBA_RIGHT_CENTER) {
		left_top_y += r->h / 2;
	}

	if (oalign == LTBA_CENTER_TOP
		|| oalign == LTBA_LEFT_TOP
		|| oalign == LTBA_RIGHT_TOP) {
		// do nothing on left_top_y
	}

	r->x = left_top_x;
	r->y = left_top_y;
}


void lwuidim_convert_align(const LWUIDIM* d, const LW_UI_ALIGN align, LWUIDIM* r) {

	convert_to_left_top(d, r);

	// 'r' is left-top mode for now.

	float x = r->x;
	float y = r->y;

	// Horizontal axis

	if (align == LTBA_CENTER_BOTTOM
		|| align == LTBA_CENTER_CENTER
		|| align == LTBA_CENTER_TOP) {
		x += r->w / 2;
	}

	if (align == LTBA_LEFT_BOTTOM
		|| align == LTBA_LEFT_CENTER
		|| align == LTBA_LEFT_TOP) {
		// do nothing on left_top_x
	}

	if (align == LTBA_RIGHT_BOTTOM
		|| align == LTBA_RIGHT_CENTER
		|| align == LTBA_RIGHT_TOP) {
		x += r->w;
	}

	// Vertical axis

	if (align == LTBA_CENTER_BOTTOM
		|| align == LTBA_LEFT_BOTTOM
		|| align == LTBA_RIGHT_BOTTOM) {
		y -= r->h;
	}

	if (align == LTBA_CENTER_CENTER
		|| align == LTBA_LEFT_CENTER
		|| align == LTBA_RIGHT_CENTER) {
		y -= r->h / 2;
	}

	if (align == LTBA_CENTER_TOP
		|| align == LTBA_LEFT_TOP
		|| align == LTBA_RIGHT_TOP) {
		// do nothing on left_top_y
	}

	r->x = x;
	r->y = y;
	r->align = align;
}

int lwuidim_contains(const LWUIDIM* d, float x, float y) {
	LWUIDIM d_lt;
	lwuidim_convert_align(d, LTBA_LEFT_TOP, &d_lt);
	return d_lt.x <= x && x <= d_lt.x + d_lt.w && d_lt.y - d_lt.h <= y && y <= d_lt.y;
}
