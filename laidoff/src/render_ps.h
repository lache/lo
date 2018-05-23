#pragma once

#include <linmath.h>

typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWEMITTER2OBJECT LWEMITTER2OBJECT;
typedef struct _LWPSCONTEXT LWPSCONTEXT;

void lwc_render_ps(const LWCONTEXT* pLwc, const LWPSCONTEXT* c);
void ps_render_explosion(const LWCONTEXT* pLwc, const LWPSCONTEXT* c, const LWEMITTER2OBJECT* emit_object, const mat4x4 proj_view, const mat4x4 model);
