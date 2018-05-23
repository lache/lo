#include <stdio.h>
#include <string.h>
#include "render_battle_result.h"
#include "lwcontext.h"
#include "render_text_block.h"
#include "render_solid.h"
#include "render_battle.h"
#include "lwuidim.h"
#include "laidoff.h"

const float header_y_center = 0.9f;
const float header_height = 0.2f;
const float footer_y_center = -0.8f;
const float footer_height = 0.2f;

void get_player_creature_result_ui_box(int pos, float screen_aspect_ratio, float* left_top_x, float* left_top_y, float* area_width, float* area_height) {
    
    get_player_creature_ui_box(pos, screen_aspect_ratio, left_top_x, left_top_y, area_width, area_height);
    
    // Align area to the vertically middle
    *left_top_y = *area_height / 2;
}

void render_player_creature_battle_result_ui(const LWCONTEXT* pLwc, const LWBATTLECREATURE* c, int pos) {
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_LEFT_TOP;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_E;
    text_block.multiline = 1;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
    
    
    
    
    float left_top_x = 0;
    float left_top_y = 0;
    
    float area_width = 0;
    float area_height = 0;
    
    get_player_creature_result_ui_box(pos, pLwc->aspect_ratio, &left_top_x, &left_top_y, &area_width, &area_height);
    
    const float block_x_margin = 0.075f * pLwc->aspect_ratio;
    const float block_y_margin = 0.025f;
    
    const float bar_width = area_width - block_x_margin * 2;
    const float bar_height = 0.03f;
    
    // Square portrait
    const float square_portrait_size = LWMIN(area_width - 2 * block_x_margin, 1 - left_top_y - header_height - 2 * block_y_margin);
    lw_load_tex(pLwc, c->square_portrait_lae);
    lw_load_tex(pLwc, c->square_portrait_lae + 1);
    render_solid_vb_ui_alpha(pLwc,
                             left_top_x + area_width / 2,
                             left_top_y + square_portrait_size / 2,
                             square_portrait_size,
                             square_portrait_size,
                             pLwc->tex_atlas[c->square_portrait_lae],
                             pLwc->tex_atlas[c->square_portrait_lae + 1],
                             LVT_CENTER_CENTER_ANCHORED_SQUARE,
                             1,
                             0,
                             0,
                             0,
                             0);
    
    // Lv.
    {
        char str[32];
        sprintf(str, "Lv.%d", c->lv);
        text_block.text = str;
        text_block.text_bytelen = (int)strlen(text_block.text);
        text_block.begin_index = 0;
        text_block.end_index = text_block.text_bytelen;
        text_block.text_block_x = left_top_x + block_x_margin;
        text_block.text_block_y = left_top_y - block_y_margin;
        
        render_text_block(pLwc, &text_block);
    }
    
    // Name
    {
        text_block.text = c->name;
        text_block.text_bytelen = (int)strlen(text_block.text);
        text_block.begin_index = 0;
        text_block.end_index = text_block.text_bytelen;
        text_block.text_block_y -= text_block.text_block_line_height;
        
        render_text_block(pLwc, &text_block);
    }
    
    // EXP
    {
        char str[32];
        sprintf(str, "EXP %d", c->render_exp);
        text_block.text = str;
        text_block.text_bytelen = (int)strlen(text_block.text);
        text_block.begin_index = 0;
        text_block.end_index = text_block.text_bytelen;
        text_block.text_block_y -= text_block.text_block_line_height;
        render_text_block(pLwc, &text_block);
        
        sprintf(str, "+%d", c->earn_exp);
        text_block.text = str;
        text_block.text_bytelen = (int)strlen(text_block.text);
        text_block.begin_index = 0;
        text_block.end_index = text_block.text_bytelen;
        text_block.text_block_x += bar_width; // temporary change
        text_block.align = LTBA_RIGHT_TOP; // temporary change
        SET_COLOR_RGBA_FLOAT_ARRAY(text_block.color_normal_glyph, EXP_COLOR); // temporary change
        render_text_block(pLwc, &text_block);
        
        // Revert value
        text_block.text_block_x -= bar_width;
        text_block.align = LTBA_LEFT_TOP;
    }
    
    const float bar_boundary_thickness = 0.015f;
    
    render_solid_box_ui(
                        pLwc,
                        text_block.text_block_x - bar_boundary_thickness / 2,
                        text_block.text_block_y - text_block.text_block_line_height - bar_boundary_thickness / 2,
                        bar_width + bar_boundary_thickness,
                        bar_height + bar_boundary_thickness,
                        pLwc->tex_programmed[LPT_SOLID_BLACK]
                        );
    
    render_solid_box_ui(
                        pLwc,
                        text_block.text_block_x,
                        text_block.text_block_y - text_block.text_block_line_height,
                        bar_width,
                        bar_height,
                        pLwc->tex_programmed[LPT_SOLID_GRAY]
                        );
    
    render_solid_box_ui(
                        pLwc,
                        text_block.text_block_x,
                        text_block.text_block_y - text_block.text_block_line_height,// + 0.01f,
                        bar_width * (float)c->render_exp / c->max_exp,
                        bar_height,
                        pLwc->tex_programmed[LPT_SOLID_EXP_COLOR]
                        );
}

void render_header(const LWCONTEXT* pLwc) {
    
    render_solid_vb_ui_flip_y_uv(pLwc, 0, header_y_center, 2 * pLwc->aspect_ratio, header_height,
                                 pLwc->tex_programmed[LPT_BOTH_END_GRADIENT_HORIZONTAL], LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                 1, 39 / 255.0f, 74 / 255.0f, 110 / 255.0f, 1.0f, 0);
    
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_CENTER_CENTER;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_A;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
    text_block.text = LWU("BATTLE RESULT");
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = 0;
    text_block.text_block_y = header_y_center;
    text_block.multiline = 1;
    render_text_block(pLwc, &text_block);
}

void get_battle_result_next_button_dim(LWUIDIM* d) {
    d->align = LTBA_CENTER_CENTER;
    d->x = 0;
    d->y = footer_y_center;
    d->w = 1.0;
    d->h = footer_height;
}

void render_next_button(const LWCONTEXT* pLwc) {
    LWUIDIM next_button_d;
    get_battle_result_next_button_dim(&next_button_d);
    render_solid_vb_ui_flip_y_uv(pLwc,
                                 next_button_d.x, next_button_d.y,
                                 next_button_d.w, next_button_d.h,
                                 pLwc->tex_programmed[LPT_BOTH_END_GRADIENT_HORIZONTAL], LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                 1, 94 / 255.0f, 207 / 255.0f, 73 / 255.0f, 1.0f, 0);
    
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_CENTER_CENTER;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_C;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
    text_block.text = LWU("OK");
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = 0;
    text_block.text_block_y = footer_y_center;
    text_block.multiline = 1;
    render_text_block(pLwc, &text_block);
}

void render_footer_button(const LWCONTEXT* pLwc) {
    render_next_button(pLwc);
}

void lwc_render_battle_result(const LWCONTEXT* pLwc) {
    LW_GL_VIEWPORT();
    lw_clear_color();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    render_header(pLwc);
    
    // Player battle creature UI
    ARRAY_ITERATE_VALID(const LWBATTLECREATURE, pLwc->player) {
        render_player_creature_battle_result_ui(pLwc, e, i);
    } ARRAY_ITERATE_VALID_END();
    
    render_damage_text(pLwc, pLwc->battle_view, pLwc->battle_proj, pLwc->proj, 1.0f);
    
    render_footer_button(pLwc);
}

