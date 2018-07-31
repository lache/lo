#pragma once

#include "lwgl.h"
#include "lwvertexshader.h"
#include "lwfragshader.h"

typedef struct _LWCONTEXT LWCONTEXT;

typedef enum _LW_SHADER_TYPE {
	LWST_DEFAULT,
	LWST_FONT,
    LWST_PIXEL_FONT,
	LWST_ETC1,
	LWST_SKIN,
	LWST_FAN,
	LWST_EMITTER,
	LWST_EMITTER2,
	LWST_COLOR,
	LWST_PANEL,
	LWST_SPHERE_REFLECT,
	LWST_SPHERE_REFLECT_FLOOR,
	LWST_RINGGAUGE,
	LWST_RADIALWAVE,
	LWST_DEFAULT_NORMAL,
	LWST_DEFAULT_NORMAL_COLOR,
    LWST_LINE,
    LWST_MORPH,
    LWST_TILEMAP,

	LWST_COUNT,
} LW_SHADER_TYPE;
#define MAX_SHADER (LWST_COUNT)
typedef struct _LWSHADERFILENAME {
	LW_SHADER_TYPE shader_type;
	const char *debug_shader_name;
	LW_VERTEX_SHADER lwvs;
	LW_FRAG_SHADER lwfs;
} LWSHADERFILENAME;
static const LWSHADERFILENAME shader_filename[] = {
	{ LWST_DEFAULT,                 "Default Shader",               LWVS_DEFAULT,				LWFS_DEFAULT,},
	{ LWST_FONT,                    "Outline Font Shader",          LWVS_DEFAULT,				LWFS_FONT,},
    { LWST_PIXEL_FONT,              "Pixel Font Shader",            LWVS_DEFAULT,               LWFS_PIXEL_FONT,},
	{ LWST_ETC1,                    "ETC1 with Alpha Shader",       LWVS_DEFAULT,				LWFS_ETC1,},
	{ LWST_SKIN,                    "Skin Shader",                  LWVS_SKIN,					LWFS_DEFAULT,},
	{ LWST_FAN,                     "Fan Shader",                   LWVS_FAN,					LWFS_FAN,},
	{ LWST_EMITTER,                 "Emitter Shader",               LWVS_EMITTER,				LWFS_EMITTER,},
	{ LWST_EMITTER2,                "Emitter2 Shader",              LWVS_EMITTER2,				LWFS_EMITTER2,},
	{ LWST_COLOR,                   "Color Shader",                 LWVS_DEFAULT,				LWFS_COLOR,},
	{ LWST_PANEL,                   "Panel Shader",                 LWVS_DEFAULT,				LWFS_PANEL,},
	{ LWST_SPHERE_REFLECT,          "Sphere Reflect Shader",        LWVS_SPHERE_REFLECT,		LWFS_SPHERE_REFLECT,},
	{ LWST_SPHERE_REFLECT_FLOOR,    "Sphere Reflect Floor Shader",  LWVS_SPHERE_REFLECT,		LWFS_SPHERE_REFLECT_FLOOR,},
	{ LWST_RINGGAUGE,               "Ringgauge Shader",             LWVS_DEFAULT,				LWFS_RINGGAUGE,},
	{ LWST_RADIALWAVE,              "Radial wave Shader",           LWVS_DEFAULT,				LWFS_RADIALWAVE,},
	{ LWST_DEFAULT_NORMAL,          "Default Normal Shader",        LWVS_DEFAULT_NORMAL,		LWFS_DEFAULT_NORMAL,},
	{ LWST_DEFAULT_NORMAL_COLOR,    "Default Normal Color Shader",  LWVS_DEFAULT_NORMAL_COLOR,	LWFS_DEFAULT_NORMAL_COLOR, },
    { LWST_LINE,                    "Line Shader",                  LWVS_LINE,                  LWFS_LINE, },
    { LWST_MORPH,                   "Morph Shader",                 LWVS_MORPH,                 LWFS_DEFAULT, },
    { LWST_TILEMAP,                 "Tilemap Shader",               LWVS_TILEMAP,               LWFS_TILEMAP, },
};
LwStaticAssert(ARRAY_SIZE(shader_filename) == LWST_COUNT, "LWST_COUNT error");

typedef struct _LWSHADER {
	int valid;

	GLuint vertex_shader;
	GLuint fragment_shader;
	GLuint program;

	// Vertex attributes
	GLint vpos_location;
	GLint vnor_location;
	GLint vcol_location;
	GLint vuv_location;
	GLint vs9_location;
	GLint vbweight_location;
	GLint vbmat_location;
	GLint theta_location;
	GLint shade_location;
	GLint a_pID;
	GLint a_pRadiusOffset;
	GLint a_pVelocityOffset;
	GLint a_pDecayOffset;
	GLint a_pSizeOffset;
	GLint a_pColorOffset;
    GLint vpos2_location; // morph vertex

	// Uniforms
	GLint mvp_location;
	GLint vuvoffset_location;
	GLint vuvscale_location;
	GLint vs9offset_location;
	GLint m_location;
	GLint alpha_multiplier_location;
	GLint overlay_color_location;
	GLint overlay_color_ratio_location;
	GLint multiply_color_location;
	GLint diffuse_location;
	GLint diffuse_arrow_location;
	GLint alpha_only_location;
	GLint glyph_color_location;
	GLint outline_color_location;
	GLint bone_location;
	GLint rscale_location;
	GLint thetascale_location;
	GLint projection_matrix_location;
	GLint k_location;
	GLint color_location;
	GLint time_location;
	GLint texture_location;
	GLint u_ProjectionViewMatrix;
	GLint u_ModelMatrix;
	GLint u_Gravity;
	GLint u_Time;
	GLint u_eRadius;
	GLint u_eVelocity;
	GLint u_eDecay;
	GLint u_eSizeStart;
	GLint u_eSizeEnd;
	GLint u_eScreenWidth;
	GLint u_eColorStart;
	GLint u_eColorEnd;
	GLint u_Texture;
	GLint u_TextureAlpha;
	GLint time;
	GLint resolution;
	GLint sphere_pos;
	GLint sphere_col;
	GLint sphere_col_ratio;
	GLint sphere_speed;
	GLint sphere_move_rad;
	GLint reflect_size;
	GLint arrow_center;
	GLint arrow_angle;
	GLint arrow_scale;
	GLint arrowRotMat2;
	GLint gauge_ratio;
	GLint full_color;
	GLint empty_color;
	GLint wrap_offset;
    GLint morph_weight;
    GLint vertex_color;
    GLint vertex_color_ratio;
} LWSHADER;

int lw_create_shader_program(const char *shader_name, LWSHADER *pShader, GLuint vs, GLuint fs);

void lw_create_all_shader_program(LWCONTEXT *pLwc);

void lw_delete_all_shader_program(LWCONTEXT *pLwc);

void lw_create_lazy_shader_program(const LWCONTEXT *_pLwc, LW_SHADER_TYPE shader_index);
