#include <string.h>

#include "lwcontext.h"
#include "render_dialog.h"
#include "lwlog.h"
#include "font.h"
#include "render_text_block.h"
#include "laidoff.h"

static void render_portrait(const LWCONTEXT* pLwc) {
    int shader_index = LWST_ETC1;

    lazy_glUseProgram(pLwc, shader_index);
    glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, default_uv_offset);
    glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, default_uv_scale);
    glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, 1);
    glUniform1i(pLwc->shader[shader_index].diffuse_location, 0); // 0 means GL_TEXTURE0
    glUniform1i(pLwc->shader[shader_index].alpha_only_location, 1); // 1 means GL_TEXTURE1
    glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)pLwc->proj);

    lazy_glBindBuffer(pLwc, LVT_CENTER_CENTER_ANCHORED_SQUARE);
    bind_all_vertex_attrib_etc1_with_alpha(pLwc, LVT_CENTER_CENTER_ANCHORED_SQUARE);
    glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0);
    glActiveTexture(GL_TEXTURE0);
    lazy_tex_atlas_glBindTexture(pLwc, LAE_P_DOHEE);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
    glActiveTexture(GL_TEXTURE1);
    lazy_tex_atlas_glBindTexture(pLwc, LAE_P_DOHEE_ALPHA);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[LVT_CENTER_CENTER_ANCHORED_SQUARE].vertex_count);
}

static void render_dialog_balloon(const LWCONTEXT* pLwc) {
    int shader_index = LWST_DEFAULT;

    lazy_glUseProgram(pLwc, shader_index);
    glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, default_uv_offset);
    glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, default_uv_scale);
    glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, 0.75f);
    glUniform1i(pLwc->shader[shader_index].diffuse_location, 0); // 0 means GL_TEXTURE0
    glUniform1i(pLwc->shader[shader_index].alpha_only_location, 1); // 1 means GL_TEXTURE1
    glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)pLwc->proj);



    float ui_scale_x = pLwc->aspect_ratio;
    float ui_scale_y = 0.275f;

    mat4x4 model_translate;
    mat4x4 model;
    mat4x4 identity_view; mat4x4_identity(identity_view);
    mat4x4 view_model;
    mat4x4 proj_view_model;
    mat4x4 model_scale;

    const float scale = 1.0f;

    mat4x4_identity(model_scale);
    mat4x4_scale_aniso(model_scale, model_scale, ui_scale_x, ui_scale_y, scale);

    mat4x4_translate(model_translate,
                     0,
                     -1.0f,
                     0);

    mat4x4_identity(model);
    mat4x4_mul(model, model_translate, model_scale);

    mat4x4_mul(view_model, identity_view, model);

    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, pLwc->proj, view_model);

    lazy_glBindBuffer(pLwc, LVT_CENTER_BOTTOM_ANCHORED_SQUARE);
    bind_all_vertex_attrib(pLwc, LVT_CENTER_BOTTOM_ANCHORED_SQUARE);
    glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0);
    glActiveTexture(GL_TEXTURE0);
    lazy_tex_atlas_glBindTexture(pLwc, LAE_U_DIALOG_BALLOON);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
    glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[LVT_CENTER_BOTTOM_ANCHORED_SQUARE].vertex_count);
}

static void render_bg(const LWCONTEXT* pLwc) {
    int shader_index = LWST_DEFAULT;



    float ui_scale_x = pLwc->aspect_ratio;
    float ui_scale_y = 1;

    mat4x4 model_translate;
    mat4x4 model;
    mat4x4 identity_view; mat4x4_identity(identity_view);
    mat4x4 view_model;
    mat4x4 proj_view_model;
    mat4x4 model_scale;

    const float scale = 1.0f;

    mat4x4_identity(model_scale);
    mat4x4_scale_aniso(model_scale, model_scale, ui_scale_x, ui_scale_y, scale);

    mat4x4_translate(model_translate,
                     0,
                     0,
                     0);

    mat4x4_identity(model);
    mat4x4_mul(model, model_translate, model_scale);

    mat4x4_mul(view_model, identity_view, model);

    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, pLwc->proj, view_model);

    GLenum error_enum;
    {
        lazy_glUseProgram(pLwc, shader_index);
        error_enum = glGetError();
        lazy_glBindBuffer(pLwc, LVT_CENTER_CENTER_ANCHORED_SQUARE);
        error_enum = glGetError();
        bind_all_vertex_attrib(pLwc, LVT_CENTER_CENTER_ANCHORED_SQUARE);
        glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat *)proj_view_model);
        error_enum = glGetError();
        glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0);
        error_enum = glGetError();
        lazy_tex_atlas_glBindTexture(pLwc, pLwc->dialog_bg_tex_index);
        error_enum = glGetError();
        set_tex_filter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR); // when mip-map enabled
        error_enum = glGetError();
        //glTexParameterf(GL_TEXTURE_2D, 0x84FE, 0);
        //set_tex_filter(GL_LINEAR, GL_LINEAR);
        glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[LVT_CENTER_CENTER_ANCHORED_SQUARE].vertex_count);
        error_enum = glGetError();
    }
}

void lwc_render_dialog(const LWCONTEXT* pLwc) {
    // ****

    LW_GL_VIEWPORT();
    lw_clear_color();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //LOGV("Viewport size: %d x %d", pLwc->width, pLwc->height);
    // Result curtain
    //mat4x4 identity;
    //mat4x4_identity(identity);

    int shader_index = LWST_DEFAULT;

    lazy_glUseProgram(pLwc, shader_index);
    glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, default_uv_offset);
    glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, default_uv_scale);
    glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, 1);
    glUniform1i(pLwc->shader[shader_index].diffuse_location, 0); // 0 means GL_TEXTURE0
    glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)pLwc->proj);

    glActiveTexture(GL_TEXTURE0);
    lazy_tex_atlas_glBindTexture(pLwc, pLwc->tex_atlas_index);

    if (pLwc->dialog_bg_tex_index) {
        render_bg(pLwc);
    }

    if (pLwc->dialog_portrait_tex_index) {
        render_portrait(pLwc);
    }

    glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0);

    render_dialog_balloon(pLwc);

    LWTEXTBLOCK text_block;
    text_block.align = LTBA_LEFT_TOP;
    text_block.text = pLwc->dialog;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = pLwc->dialog_start_index;
    text_block.end_index = text_block.begin_index + pLwc->render_char < text_block.text_bytelen ? text_block.begin_index +
        pLwc->render_char : text_block.text_bytelen;
    text_block.text_block_x = -0.75f * pLwc->aspect_ratio;
    text_block.text_block_y = -0.55f;
    text_block.text_block_width = 0.70f * 2 * pLwc->aspect_ratio;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_A;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_A;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);

    render_text_block(pLwc, &text_block);
}
