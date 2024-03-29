#include "render_text_block.h"
#include "lwcontext.h"
#include "font.h"
#include "unicode.h"
#include "render_solid.h"
#include "lwlog.h"
#include "laidoff.h"
#include <string.h>
#include "lwtextblock.h"

#define MAX_UNICODE_STRLEN (1024)
#define MAX_TEXT_BLOCK_LINE (64)

#define LW_COLOR_WHITE { 1, 1, 1, 1 }
#define LW_COLOR_BLACK { 0, 0, 0, 1 }
#define LW_COLOR_YELLOW { 1, 1, 0, 1 }

//const static float normal_outline_thickness = 3;
//const static float emp_outline_thickness = 3;

static float get_proportional_font_size(int width, int height, float font_size) {
    //return 1.0f;
    //return font_size;
    
    if (width > height) {
        return (float)height / (360.0f) * font_size;
    } else {
        return (float)width / (360.0f) * font_size;
    }
    
    //return font_size;
}

static void get_align_offset_and_mode(const LWCONTEXT* pLwc,
                                      const LWTEXTBLOCK* text_block,
                                      const int line,
                                      int width_sum[MAX_TEXT_BLOCK_LINE],
                                      int height_sum,
                                      int first_height,
                                      float prop_font_size,
                                      float* align_offset_x,
                                      float* align_offset_y,
                                      int* x_align_mode,
                                      int* y_align_mode) {
    // Horizontal
    if (text_block->align == LTBA_LEFT_TOP
        || text_block->align == LTBA_LEFT_CENTER
        || text_block->align == LTBA_LEFT_BOTTOM) {
        // LEFT
        *x_align_mode = -1;
    } else if (text_block->align == LTBA_CENTER_TOP
               || text_block->align == LTBA_CENTER_CENTER
               || text_block->align == LTBA_CENTER_BOTTOM) {
        // CENTER
        *x_align_mode = 0;
        *align_offset_x = -width_sum[line] / 2.0f;
    } else {
        // RIGHT
        *x_align_mode = 1;
        *align_offset_x = (float)-width_sum[line];
    }
    // Vertical
    if (text_block->align == LTBA_LEFT_TOP
        || text_block->align == LTBA_CENTER_TOP
        || text_block->align == LTBA_RIGHT_TOP) {
        // TOP
        *y_align_mode = 1;
    } else if (text_block->align == LTBA_LEFT_CENTER
               || text_block->align == LTBA_CENTER_CENTER
               || text_block->align == LTBA_RIGHT_CENTER) {
        // CENTER
        *y_align_mode = 0;
        *align_offset_y = (height_sum - first_height) / 2.0f;
    } else {
        // BOTTOM
        *y_align_mode = -1;
    }
    // Pixel perfect (screen-space coordinates) or UI (normalized) coordinates
    if (text_block->pixel_perfect == 0) {
        *align_offset_x = *align_offset_x / pLwc->viewport_width * 2 * pLwc->viewport_rt_x * prop_font_size;
        *align_offset_y = *align_offset_y / pLwc->viewport_height * 2 * pLwc->viewport_rt_y * prop_font_size;
    }
}

