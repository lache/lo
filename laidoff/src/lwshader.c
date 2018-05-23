#include "lwshader.h"
#include "lwlog.h"
#include "lwcontext.h"
#include <stdlib.h>

int lw_create_shader_program(const char* shader_name, LWSHADER* pShader, GLuint vs, GLuint fs) {
    pShader->valid = 0;
    pShader->vertex_shader = vs;
    pShader->fragment_shader = fs;
    pShader->program = glCreateProgram();
    glAttachShader(pShader->program, pShader->vertex_shader);
    glAttachShader(pShader->program, pShader->fragment_shader);
    glLinkProgram(pShader->program);
    //Note the different functions here: glGetProgram* instead of glGetShader*.
    GLint isLinked = 0;
    glGetProgramiv(pShader->program, GL_LINK_STATUS, (int *)&isLinked);
    if (isLinked == GL_FALSE) {
        GLint maxLength = 0;
        glGetProgramiv(pShader->program, GL_INFO_LOG_LENGTH, &maxLength);
        //The maxLength includes the NULL character
        GLchar *errorLog = (GLchar *)calloc(maxLength, sizeof(GLchar));
        glGetProgramInfoLog(pShader->program, maxLength, &maxLength, errorLog);
        LOGE("'%s' Program Link Error: %s", shader_name, errorLog);
        free(errorLog);
        //We don't need the program anymore.
        glDeleteProgram(pShader->program);
        //Use the infoLog as you see fit.
        //In this simple program, we'll just leave
        return -1;
    }

    glUseProgram(pShader->program);
    // Uniforms
    pShader->mvp_location = glGetUniformLocation(pShader->program, "MVP");
    pShader->vuvoffset_location = glGetUniformLocation(pShader->program, "vUvOffset");
    pShader->vuvscale_location = glGetUniformLocation(pShader->program, "vUvScale");
    pShader->vs9offset_location = glGetUniformLocation(pShader->program, "vS9Offset");
    pShader->m_location = glGetUniformLocation(pShader->program, "M");
    pShader->alpha_multiplier_location = glGetUniformLocation(pShader->program, "alpha_multiplier");
    pShader->overlay_color_location = glGetUniformLocation(pShader->program, "overlay_color");
    pShader->overlay_color_ratio_location = glGetUniformLocation(pShader->program, "overlay_color_ratio");
    pShader->multiply_color_location = glGetUniformLocation(pShader->program, "multiply_color");
    pShader->diffuse_location = glGetUniformLocation(pShader->program, "diffuse");
    pShader->diffuse_arrow_location = glGetUniformLocation(pShader->program, "diffuse_arrow");
    pShader->alpha_only_location = glGetUniformLocation(pShader->program, "alpha_only");
    pShader->glyph_color_location = glGetUniformLocation(pShader->program, "glyph_color");
    pShader->outline_color_location = glGetUniformLocation(pShader->program, "outline_color");
    pShader->bone_location = glGetUniformLocation(pShader->program, "bone");
    pShader->rscale_location = glGetUniformLocation(pShader->program, "rscale");
    pShader->thetascale_location = glGetUniformLocation(pShader->program, "thetascale");
    pShader->projection_matrix_location = glGetUniformLocation(pShader->program, "uProjectionMatrix");
    pShader->k_location = glGetUniformLocation(pShader->program, "uK");
    pShader->color_location = glGetUniformLocation(pShader->program, "uColor");
    pShader->time_location = glGetUniformLocation(pShader->program, "uTime");
    pShader->texture_location = glGetUniformLocation(pShader->program, "uTexture");
    pShader->u_ProjectionViewMatrix = glGetUniformLocation(pShader->program, "u_ProjectionViewMatrix");
    pShader->u_ModelMatrix = glGetUniformLocation(pShader->program, "u_ModelMatrix");
    pShader->u_Gravity = glGetUniformLocation(pShader->program, "u_Gravity");
    pShader->u_Time = glGetUniformLocation(pShader->program, "u_Time");
    pShader->u_eRadius = glGetUniformLocation(pShader->program, "u_eRadius");
    pShader->u_eVelocity = glGetUniformLocation(pShader->program, "u_eVelocity");
    pShader->u_eDecay = glGetUniformLocation(pShader->program, "u_eDecay");
    pShader->u_eSizeStart = glGetUniformLocation(pShader->program, "u_eSizeStart");
    pShader->u_eSizeEnd = glGetUniformLocation(pShader->program, "u_eSizeEnd");
    pShader->u_eScreenWidth = glGetUniformLocation(pShader->program, "u_eScreenWidth");
    pShader->u_eColorStart = glGetUniformLocation(pShader->program, "u_eColorStart");
    pShader->u_eColorEnd = glGetUniformLocation(pShader->program, "u_eColorEnd");
    pShader->u_Texture = glGetUniformLocation(pShader->program, "u_Texture");
    pShader->u_TextureAlpha = glGetUniformLocation(pShader->program, "u_TextureAlpha");
    pShader->time = glGetUniformLocation(pShader->program, "time");
    pShader->resolution = glGetUniformLocation(pShader->program, "resolution");
    pShader->sphere_pos = glGetUniformLocation(pShader->program, "sphere_pos");
    pShader->sphere_col = glGetUniformLocation(pShader->program, "sphere_col");
    pShader->sphere_col_ratio = glGetUniformLocation(pShader->program, "sphere_col_ratio");
    pShader->sphere_speed = glGetUniformLocation(pShader->program, "sphere_speed");
    pShader->sphere_move_rad = glGetUniformLocation(pShader->program, "sphere_move_rad");
    pShader->reflect_size = glGetUniformLocation(pShader->program, "reflect_size");
    pShader->arrow_center = glGetUniformLocation(pShader->program, "arrow_center");
    pShader->arrow_angle = glGetUniformLocation(pShader->program, "arrow_angle");
    pShader->arrow_scale = glGetUniformLocation(pShader->program, "arrow_scale");
    pShader->arrowRotMat2 = glGetUniformLocation(pShader->program, "arrowRotMat2");
    pShader->gauge_ratio = glGetUniformLocation(pShader->program, "gauge_ratio");
    pShader->full_color = glGetUniformLocation(pShader->program, "full_color");
    pShader->empty_color = glGetUniformLocation(pShader->program, "empty_color");
    pShader->wrap_offset = glGetUniformLocation(pShader->program, "wrap_offset");
    pShader->morph_weight = glGetUniformLocation(pShader->program, "morph_weight");
    pShader->vertex_color = glGetUniformLocation(pShader->program, "vertex_color");
    pShader->vertex_color_ratio = glGetUniformLocation(pShader->program, "vertex_color_ratio");

    // Set initial value...
    glUseProgram(pShader->program);
    if (pShader->multiply_color_location >= 0) {
        glUniform3f(pShader->multiply_color_location, 1.0f, 1.0f, 1.0f);
    }

    // Attribs
    pShader->vpos_location = glGetAttribLocation(pShader->program, "vPos");
	pShader->vnor_location = glGetAttribLocation(pShader->program, "vNor");
    pShader->vcol_location = glGetAttribLocation(pShader->program, "vCol");
    pShader->vuv_location = glGetAttribLocation(pShader->program, "vUv");
    pShader->vs9_location = glGetAttribLocation(pShader->program, "vS9");
    pShader->vbweight_location = glGetAttribLocation(pShader->program, "vBw");
    pShader->vbmat_location = glGetAttribLocation(pShader->program, "vBm");
    pShader->theta_location = glGetAttribLocation(pShader->program, "aTheta");
    pShader->shade_location = glGetAttribLocation(pShader->program, "aShade");
    pShader->a_pID = glGetAttribLocation(pShader->program, "a_pID");
    pShader->a_pRadiusOffset = glGetAttribLocation(pShader->program, "a_pRadiusOffset");
    pShader->a_pVelocityOffset = glGetAttribLocation(pShader->program, "a_pVelocityOffset");
    pShader->a_pDecayOffset = glGetAttribLocation(pShader->program, "a_pDecayOffset");
    pShader->a_pSizeOffset = glGetAttribLocation(pShader->program, "a_pSizeOffset");
    pShader->a_pColorOffset = glGetAttribLocation(pShader->program, "a_pColorOffset");
    pShader->vpos2_location = glGetAttribLocation(pShader->program, "vPos2");

    pShader->valid = 1;
    return 0;
}

