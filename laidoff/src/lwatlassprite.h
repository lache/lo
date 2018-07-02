#pragma once

#include "sprite_data.h"
#include "lwmacro.h"
#include "lwatlasenum.h"

#ifdef __cplusplus
extern "C" {;
#endif

typedef struct _LWCONTEXT LWCONTEXT;

typedef enum _LW_ATLAS_SPRITE {
    LAS_COMMAND_SELECTED_BG,
    LAS_HANNIBAL_FAT,
    LAS_HANNIBAL,
    LAS_ICECREAM_FAT,
    LAS_ICECREAM,
    LAS_PSLOT1,
    LAS_SHADOW,
    LAS_UI_ALL,

    LAS_COUNT,

    LAS_DONTCARE,
} LW_ATLAS_SPRITE;

typedef enum _LW_ATLAS_CONF {
    LAC_RESULT_TITLE,
    LAC_PREPARE_TITLE,
    LAC_UI_BUTTON,
    LAC_FLAGS_MINI,
    LAC_CAPTAIN,
    LAC_SHIP,

    LAC_COUNT,
} LW_ATLAS_CONF;

static const char *atlas_conf_filename[] = {
    ASSETS_BASE_PATH "atlas" PATH_SEPARATOR "result-title-atlas-a.json",
    ASSETS_BASE_PATH "atlas" PATH_SEPARATOR "prepare-title-atlas-a.json",
    ASSETS_BASE_PATH "atlas" PATH_SEPARATOR "ui-button-atlas.json",
    ASSETS_BASE_PATH "atlas" PATH_SEPARATOR "flags-mini.json",
    ASSETS_BASE_PATH "atlas" PATH_SEPARATOR "captain.json",
    ASSETS_BASE_PATH "atlas" PATH_SEPARATOR "ship.json",
};

static LW_ATLAS_ENUM atlas_first_lae[] = {
    LAE_RESULT_TITLE_ATLAS,
    LAE_PREPARE_TITLE_ATLAS,
    LAE_UI_BUTTON_ATLAS,
    LAE_FLAGS_MINI0,
    LAE_CAPTAIN0,
    LAE_SHIP0,
};

static LW_ATLAS_ENUM atlas_first_alpha_lae[] = {
    LAE_RESULT_TITLE_ATLAS_ALPHA,
    LAE_PREPARE_TITLE_ATLAS_ALPHA,
    LAE_DONTCARE,
    LAE_FLAGS_MINI0_ALPHA,
    LAE_DONTCARE,
    LAE_DONTCARE,
};

typedef struct _LWATLASSPRITE {
    char name[64];
    int x;
    int y;
    int width;
    int height;
    int atlas_index;
} LWATLASSPRITE;

typedef struct _LWATLASSPRITEARRAY {
    char atlas_name[64];
    int count;
    LWATLASSPRITE* first;
    LW_ATLAS_ENUM first_lae;
    LW_ATLAS_ENUM first_alpha_lae;
} LWATLASSPRITEARRAY;

typedef struct _LWATLASSPRITEPTR {
    const LWATLASSPRITEARRAY* atlas;
    const LWATLASSPRITE* sprite;
} LWATLASSPRITEPTR;

LwStaticAssert(ARRAY_SIZE(SPRITE_DATA[0]) == LAS_COUNT, "LAS_COUNT error");
LwStaticAssert(ARRAY_SIZE(atlas_conf_filename) == LAC_COUNT, "LAC_COUNT error");
LwStaticAssert(ARRAY_SIZE(atlas_first_lae) == LAC_COUNT, "LAC_COUNT error");
LwStaticAssert(ARRAY_SIZE(atlas_first_alpha_lae) == LAC_COUNT, "LAC_COUNT error");

const LWATLASSPRITE* atlas_sprite_name(const LWCONTEXT* pLwc, LW_ATLAS_CONF lac, const char* name);
LWATLASSPRITEPTR atlas_name_sprite_name(const LWCONTEXT* pLwc, const char* atlas_name, const char* sprite_name);
LW_ATLAS_ENUM atlas_sprite_lae(const LWATLASSPRITEPTR* ptr);
LW_ATLAS_ENUM atlas_sprite_alpha_lae(const LWATLASSPRITEPTR* ptr);
void atlas_sprite_uv(const LWATLASSPRITE* sprite, int width, int height, float uv_offset[2], float uv_scale[2]);
void render_atlas_sprite_ptr(const LWCONTEXT* pLwc,
                             const LWATLASSPRITE* sprite,
                             LW_ATLAS_ENUM lae,
                             LW_ATLAS_ENUM lae_alpha,
                             float sprite_width,
                             float x,
                             float y,
                             float ui_alpha,
                             int lvt);
void render_atlas_sprite(const LWCONTEXT* pLwc,
                         LW_ATLAS_CONF atlas_conf,
                         const char* sprite_name,
                         LW_ATLAS_ENUM lae,
                         LW_ATLAS_ENUM lae_alpha,
                         float sprite_width,
                         float x,
                         float y,
                         float ui_alpha,
                         int lvt);
#ifdef __cplusplus
}
#endif
