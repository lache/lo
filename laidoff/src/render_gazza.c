#include <string.h>
#include "render_gazza.h"
#include "lwcontext.h"
#include "render_solid.h"
#include "laidoff.h"
#include "htmlui.h"
#include "linmath.h"
#include "lwmath.h"
#include <stdio.h>
#include "lwlog.h"
#include <assert.h>
#include "platform_detection.h"
#include <stdlib.h>
#include "lwfbo.h"
#if LW_PLATFORM_IOS
#include <alloca.h>
#endif
#include "logic.h"
#include "lwtimepoint.h"
#include <float.h>

static void render_htmlui_touch_rect(const LWCONTEXT* pLwc) {
    const int touch_rect_count = htmlui_get_touch_rect_count(pLwc->htmlui);
    const double now = lwtimepoint_now_seconds();
    for (int i = 0; i < touch_rect_count; i++) {
        double start;
        float x, y, z, width, height, extend_width, extend_height;
        mat4x4 view, proj;
        htmlui_get_touch_rect(pLwc->htmlui, i, &start, &x, &y, &z, &width, &height, &extend_width, &extend_height, view, proj);
        const double progress = now - start;
        if (progress < 0.2) {
            mat4x4 identity_view; mat4x4_identity(identity_view);
            const float sx1 = (float)(width + extend_width * 1.0f * progress / 0.2) / 2;
            const float sy1 = (float)(height + extend_height * 1.0f * progress / 0.2) / 2;
            const float sz1 = 1.0f;
            render_solid_vb_uv_shader_rot_view_proj(pLwc,
                                                    x,
                                                    y,
                                                    z,
                                                    sx1,
                                                    sy1,
                                                    sz1,
                                                    pLwc->tex_atlas[LAE_ZERO_FOR_BLACK],
                                                    LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                                    (float)(progress),
                                                    0.6f,
                                                    0.3f,
                                                    0.8f,
                                                    1.0f,
                                                    default_uv_offset,
                                                    default_flip_y_uv_scale,
                                                    LWST_DEFAULT,
                                                    0,
                                                    view,
                                                    proj);
            const float sx2 = (float)(width + extend_width * 1.6f * progress / 0.2) / 2;
            const float sy2 = (float)(height + extend_height * 1.6f * progress / 0.2) / 2;
            const float sz2 = 1.0f;
            render_solid_vb_uv_shader_rot_view_proj(pLwc,
                                                    x,
                                                    y,
                                                    z,
                                                    sx2,
                                                    sy2,
                                                    sz2,
                                                    pLwc->tex_atlas[LAE_ZERO_FOR_BLACK],
                                                    LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                                    (float)(progress),
                                                    0.0f,
                                                    0.0f,
                                                    0.5f,
                                                    1.0f,
                                                    default_uv_offset,
                                                    default_flip_y_uv_scale,
                                                    LWST_DEFAULT,
                                                    0,
                                                    view,
                                                    proj);
        }
    }
}

void lwc_render_gazza(const LWCONTEXT* pLwc) {
    LW_GL_VIEWPORT();
    lw_clear_color();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // revert to original viewport
    glViewport(0,
               0,
               pLwc->window_width,
               pLwc->window_height);
    lw_set_viewport_size((LWCONTEXT*)pLwc,
                         pLwc->window_width,
                         pLwc->window_height);
    // overwrite ui projection matrix
    logic_update_default_ui_proj_for_htmlui(pLwc->shared_fbo.width, pLwc->shared_fbo.height, ((LWCONTEXT*)pLwc)->proj);
    // render HTML UI queued at render command queue
    htmlui_render_render_commands(pLwc->htmlui);
    lw_set_viewport_size((LWCONTEXT*)pLwc,
                         pLwc->window_width,
                         pLwc->window_height);
    render_htmlui_touch_rect(pLwc);
    glEnable(GL_DEPTH_TEST);
}
