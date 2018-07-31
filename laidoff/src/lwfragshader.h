#pragma once

#include "lwmacro.h"
#include "lwgl.h"

typedef struct _LWCONTEXT LWCONTEXT;

typedef enum _LW_FRAG_SHADER {
    LWFS_DEFAULT,
    LWFS_COLOR,
    LWFS_PANEL,
    LWFS_FONT,
    LWFS_PIXEL_FONT,
    LWFS_ETC1,
    LWFS_FAN,
    LWFS_EMITTER,
    LWFS_EMITTER2,
    LWFS_SPHERE_REFLECT,
    LWFS_SPHERE_REFLECT_FLOOR,
    LWFS_RINGGAUGE,
    LWFS_RADIALWAVE,
    LWFS_DEFAULT_NORMAL,
	LWFS_DEFAULT_NORMAL_COLOR,
    LWFS_LINE,
    LWFS_TILEMAP,

    LWFS_COUNT,
} LW_FRAG_SHADER;
static const char* frag_shader_filename[] = {
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "default-frag.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "color-frag.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "panel-frag.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "font-frag.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "pixel-font-frag.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "etc1-frag.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "fan-frag.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "emitter-frag.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "emitter2-frag.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "sphere-reflect-frag.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "sphere-reflect-floor-frag.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "ringgauge-frag.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "radialwave-frag.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "default-normal-frag.glsl",
	ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "default-normal-color-frag.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "line-frag.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "tilemap-frag.glsl",
};
LwStaticAssert(ARRAY_SIZE(frag_shader_filename) == LWFS_COUNT, "LWFS_COUNT error");

GLuint lw_create_frag_shader(LW_FRAG_SHADER lwfs, const char* path);
void lw_create_all_frag_shader(LWCONTEXT* pLwc);
void lw_delete_all_frag_shader(LWCONTEXT* pLwc);
