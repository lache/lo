#pragma once

#include "lwmacro.h"
#include "lwgl.h"

typedef struct _LWCONTEXT LWCONTEXT;

typedef enum _LW_VERTEX_SHADER {
    LWVS_DEFAULT,
    LWVS_SKIN,
    LWVS_FAN,
    LWVS_EMITTER,
    LWVS_EMITTER2,
    LWVS_SPHERE_REFLECT,
    LWVS_DEFAULT_NORMAL,
	LWVS_DEFAULT_NORMAL_COLOR,
    LWVS_LINE,
    LWVS_MORPH,

    LWVS_COUNT,
} LW_VERTEX_SHADER;
static const char* vertex_shader_filename[] = {
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "default-vert.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "skin-vert.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "fan-vert.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "emitter-vert.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "emitter2-vert.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "sphere-reflect-vert.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "default-normal-vert.glsl",
	ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "default-normal-color-vert.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "line-vert.glsl",
    ASSETS_BASE_PATH GLSL_DIR_NAME PATH_SEPARATOR "morph-vert.glsl",
};
LwStaticAssert(ARRAY_SIZE(vertex_shader_filename) == LWVS_COUNT, "LWVS_COUNT error");

GLuint lw_create_vertex_shader(LW_VERTEX_SHADER lwvs, const char* path);
void lw_create_all_vertex_shader(LWCONTEXT* pLwc);
void lw_delete_all_vertex_shader(LWCONTEXT* pLwc);