static void render_font_glyph(const LWCONTEXT* pLwc,
                              const LWTEXTBLOCK* text_block,
                              int unicode_strlen,
                              const unsigned int* unicode_str,
                              int width_sum[MAX_TEXT_BLOCK_LINE],
                              int height_sum,
                              int first_width,
                              int first_width_set,
                              int first_height,
                              const BMF_CHAR** bc,
                              int shader_index,
                              LW_VBO_TYPE lvt,
                              float prop_font_size,
                              float* size_scaled_xadvance_accum,
                              float* max_size_scaled_xadvance_accum,
                              const float color_normal_glyph[4],
                              const float color_normal_outline[4],
                              const float color_emp_glyph[4],
                              const float color_emp_outline[4],
                              int query_only) {
    const int font_base = font_get_base(pLwc->pFnt);
    const int font_line_height = font_get_line_height(pLwc->pFnt);
    
    int line = 0;
    int color_emp = 0;
    
    float last_x = 0;
    float last_size_scaled_xadvance = 0;

    *size_scaled_xadvance_accum = 0;
    *max_size_scaled_xadvance_accum = 0;

    float align_offset_x = 0;
    float align_offset_y = 0;
    int x_align_mode = 0;
    int y_align_mode = 0;
    // update align offset and align mode for the first line
    get_align_offset_and_mode(pLwc,
                              text_block,
                              line,
                              width_sum,
                              height_sum,
                              first_height,
                              prop_font_size,
                              &align_offset_x,
                              &align_offset_y,
                              &x_align_mode,
                              &y_align_mode);
    mat4x4 model_translate;
    mat4x4 model;
    mat4x4 identity_view;
    mat4x4 view_model;
    mat4x4 proj_view_model;
    mat4x4 model_scale;
    
    mat4x4_identity(identity_view);
    
    for (int i = 0; i < unicode_strlen; i++) {
        if (unicode_str[i] == '\n') {
            if (text_block->multiline) {
                *max_size_scaled_xadvance_accum = LWMAX(*max_size_scaled_xadvance_accum, *size_scaled_xadvance_accum);
                *size_scaled_xadvance_accum = 0;
                line++;
                // update align offset and align mode for the next line
                get_align_offset_and_mode(pLwc,
                                          text_block,
                                          line,
                                          width_sum,
                                          height_sum,
                                          first_height,
                                          prop_font_size,
                                          &align_offset_x,
                                          &align_offset_y,
                                          &x_align_mode,
                                          &y_align_mode);
                continue;
            } else {
                break;
            }
        }
        
        const BMF_CHAR* bci = bc[i];
        if (bci) {
            float ui_scale_x = 0;
            float ui_scale_y = 0;
            if (text_block->pixel_perfect == 0) {
                ui_scale_x = prop_font_size * (float)bci->width / pLwc->viewport_width * pLwc->viewport_rt_x;
                ui_scale_y = prop_font_size * (float)bci->height / pLwc->viewport_height * pLwc->viewport_rt_y;
            } else {
                //const float scale_per_pixel_x = 2.0f * pLwc->viewport_rt_x / pLwc->viewport_width / 2;
                //const float scale_per_pixel_y = 2.0f * pLwc->viewport_rt_y / pLwc->viewport_height / 2;
                ui_scale_x = prop_font_size * (float)bci->width / 2;
                ui_scale_y = prop_font_size * (float)bci->height / 2;
            }
            
            mat4x4_identity(model_scale);
            mat4x4_scale_aniso(model_scale, model_scale, ui_scale_x, ui_scale_y, 1.0f);
            
            float xoffset = 0;//((float)+bci->xoffset + *size_scaled_xadvance_accum) / pLwc->viewport_width * 2;
            float yoffset = 0;//((float)-bci->yoffset) / pLwc->viewport_height * 2;
            
            if (y_align_mode == 1) {
                yoffset = ((float)-bci->yoffset + font_line_height - font_base);// / pLwc->viewport_height * 2;
            } else if (y_align_mode == 0) {
                yoffset = ((float)font_line_height / 2 - bci->yoffset - bci->height / 2);// / pLwc->viewport_height * 2;
            } else {
                yoffset = ((float)font_base - bci->yoffset - bci->height);// / pLwc->viewport_height * 2;
            }
            if (text_block->pixel_perfect == 0) {
                yoffset *= 2.0f / pLwc->viewport_height;
            }
            
            if (x_align_mode == -1) {
                xoffset = ((float)+bci->xoffset + *size_scaled_xadvance_accum);// / pLwc->viewport_width * 2;
            } else if (x_align_mode == 0) {
                xoffset = ((float)bci->xoffset + (float)bci->width / 2 + *size_scaled_xadvance_accum);// / pLwc->viewport_width * 2;
            } else {
                xoffset = ((float)bci->xoffset + (float)bci->width + *size_scaled_xadvance_accum);// / pLwc->viewport_width * 2;
            }
            if (text_block->pixel_perfect == 0) {
                xoffset *= 2.0f / pLwc->viewport_width;
            }
            
            const float x = align_offset_x + text_block->text_block_x + prop_font_size * xoffset * ((text_block->pixel_perfect == 0) ? pLwc->viewport_rt_x : 1);
            const float y = align_offset_y + text_block->text_block_y + prop_font_size * yoffset * ((text_block->pixel_perfect == 0) ? pLwc->viewport_rt_y : 1) + (line * -(pLwc->viewport_rt_y * 2.0f / pLwc->viewport_height * prop_font_size * font_line_height));

            
            last_x = x;
            
            mat4x4_translate(model_translate, x, y, 0);
            
            mat4x4_identity(model);
            mat4x4_mul(model, model_translate, model_scale);
            
            mat4x4_mul(view_model, identity_view, model);
            
            mat4x4_identity(proj_view_model);
            mat4x4_mul(proj_view_model, pLwc->proj, view_model);
            
            if (query_only == 0) {
                glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE,
                                   (const GLfloat *)proj_view_model);
                glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0);
                if (shader_index == LWST_FONT) {
                    set_tex_filter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
                } else if (shader_index == LWST_PIXEL_FONT) {
                    set_tex_filter(GL_NEAREST, GL_NEAREST);
                }
                if (/* DISABLES CODE */ (0) && pLwc->font_texture_texture_mode) {
                    glBindTexture(GL_TEXTURE_2D, pLwc->tex_programmed[LPT_SOLID_WHITE_WITH_ALPHA]);
                    glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, default_uv_offset);
                    glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, default_uv_scale);
                } else {
                    glBindTexture(GL_TEXTURE_2D, pLwc->tex_font_atlas[bci->page]);
                    // TODO: font texture resolution hardcoded to 1024x1024
                    set_texture_parameter_values(pLwc, bci->x, bci->y, bci->width, bci->height, 1024, 1024, shader_index);
                }
            }
            
            if (unicode_str[i] == '<') {
                color_emp = 1;
                continue;
            }
            
            if (unicode_str[i] == '>') {
                color_emp = 0;
                continue;
            }
            
            if (query_only == 0) {
                glUniform4fv(pLwc->shader[shader_index].glyph_color_location, 1,
                             color_emp ? color_emp_glyph : color_normal_glyph);
                glUniform4fv(pLwc->shader[shader_index].outline_color_location, 1,
                             color_emp ? color_emp_outline : color_normal_outline);
                
                if (bci->id != ' ') {
                    // Vertex buffer is specified outside this loop
                    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
                }
            }
            
            //const BMF_CHAR* bci_next = bc[i + 1];
            
            const float outline_xadvance = 0;// (float)(color_emp ? emp_outline_thickness : normal_outline_thickness) / 2;
            
            if (x_align_mode == -1) {
                last_size_scaled_xadvance = bci->xadvance + outline_xadvance;
            } else if (x_align_mode == 0) {
                //last_size_scaled_xadvance = (bci_next ? ((float)-bci->width / 2 + bci->xadvance + bci_next->width / 2) : bci->xadvance) + outline_xadvance;
                last_size_scaled_xadvance = bci->xadvance + outline_xadvance;
                //last_size_scaled_xadvance = (bci_next ? ((float)bci->xadvance / 2 + bci_next->xadvance / 2) : ((float)bci->xadvance / 2)) + outline_xadvance;
            } else {
                last_size_scaled_xadvance = bci->xadvance + outline_xadvance;
            }
            *size_scaled_xadvance_accum += last_size_scaled_xadvance;
            
            float size_scaled_xadvance_accum_scaled = prop_font_size * (*size_scaled_xadvance_accum);
            if (text_block->pixel_perfect == 0) {
                size_scaled_xadvance_accum_scaled *= 2 * pLwc->viewport_rt_x / pLwc->viewport_width;
			} else {
				size_scaled_xadvance_accum_scaled *= prop_font_size;
			}

            if (size_scaled_xadvance_accum_scaled > text_block->text_block_width) {
                *size_scaled_xadvance_accum = 0;
                line++;
            }
        }
    }
    *max_size_scaled_xadvance_accum = LWMAX(*max_size_scaled_xadvance_accum, *size_scaled_xadvance_accum);
}

