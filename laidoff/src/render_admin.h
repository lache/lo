#pragma once

typedef struct _LWCONTEXT LWCONTEXT;

void lwc_render_admin(const LWCONTEXT* pLwc);
void touch_admin(LWCONTEXT* pLwc, float x0, float y0, float x1, float y1);
