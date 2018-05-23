#include "lwbutton.h"
#include "lwmacro.h"
#include "lwlog.h"
#include <string.h>
#include "lwcontext.h"
#include "render_solid.h"

LWBUTTON* lwbutton_lae_append_atlas_additive(const LWCONTEXT* pLwc,
                                             LWBUTTONLIST* button_list,
                                             const char* id,
                                             float x,
                                             float y,
                                             float w,
                                             float h,
                                             LW_ATLAS_ENUM lae,
                                             LW_ATLAS_CONF lac,
                                             const char* atlas_sprite_name,
                                             float ui_alpha,
                                             float over_r,
                                             float over_g,
                                             float over_b) {
    LWBUTTON* b = lwbutton_append(pLwc, button_list, id, x, y, w, h);
    b->lae = lae;
    b->lae_alpha = 0; // not used in additive case
    b->ui_alpha = ui_alpha;
    b->over_r = over_r;
    b->over_g = over_g;
    b->over_b = over_b;
    b->enable_atlas = 1;
    b->enable_additive = 1;
    strcpy(b->atlas_sprite_name, atlas_sprite_name);
    b->lac = lac;
    return b;
}

LWBUTTON* lwbutton_lae_append(const LWCONTEXT* pLwc,
                              LWBUTTONLIST* button_list,
                              const char* id,
                              float x,
                              float y,
                              float w,
                              float h,
                              LW_ATLAS_ENUM lae,
                              LW_ATLAS_ENUM lae_alpha,
                              float ui_alpha,
                              float over_r,
                              float over_g,
                              float over_b) {
    LWBUTTON* b = lwbutton_append(pLwc, button_list, id, x, y, w, h);
    b->lae = lae;
    b->lae_alpha = lae_alpha;
    b->ui_alpha = ui_alpha;
    b->over_r = over_r;
    b->over_g = over_g;
    b->over_b = over_b;
    return b;
}

LWBUTTON* lwbutton_append(const LWCONTEXT* pLwc,
                          LWBUTTONLIST* button_list,
                          const char* id,
                          float x,
                          float y,
                          float w,
                          float h) {
    if (button_list->button_count >= ARRAY_SIZE(button_list->button)) {
        LOGEP("ARRAY_SIZE(button_list->button) exceeded");
        return 0;
    }
    LWBUTTON* b = &button_list->button[button_list->button_count];
    if (strlen(id) > ARRAY_SIZE(b->id) - 1) {
        LOGEP("ARRAY_SIZE(b->id) exceeded");
        return 0;
    }
    strcpy(b->id, id);
    b->x = x;
    b->y = y;
    b->w = w;
    b->h = h;
    b->over_r = 1.0f;
    b->over_g = 1.0f;
    b->over_b = 1.0f;
    b->viewport_x = pLwc->viewport_x;
    b->viewport_y = pLwc->viewport_y;
    button_list->button_count++;
    return b;
}

int lwbutton_press(const LWCONTEXT* pLwc,
                   const LWBUTTONLIST* button_list,
                   float x,
                   float y,
                   float* w_ratio,
                   float* h_ratio) {
    if (button_list->button_count >= ARRAY_SIZE(button_list->button)) {
        LOGEP("ARRAY_SIZE(button_list->button) exceeded");
        return -1;
    }
    for (int i = 0; i < button_list->button_count; i++) {
        const LWBUTTON* b = &button_list->button[i];
        // adjust x, y position according to viewport offset
        float x_v = x - (float)b->viewport_x / pLwc->width * 2 * pLwc->aspect_ratio;
        float y_v = y - (float)b->viewport_y / pLwc->height * 2;
        if (b->x <= x_v && x_v <= b->x + b->w && b->y - b->h <= y_v && y_v <= b->y) {
            *w_ratio = (x_v - b->x) / b->w;
            *h_ratio = 1.0f - (y_v - (b->y - b->h)) / b->h;
            return i;
        }
    }
    return -1;
}

const char* lwbutton_id(const LWBUTTONLIST* button_list, int idx) {
    if (idx >= ARRAY_SIZE(button_list->button)) {
        LOGEP("ARRAY_SIZE(button_list->button) exceeded");
        return 0;
    }
    return button_list->button[idx].id;
}

void render_lwbutton(const LWCONTEXT* pLwc, const LWBUTTONLIST* button_list) {
    for (int i = 0; i < button_list->button_count; i++) {
        LWBUTTON* b = (LWBUTTON*)&button_list->button[i];
        if (b->rendered) {
            continue;
        }
        b->rendered = 1;
        if (b->ui_alpha) {
            if (b->enable_additive) {
                lwc_enable_additive_blending();
            }
            if (b->enable_atlas) {
                // atlas sprite provided
                lw_load_tex(pLwc, b->lae);
                render_atlas_sprite(pLwc,
                                    b->lac,
                                    b->atlas_sprite_name,
                                    b->lae,
                                    LAE_DONTCARE,
                                    b->w,
                                    // b->h calculated automatically to retain aspect ratio of sprite
                                    b->x,
                                    b->y,
                                    b->ui_alpha,
                                    LVT_LEFT_TOP_ANCHORED_SQUARE);
            } else {
                // lae and lae_alpha provided
                lw_load_tex(pLwc, b->lae);
                lw_load_tex(pLwc, b->lae_alpha);
                render_solid_vb_ui_alpha(pLwc,
                                         b->x,
                                         b->y,
                                         b->w,
                                         b->h,
                                         pLwc->tex_atlas[b->lae],
                                         pLwc->tex_atlas[b->lae_alpha],
                                         LVT_LEFT_TOP_ANCHORED_SQUARE,
                                         b->ui_alpha,
                                         b->over_r,
                                         b->over_g,
                                         b->over_b,
                                         1.0f);
            }
            if (b->enable_additive) {
                lwc_disable_additive_blending();
            }
        }
    }
}
