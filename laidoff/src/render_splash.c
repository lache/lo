#include "render_splash.h"
#include "lwgl.h"
#include "lwcontext.h"
#include "laidoff.h"
#include "render_solid.h"
#include "render_text_block.h"
#include "lwmacro.h"

static void s_render_logo(const LWCONTEXT* pLwc) {
    lw_load_tex(pLwc, LAE_SPLASH512512);
	render_solid_vb_ui(pLwc,
                       0,
                       0,
                       2,
                       2,
                       pLwc->tex_atlas[LAE_SPLASH512512],
                       LVT_CENTER_CENTER_ANCHORED_SQUARE,
                       1,
                       0,
                       0,
                       0,
                       0);
}

void lwc_render_splash(const LWCONTEXT* pLwc) {
	LW_GL_VIEWPORT();
	glClearColor(0, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	s_render_logo(pLwc);
}
