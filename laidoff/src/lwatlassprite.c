#include "lwatlassprite.h"
#include "lwcontext.h"
#include "render_solid.h"
#include <string.h>
#include <assert.h>

const LWATLASSPRITE *atlas_sprite_name(const LWCONTEXT *pLwc, LW_ATLAS_CONF lac, const char *name) {
    const LWATLASSPRITEARRAY *a = &pLwc->atlas_conf[lac];
    for (int i = 0; i < a->count; i++) {
        if (strcmp(a->first[i].name, name) == 0) {
            return &a->first[i];
        }
    }
    return NULL;
}

LWATLASSPRITEPTR
atlas_name_sprite_name(const LWCONTEXT *pLwc, const char *atlas_name, const char *sprite_name) {
    LWATLASSPRITEPTR ptr = {0, 0};
    for (int i = 0; i < LAC_COUNT; i++) {
        if (strcmp(pLwc->atlas_conf[i].atlas_name, atlas_name) == 0) {
            for (int j = 0; j < pLwc->atlas_conf[i].count; j++) {
                if (strcmp(pLwc->atlas_conf[i].first[j].name, sprite_name) == 0) {
                    ptr.atlas = &pLwc->atlas_conf[i];
                    ptr.sprite = &pLwc->atlas_conf[i].first[j];
                }
            }
        }
    }
    return ptr;
}

LW_ATLAS_ENUM atlas_sprite_lae(const LWATLASSPRITEPTR *ptr) {
    int stride = ptr->atlas->first_alpha_lae != LAE_DONTCARE ? 2 : 1;
    return ptr->atlas->first_lae + stride * ptr->sprite->atlas_index;
}

LW_ATLAS_ENUM atlas_sprite_alpha_lae(const LWATLASSPRITEPTR *ptr) {
    int stride = ptr->atlas->first_alpha_lae != LAE_DONTCARE ? 2 : 1;
    return ptr->atlas->first_alpha_lae + stride * ptr->sprite->atlas_index;
}

void atlas_sprite_uv(const LWATLASSPRITE *sprite, int width, int height, float uv_offset[2],
                     float uv_scale[2]) {
    assert(width);
    assert(height);
    uv_scale[0] = (float) sprite->width / width;
    uv_scale[1] = (float) sprite->height / height;
    uv_offset[0] = (float) sprite->x / width;
    uv_offset[1] = (float) sprite->y / height;
}

void render_atlas_sprite_ptr(const LWCONTEXT *pLwc,
                             const LWATLASSPRITE *sprite,
                             LW_ATLAS_ENUM lae,
                             LW_ATLAS_ENUM lae_alpha,
                             float sprite_width,
                             float x,
                             float y,
                             float ui_alpha,
                             int lvt) {
    float uv_offset[2];
    float uv_scale[2];
    // pLwc->tex_atlas_width[lae], pLwc->tex_atlas_height[lae] should be filled
    // before calling atlas_sprite_uv()
    lw_load_tex(pLwc, lae);
    atlas_sprite_uv(sprite,
                    pLwc->tex_atlas_width[lae],
                    pLwc->tex_atlas_height[lae],
                    uv_offset,
                    uv_scale);
    const float sprite_aspect_ratio = uv_scale[0] / uv_scale[1];
    if (lae_alpha != LAE_DONTCARE) {
        lw_load_tex(pLwc, lae_alpha);
        render_solid_vb_ui_alpha_uv(pLwc,
                                    x,
                                    y,
                                    sprite_width,
                                    sprite_width / sprite_aspect_ratio,
                                    pLwc->tex_atlas[lae],
                                    pLwc->tex_atlas[lae_alpha],
                                    lvt,
                                    ui_alpha,
                                    0.0f,
                                    0.0f,
                                    0.0f,
                                    0.0f,
                                    uv_offset,
                                    uv_scale);
    } else {
        render_solid_vb_ui_uv_shader_rot(pLwc,
                                         x,
                                         y,
                                         sprite_width,
                                         sprite_width / sprite_aspect_ratio,
                                         pLwc->tex_atlas[lae],
                                         lvt,
                                         ui_alpha,
                                         0.0f,
                                         0.0f,
                                         0.0f,
                                         0.0f,
                                         uv_offset,
                                         uv_scale,
                                         LWST_DEFAULT,
                                         0);
    }
}

void render_atlas_sprite(const LWCONTEXT *pLwc,
                         LW_ATLAS_CONF atlas_conf,
                         const char *sprite_name,
                         LW_ATLAS_ENUM lae,
                         LW_ATLAS_ENUM lae_alpha,
                         float sprite_width,
                         float x,
                         float y,
                         float ui_alpha,
                         int lvt) {
    const LWATLASSPRITE *sprite = atlas_sprite_name(pLwc, atlas_conf, sprite_name);
    render_atlas_sprite_ptr(pLwc, sprite, lae, lae_alpha, sprite_width, x, y, ui_alpha, lvt);
}