void render_text_block(const LWCONTEXT* pLwc, const LWTEXTBLOCK* text_block) {
    render_text_block_alpha(pLwc, text_block, 1.0f);
}

void render_text_block_alpha(const LWCONTEXT* pLwc, const LWTEXTBLOCK* text_block, float ui_alpha) {
    render_query_text_block_alpha(pLwc, text_block, ui_alpha, 0, 0);
}

void render_query_text_block_alpha(const LWCONTEXT* pLwc, const LWTEXTBLOCK* text_block, float ui_alpha, LWTEXTBLOCKQUERYRESULT* query_result, int query_only) {
    // Font data not ready
    if (!pLwc->pFnt) {
        return;
    }
    /* test7.fnt */
    int shader_index = LWST_FONT;
    /* neo.fnt */
    //int shader_index = LWST_PIXEL_FONT;
    LW_VBO_TYPE lvt = LVT_DONTCARE;
    if (query_only == 0) {
        lazy_glUseProgram(pLwc, shader_index);

        glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, default_uv_offset);
        glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, default_uv_scale);
        glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, ui_alpha);

        glActiveTexture(GL_TEXTURE0);
        // 0 means GL_TEXTURE0
        glUniform1i(pLwc->shader[shader_index].diffuse_location, 0);
        // default texture param
        glBindTexture(GL_TEXTURE_2D, pLwc->tex_font_atlas[0]);

        glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE,
            (const GLfloat *)pLwc->proj);


        lvt = LVT_LEFT_TOP_ANCHORED_SQUARE + (text_block->align - LTBA_LEFT_TOP);

        lazy_glBindBuffer(pLwc, lvt);
        bind_all_vertex_attrib_font(pLwc, lvt, shader_index);
        glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, default_uv_offset);
        glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, default_uv_scale);
    }


    unsigned int unicode_str[MAX_UNICODE_STRLEN];
    size_t unicode_strlen = u8_toucs(
        unicode_str,
        ARRAY_SIZE(unicode_str),
        text_block->text + text_block->begin_index,
        text_block->end_index - text_block->begin_index
    );

    if (unicode_strlen >= MAX_UNICODE_STRLEN) {
        LOGE("CRITICAL RUNTIME ERROR! unicode_strlen overflow");
    }
    unicode_str[MAX_UNICODE_STRLEN - 1] = '\0';

    const BMF_CHAR* bc[MAX_UNICODE_STRLEN];
    // Calculate total font block size
    int width_sum[MAX_TEXT_BLOCK_LINE] = { 0, };
    int line = 0;
    int height_sum = 0;
    int first_width = 0;
    int first_width_set = 0;
    int first_height = 0;
    const int font_line_height = font_get_line_height(pLwc->pFnt);
    for (size_t i = 0; i < unicode_strlen; i++) {
        bc[i] = font_binary_search_char(pLwc->pFnt, unicode_str[i]);
        if (unicode_str[i] == '<' || unicode_str[i] == '>') {
            continue;
        }
        if (bc[i]) {
            const int width = bc[i]->xadvance; // : bc[i]->width);
            if (height_sum == 0) {
                height_sum = font_line_height;
                first_height = font_line_height;
            }
            if (unicode_str[i] == '\n') {
                height_sum += font_line_height;
                line++;
            } else {
                width_sum[line] += width;
            }

            if (!first_width_set) {
                first_width = width;
                first_width_set = 1;
            }
        }
    }
    bc[unicode_strlen] = 0;

    float prop_font_size = 1.0f;
    if (text_block->pixel_perfect == 0) {
        prop_font_size = get_proportional_font_size(pLwc->viewport_width, pLwc->viewport_height, text_block->size);
    }
    float size_scaled_xadvance_accum = 0;
    float max_size_scaled_xadvance_accum = 0;
    float color_normal_outline_null[4];
    float color_emp_outline_null[4];
    memcpy(color_normal_outline_null, text_block->color_normal_glyph, sizeof(color_normal_outline_null));
    memcpy(color_emp_outline_null, text_block->color_emp_glyph, sizeof(color_emp_outline_null));
    color_normal_outline_null[3] *= 0;
    color_emp_outline_null[3] *= 0;
    const float* colors[][4] = {
        {
            text_block->color_normal_glyph,
            text_block->color_normal_outline,
            text_block->color_emp_glyph,
            text_block->color_emp_outline,
        },
        {
            text_block->color_normal_glyph,
            color_normal_outline_null,
            text_block->color_emp_glyph,
            color_emp_outline_null,
        },
    };
    for (int i = 0; i < 1; i++) {
        // render glyph
        render_font_glyph(pLwc,
                          text_block,
                          (int)unicode_strlen,
                          unicode_str,
                          width_sum,
                          height_sum,
                          first_width,
                          first_width_set,
                          first_height,
                          bc,
                          shader_index,
                          lvt,
                          prop_font_size,
                          &size_scaled_xadvance_accum,
                          &max_size_scaled_xadvance_accum,
                          colors[i][0],
                          colors[i][1],
                          colors[i][2],
                          colors[i][3],
                          query_only);
    }

    if (query_only == 0) {
        glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0);

        // Render text block debug indicator
        if (pLwc->font_texture_texture_mode) {
            const float width_pixels = (float)(2 * pLwc->viewport_rt_x) / pLwc->viewport_width;
            const float height_pixels = (float)(2 * pLwc->viewport_rt_y) / pLwc->viewport_height;

            //const float w = 2 * width_pixels;
            const float h = 2 * height_pixels;

            // text total width debug indicator
            render_solid_vb_ui(
                pLwc,
                text_block->text_block_x,
                text_block->text_block_y,
                prop_font_size * max_size_scaled_xadvance_accum * pLwc->viewport_rt_x / pLwc->viewport_width * 2,
                h,
                pLwc->tex_programmed[LPT_SOLID_RED],
                lvt,
                1,
                0,
                0,
                0,
                0
            );

            const float w2 = 2 * width_pixels;
            const float h2 = 2 * height_pixels;

            // text origin point debug indicator
            render_solid_vb_ui(
                pLwc,
                text_block->text_block_x,
                text_block->text_block_y,
                w2,
                h2,
                pLwc->tex_programmed[LPT_SOLID_YELLOW],
                LVT_CENTER_CENTER_ANCHORED_SQUARE,
                1,
                0,
                0,
                0,
                0
            );
        }
    }

    if (query_result) {
        query_result->total_glyph_width = prop_font_size * size_scaled_xadvance_accum;
        if (text_block->pixel_perfect == 0) {
          query_result->total_glyph_width *= pLwc->viewport_rt_x / pLwc->viewport_width * 2;
        }
        query_result->glyph_height = text_block->text_block_line_height;
    }
}