void lw_create_all_shader_program(LWCONTEXT* pLwc) {
    for (int i = 0; i < LWST_COUNT; i++) {
        lw_create_shader_program(shader_filename[i].debug_shader_name,
                         &pLwc->shader[i],
                         pLwc->vertex_shader[shader_filename[i].lwvs],
                         pLwc->frag_shader[shader_filename[i].lwfs]);
    }
}

void lw_delete_all_shader_program(LWCONTEXT* pLwc) {
    for (int i = 0; i < LWST_COUNT; i++) {
        glDeleteProgram(pLwc->shader[i].program);
    }
}

void lw_create_lazy_shader_program(const LWCONTEXT* _pLwc, LW_SHADER_TYPE shader_index) {
    LWCONTEXT* pLwc = (LWCONTEXT*)_pLwc;
    // early-pruning
    if (pLwc->shader[shader_index].program) {
        return;
    }
    // create vertex shader if not created
    LW_VERTEX_SHADER lwvs = shader_filename[shader_index].lwvs;
    if (pLwc->vertex_shader[lwvs] == 0)         {
        pLwc->vertex_shader[lwvs] = lw_create_vertex_shader(lwvs, vertex_shader_filename[lwvs]);
    }
    // create frag shader if not created
    LW_FRAG_SHADER lwfs = shader_filename[shader_index].lwfs;
    if (pLwc->frag_shader[lwfs] == 0) {
        pLwc->frag_shader[lwfs] = lw_create_frag_shader(lwfs, frag_shader_filename[lwfs]);
    }
    // create shader program if not created
    if (pLwc->shader[shader_index].program == 0) {
        const char* debug_shader_name = shader_filename[shader_index].debug_shader_name;
        lw_create_shader_program(debug_shader_name,
                                 &pLwc->shader[shader_index],
                                 pLwc->vertex_shader[lwvs],
                                 pLwc->frag_shader[lwfs]);
    }
}
