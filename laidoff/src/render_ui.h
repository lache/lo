#pragma once

typedef struct _LWCONTEXT LWCONTEXT;

void lwc_create_ui_vbo(LWCONTEXT* pLwc);
void lwc_render_ui(const LWCONTEXT* pLwc);

void render_basic_field_ui(const LWCONTEXT* pLwc);