// avoid outline overlapping by rendering the same text block twice:
// (1) black outline once
// (2) overwrite white glphy once
void render_text_block_two_pass(const LWCONTEXT* pLwc, LWTEXTBLOCK* text_block) {
    LWTEXTBLOCK text_block_bg;
    memcpy(&text_block_bg, text_block, sizeof(LWTEXTBLOCK));
    SET_COLOR_RGBA_FLOAT(text_block_bg.color_normal_glyph, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block_bg.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block_bg.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block_bg.color_emp_outline, 0, 0, 0, 1);
    render_text_block(pLwc, &text_block_bg);
    SET_COLOR_RGBA_FLOAT(text_block->color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(text_block->color_normal_outline, 1, 1, 1, 0); // should be 'white' even if alpha is zero; to correctly render thickness of the glyph
    SET_COLOR_RGBA_FLOAT(text_block->color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block->color_emp_outline, 0, 0, 0, 1);
    render_text_block(pLwc, text_block);
}

void render_text_block_two_pass_color(const LWCONTEXT* pLwc, LWTEXTBLOCK* text_block) {
    LWTEXTBLOCK text_block_bg;
    memcpy(&text_block_bg, text_block, sizeof(LWTEXTBLOCK));
    SET_COLOR_RGBA_FLOAT(text_block_bg.color_normal_glyph, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block_bg.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block_bg.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block_bg.color_emp_outline, 0, 0, 0, 1);
    render_text_block(pLwc, &text_block_bg);
    //SET_COLOR_RGBA_FLOAT(text_block->color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(text_block->color_normal_outline, text_block->color_normal_glyph[0], text_block->color_normal_glyph[1], text_block->color_normal_glyph[2], 0); // should be 'white' even if alpha is zero; to correctly render thickness of the glyph
    SET_COLOR_RGBA_FLOAT(text_block->color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block->color_emp_outline, 0, 0, 0, 1);
    render_text_block(pLwc, text_block);
}
