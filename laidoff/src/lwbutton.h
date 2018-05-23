#pragma once

#include "lwatlasenum.h"
#include "lwatlassprite.h"

#define LW_UI_IDENTIFIER_LENGTH (32)
#define LW_UI_BUTTON_LIST_SIZE (64)

typedef struct _LWCONTEXT LWCONTEXT;

typedef struct _LWBUTTON {
    char id[LW_UI_IDENTIFIER_LENGTH];
    float x, y, w, h;
    int enable_lae;
    LW_ATLAS_ENUM lae;
    LW_ATLAS_ENUM lae_alpha;
    float ui_alpha;
    float over_r;
    float over_g;
    float over_b;
    int enable_atlas;
    int enable_additive;
    LW_ATLAS_CONF lac;
    char atlas_sprite_name[64];
    int rendered;
    int viewport_x;
    int viewport_y;
} LWBUTTON;

typedef struct _LWBUTTONLIST {
    LWBUTTON button[LW_UI_BUTTON_LIST_SIZE];
    int button_count;
} LWBUTTONLIST;

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
                                             float over_b);
LWBUTTON* lwbutton_append(const LWCONTEXT* pLwc,
                          LWBUTTONLIST* button_list,
                          const char* id,
                          float x,
                          float y,
                          float w,
                          float h);
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
                              float over_b);
int lwbutton_press(const LWCONTEXT* pLwc,
                   const LWBUTTONLIST* button_list,
                   float x,
                   float y,
                   float* w_ratio,
                   float* h_ratio);
const char* lwbutton_id(const LWBUTTONLIST* button_list, int idx);
void render_lwbutton(const LWCONTEXT* pLwc, const LWBUTTONLIST* button_list);
