#include "lwcontext.h"
#include "render_mocap.h"
#include "htmlui.h"
#include "laidoff.h"
#include "logic.h"
#include "render_sg.h"

void lwc_render_mocap_fbo_body(const LWCONTEXT* pLwc, const char* html_body) {
    if (lwfbo_prerender(pLwc, &pLwc->shared_fbo) == 0) {
        htmlui_load_render_draw_body(pLwc->htmlui, html_body);
        lwfbo_postrender(pLwc, &pLwc->shared_fbo);
    }
}

void lwc_render_mocap(const LWCONTEXT* pLwc) {
    LW_GL_VIEWPORT();
    lw_clear_color();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // overwrite ui projection matrix
    logic_update_default_ui_proj_for_htmlui(pLwc->shared_fbo.width, pLwc->shared_fbo.height, ((LWCONTEXT*)pLwc)->proj);
    // render HTML UI queued at render command queue
    htmlui_render_render_commands(pLwc->htmlui);
    // revert viewport size
    lw_set_viewport_size((LWCONTEXT*)pLwc,
                         pLwc->window_width,
                         pLwc->window_height);

    lwc_render_sg(pLwc, pLwc->sg);
}
