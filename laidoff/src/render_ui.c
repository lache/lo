#include "render_ui.h"
#include "lwgl.h"
#include "lwcontext.h"
#include "laidoff.h"
#include "render_solid.h"
#include "render_text_block.h"
#include "lwmacro.h"
#include "lwbutton.h"
#include <string.h>
#include <stdio.h>

const float scrap_bg_width = 7.0f;
const float scrap_bg_height = 1.0f;

static void s_create_scrap_bg_vbo(LWCONTEXT* pLwc) {
    const float c0[3] = { 0.2f, 0.2f, 0.2f };
    const float c1[3] = { 0.3f, 0.3f, 0.3f };
    const float c2[3] = { 0.4f, 0.4f, 0.4f };
    const LWVERTEX scrap_bg[] =
    {
        { 0,                                -scrap_bg_height,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
        { scrap_bg_width - scrap_bg_height,    -scrap_bg_height,    0, c1[0], c1[1], c1[2], 1, 1, 0, 0 },
        { scrap_bg_width,                    0,                    0, c2[0], c2[1], c2[2], 1, 0, 0, 0 },
        { scrap_bg_width,                    0,                    0, c2[0], c2[1], c2[2], 1, 0, 0, 0 },
        { scrap_bg_height,                    0,                    0, c1[0], c1[1], c1[2], 0, 0, 0, 0 },
        { 0,                                -scrap_bg_height,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
    };
    LWVERTEX square_vertices[ARRAY_SIZE(scrap_bg)];
    memcpy(square_vertices, scrap_bg, sizeof(scrap_bg));
    
    const LW_VBO_TYPE lvt = LVT_UI_SCRAP_BG;
    
    glGenBuffers(1, &pLwc->vertex_buffer[lvt].vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(LWVERTEX) * ARRAY_SIZE(scrap_bg),
                 square_vertices, GL_STATIC_DRAW);
    pLwc->vertex_buffer[lvt].vertex_count = ARRAY_SIZE(scrap_bg);
}

const float tower_button_height = 1.0f;
const float tower_button_width = 1.0f;
const float tower_button_tag_width = 1.2f;
const float tower_button_tag_height = 0.35f;
const float tower_button_height_margin = 0.2f;
const float tower_button_border = 0.075f;

static void s_create_tower_button_bg_vbo(LWCONTEXT* pLwc) {
    const float c0[3] = { 0.18f, 0.46f, 0.71f };
    const float c1[3] = { 0.18f, 0.46f, 0.71f };
    const float c2[3] = { 0.18f, 0.46f, 0.71f };
    const LWVERTEX tower_button_bg[] =
    {
        { 0,                                -tower_button_height,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
        { tower_button_width,    -scrap_bg_height,    0, c1[0], c1[1], c1[2], 1, 1, 0, 0 },
        { tower_button_width,                    -tower_button_tag_height,                    0, c2[0], c2[1], c2[2], 1, 0, 0, 0 },
        
        { tower_button_width,                    -tower_button_tag_height,                    0, c2[0], c2[1], c2[2], 1, 0, 0, 0 },
        { tower_button_width + tower_button_tag_width,                    -tower_button_tag_height,                    0, c1[0], c1[1], c1[2], 0, 0, 0, 0 },
        { tower_button_width + tower_button_tag_width,                    0,                    0, c1[0], c1[1], c1[2], 0, 0, 0, 0 },
        
        { tower_button_width + tower_button_tag_width,                    0,                    0, c1[0], c1[1], c1[2], 0, 0, 0, 0 },
        { 0,                                0,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
        { tower_button_width,                    -tower_button_tag_height,                    0, c2[0], c2[1], c2[2], 1, 0, 0, 0 },
        
        { tower_button_width,                    -tower_button_tag_height,                    0, c2[0], c2[1], c2[2], 1, 0, 0, 0 },
        { 0,                                0,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
        { 0,                                -tower_button_height,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
    };
    LWVERTEX square_vertices[ARRAY_SIZE(tower_button_bg)];
    memcpy(square_vertices, tower_button_bg, sizeof(tower_button_bg));
    
    const LW_VBO_TYPE lvt = LVT_UI_TOWER_BUTTON_BG;
    
    glGenBuffers(1, &pLwc->vertex_buffer[lvt].vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(LWVERTEX) * ARRAY_SIZE(tower_button_bg),
                 square_vertices, GL_STATIC_DRAW);
    pLwc->vertex_buffer[lvt].vertex_count = ARRAY_SIZE(tower_button_bg);
}

const float left_button_height = 1.5f;
const float left_button_width = 1.0f;
const float left_button_left_edge_width = 1.0f;
const float left_button_right_edge_width = 2.0f;
const float left_button_width_margin = 0.6f;

static void s_create_left_button_bg_vbo(LWCONTEXT* pLwc) {
    const float c0[3] = { 0.18f, 0.46f, 0.71f };
    const float c1[3] = { 0.18f, 0.46f, 0.71f };
    const float c2[3] = { 0.18f, 0.46f, 0.71f };
    const LWVERTEX left_button_bg[] =
    {
        { 0,                                -left_button_height/2,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
        { left_button_width,    -left_button_height / 2,    0, c1[0], c1[1], c1[2], 1, 1, 0, 0 },
        { left_button_width + left_button_right_edge_width,                    left_button_height / 2,                    0, c2[0], c2[1], c2[2], 1, 0, 0, 0 },
        
        { left_button_width + left_button_right_edge_width,                    left_button_height / 2,                    0, c2[0], c2[1], c2[2], 1, 0, 0, 0 },
        { 0,                                left_button_height / 2,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
        { 0,                                -left_button_height / 2,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
        
        { 0,                                -left_button_height / 2,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
        { 0,                                left_button_height / 2,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
        { -left_button_left_edge_width,                    0,                    0, c2[0], c2[1], c2[2], 1, 0, 0, 0 },
    };
    LWVERTEX square_vertices[ARRAY_SIZE(left_button_bg)];
    memcpy(square_vertices, left_button_bg, sizeof(left_button_bg));
    
    const LW_VBO_TYPE lvt = LVT_UI_LEFT_BUTTON_BG;
    
    glGenBuffers(1, &pLwc->vertex_buffer[lvt].vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(LWVERTEX) * ARRAY_SIZE(left_button_bg),
                 square_vertices, GL_STATIC_DRAW);
    pLwc->vertex_buffer[lvt].vertex_count = ARRAY_SIZE(left_button_bg);
}

const float full_panel_height = 1.75f;
const float full_panel_width = 2.8f;
const float full_panel_trim = 0.2f;
const float full_panel_left_edge_width = 1.0f;
const float full_panel_right_edge_width = 2.0f;
const float full_panel_width_margin = 0.6f;

static void s_create_full_panel_bg_vbo(LWCONTEXT* pLwc) {
    const float c0[3] = { 0.2f, 0.2f, 0.2f };
    //const float c1[3] = { 0.3f, 0.3f, 0.3f };
    //const float c2[3] = { 0.4f, 0.4f, 0.4f };
    const LWVERTEX full_panel_bg[] = {
        { -full_panel_width/2 + full_panel_trim, -full_panel_height / 2,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
        { full_panel_width/2,    -full_panel_height / 2,    0, c0[0], c0[1], c0[2], 1, 1, 0, 0 },
        { full_panel_width / 2,                    full_panel_height / 2 - full_panel_trim,                    0, c0[0], c0[1], c0[2], 1, 0, 0, 0 },
        
        { full_panel_width / 2,                    full_panel_height / 2 - full_panel_trim,                    0, c0[0], c0[1], c0[2], 1, 0, 0, 0 },
        { full_panel_width / 2 - full_panel_trim, full_panel_height / 2,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
        { -full_panel_width / 2 + full_panel_trim, -full_panel_height / 2,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
        
        { -full_panel_width / 2 + full_panel_trim, -full_panel_height / 2,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
        { full_panel_width / 2 - full_panel_trim, full_panel_height / 2,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
        { -full_panel_width / 2,                    -full_panel_height / 2 + full_panel_trim,                    0, c0[0], c0[1], c0[2], 1, 0, 0, 0 },
        
        { -full_panel_width / 2,                    -full_panel_height / 2 + full_panel_trim,                    0, c0[0], c0[1], c0[2], 1, 0, 0, 0 },
        { full_panel_width / 2 - full_panel_trim, full_panel_height / 2,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
        { -full_panel_width / 2, full_panel_height / 2,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
    };
    LWVERTEX square_vertices[ARRAY_SIZE(full_panel_bg)];
    memcpy(square_vertices, full_panel_bg, sizeof(full_panel_bg));
    
    const LW_VBO_TYPE lvt = LVT_UI_FULL_PANEL_BG;
    
    glGenBuffers(1, &pLwc->vertex_buffer[lvt].vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(LWVERTEX) * ARRAY_SIZE(full_panel_bg),
                 square_vertices, GL_STATIC_DRAW);
    pLwc->vertex_buffer[lvt].vertex_count = ARRAY_SIZE(full_panel_bg);
}

const float button_height = 1.0f;
const float button_width = 6.0f;
const float button_trim = 0.2f;
const float button_depth = 0.1f;

static void s_create_button_bg_vbo(LWCONTEXT* pLwc) {
    const float c0[3] = { 0.18f, 0.46f, 0.71f };
    const float c1[3] = { 0.18f, 0.46f, 0.71f };
    const float c2[3] = { 0.18f, 0.46f, 0.71f };
    const float c3[3] = { 0.1f, 0.2f, 0.3f };
#define BTN_PC { 0, 0,    0, c0[0], c0[1], c0[2], 0, 1, 0, 0 }
#define BTN_P0 { -button_width/2+button_trim, -button_height/2,    0, c2[0], c2[1], c2[2], 0, 1, 0, 0 }
#define BTN_P1 { +button_width/2-button_trim, -button_height/2,    0, c2[0], c2[1], c2[2], 0, 1, 0, 0 }
#define BTN_P2 { +button_width/2, -button_height/2 + button_trim,    0, c2[0], c2[1], c2[2], 0, 1, 0, 0 }
#define BTN_P3 { +button_width/2, +button_height/2 - button_trim,    0, c1[0], c1[1], c1[2], 0, 1, 0, 0 }
#define BTN_P4 { +button_width/2-button_trim, +button_height/2,    0, c1[0], c1[1], c1[2], 0, 1, 0, 0 }
#define BTN_P5 { -button_width/2+button_trim, +button_height/2,    0, c1[0], c1[1], c1[2], 0, 1, 0, 0 }
#define BTN_P6 { -button_width/2, +button_height/2 - button_trim,    0, c1[0], c1[1], c1[2], 0, 1, 0, 0 }
#define BTN_P7 { -button_width/2, -button_height/2 + button_trim,    0, c2[0], c2[1], c2[2], 0, 1, 0, 0 }
    // Button depth vertices
#define BTN_P0U { -button_width/2+button_trim, -button_height/2,    0, c3[0], c3[1], c3[2], 0, 1, 0, 0 }
#define BTN_P1U { +button_width/2-button_trim, -button_height/2,    0, c3[0], c3[1], c3[2], 0, 1, 0, 0 }
#define BTN_P2U { +button_width/2, -button_height/2 + button_trim,    0, c3[0], c3[1], c3[2], 0, 1, 0, 0 }
#define BTN_P7U { -button_width/2, -button_height/2 + button_trim,    0, c3[0], c3[1], c3[2], 0, 1, 0, 0 }
#define BTN_P0B { -button_width/2+button_trim, -button_height/2 - button_depth,    0, c3[0], c3[1], c3[2], 0, 1, 0, 0 }
#define BTN_P1B { +button_width/2-button_trim, -button_height/2 - button_depth,    0, c3[0], c3[1], c3[2], 0, 1, 0, 0 }
#define BTN_P2B { +button_width/2, -button_height/2 + button_trim - button_depth,    0, c3[0], c3[1], c3[2], 0, 1, 0, 0 }
#define BTN_P7B { -button_width/2, -button_height/2 + button_trim - button_depth,    0, c3[0], c3[1], c3[2], 0, 1, 0, 0 }
    const LWVERTEX btn_bg[] = {
        BTN_P0, BTN_P1, BTN_PC,
        BTN_P1, BTN_P2, BTN_PC,
        BTN_P2, BTN_P3, BTN_PC,
        BTN_P3, BTN_P4, BTN_PC,
        BTN_P4, BTN_P5, BTN_PC,
        BTN_P5, BTN_P6, BTN_PC,
        BTN_P6, BTN_P7, BTN_PC,
        BTN_P7, BTN_P0, BTN_PC,
        // Button depth
        BTN_P7B, BTN_P0U, BTN_P7U,
        BTN_P7B, BTN_P0B, BTN_P0U,
        BTN_P0B, BTN_P1U, BTN_P0U,
        BTN_P0B, BTN_P1B, BTN_P1U,
        BTN_P1B, BTN_P2U, BTN_P1U,
        BTN_P1B, BTN_P2B, BTN_P2U,
    };
#undef BTN_PC
#undef BTN_P0
#undef BTN_P1
#undef BTN_P2
#undef BTN_P3
#undef BTN_P4
#undef BTN_P5
#undef BTN_P6
#undef BTN_P7
    LWVERTEX square_vertices[ARRAY_SIZE(btn_bg)];
    memcpy(square_vertices, btn_bg, sizeof(btn_bg));
    
    const LW_VBO_TYPE lvt = LVT_UI_BUTTON_BG;
    
    glGenBuffers(1, &pLwc->vertex_buffer[lvt].vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(LWVERTEX) * ARRAY_SIZE(btn_bg),
                 square_vertices, GL_STATIC_DRAW);
    pLwc->vertex_buffer[lvt].vertex_count = ARRAY_SIZE(btn_bg);
}

void lwc_create_ui_vbo(LWCONTEXT* pLwc) {
    s_create_scrap_bg_vbo(pLwc);
    s_create_tower_button_bg_vbo(pLwc);
    s_create_left_button_bg_vbo(pLwc);
    s_create_full_panel_bg_vbo(pLwc);
    s_create_button_bg_vbo(pLwc);
}

static void s_render_scrap(const LWCONTEXT* pLwc, LWBUTTONLIST* button_list) {
    const float scrap_bg_size_nor = 0.15f;
    const float scrap_bg_x_nor = pLwc->viewport_aspect_ratio - scrap_bg_width * scrap_bg_size_nor;
    const float scrap_bg_y_nor = 1.0f;
    lw_load_tex(pLwc, LAE_3D_APT_TEX_MIP_KTX);
    render_solid_vb_ui_flip_y_uv_shader(pLwc,
                                        scrap_bg_x_nor,
                                        scrap_bg_y_nor,
                                        2 * scrap_bg_size_nor,
                                        2 * scrap_bg_size_nor,
                                        pLwc->tex_atlas[LAE_3D_APT_TEX_MIP_KTX],
                                        LVT_UI_SCRAP_BG,
                                        1,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        LWST_COLOR);
    
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_LEFT_CENTER;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_B;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 0);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 0);
    char msg[128];
    sprintf(msg, "[=] 888,888");
    text_block.text = msg;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = scrap_bg_x_nor + scrap_bg_size_nor;
    text_block.text_block_y = scrap_bg_y_nor - (scrap_bg_size_nor / 2);
    text_block.multiline = 1;
    render_text_block(pLwc, &text_block);
}

static void s_render_tower_button(const LWCONTEXT* pLwc, LWBUTTONLIST* button_list) {
    for (int i = 0; i < 4; i++) {
        const float btn_bg_size_nor = 0.3f;
        const float wf = (tower_button_width + tower_button_tag_width) * btn_bg_size_nor;
        const float hf = tower_button_height * btn_bg_size_nor;
        const float btn_bg_x_nor = pLwc->viewport_aspect_ratio - wf;
        const float btn_bg_y_nor = 1.0f - (tower_button_height_margin + tower_button_height) * btn_bg_size_nor * (i + 1);
        const float border_scaled = tower_button_border * btn_bg_size_nor;
        const float sprite_size_nor = btn_bg_size_nor * (1.0f - 2.0f * tower_button_border);
        // Button background (frame)
        render_solid_vb_ui_flip_y_uv_shader(pLwc, btn_bg_x_nor, btn_bg_y_nor, 2 * btn_bg_size_nor, 2 * btn_bg_size_nor,
                                            0, LVT_UI_TOWER_BUTTON_BG,
                                            1, 0, 0, 0, 0, 0, LWST_COLOR);
        // Tower thumbnail sprite
        lw_load_tex(pLwc, LAE_BB_CATAPULT + i);
        render_solid_vb_ui(pLwc,
                           btn_bg_x_nor + border_scaled,
                           btn_bg_y_nor - border_scaled,
                           sprite_size_nor,
                           sprite_size_nor,
                           pLwc->tex_atlas[LAE_BB_CATAPULT + i],
                           LVT_LEFT_TOP_ANCHORED_SQUARE,
                           1,
                           0,
                           0,
                           0,
                           0);
        // Scrap price
        LWTEXTBLOCK text_block;
        text_block.align = LTBA_LEFT_CENTER;
        text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
        text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
        text_block.size = DEFAULT_TEXT_BLOCK_SIZE_E;
        SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
        SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 0);
        SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
        SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 0);
        char msg[128];
        sprintf(msg, "[=] %d,000", i + 1);
        text_block.text = msg;
        text_block.text_bytelen = (int)strlen(text_block.text);
        text_block.begin_index = 0;
        text_block.end_index = text_block.text_bytelen;
        text_block.text_block_x = btn_bg_x_nor + (tower_button_width) * btn_bg_size_nor;
        text_block.text_block_y = btn_bg_y_nor - tower_button_tag_height / 2 * btn_bg_size_nor;
        text_block.multiline = 1;
        render_text_block(pLwc, &text_block);
        // Register as a button
        char btn_id[32];
        sprintf(btn_id, "seltower%d", i);
        lwbutton_append(pLwc, button_list, btn_id, btn_bg_x_nor, btn_bg_y_nor, wf, hf);
    }
}

static void s_render_tower_page_button(const LWCONTEXT* pLwc, LWBUTTONLIST* button_list) {
    const float left_button_total_width = left_button_width + left_button_left_edge_width + left_button_right_edge_width;
    
    const float btn_bg_size_nor = 0.1f;
    const float wf = left_button_total_width * btn_bg_size_nor;
    const float hf = left_button_height * btn_bg_size_nor;
    const float btn_bg_x_nor = pLwc->viewport_aspect_ratio - (left_button_width + left_button_right_edge_width + left_button_width_margin + left_button_width + left_button_left_edge_width) * btn_bg_size_nor;
    const float btn_bg_y_nor = -1.0f + left_button_height * btn_bg_size_nor;
    
    // Left arrow button
    render_solid_vb_ui_flip_y_uv_shader_rot(pLwc, btn_bg_x_nor, btn_bg_y_nor, 2 * btn_bg_size_nor, 2 * btn_bg_size_nor,
                                            0, LVT_UI_LEFT_BUTTON_BG,
                                            1, 0, 0, 0, 0, 0, LWST_COLOR, 0);
    // Register as a button
    lwbutton_append(pLwc, button_list, "seltower_left", btn_bg_x_nor - (left_button_width*btn_bg_size_nor), btn_bg_y_nor + hf/2, wf - (left_button_right_edge_width*btn_bg_size_nor / 2), hf);
    // Right arrow button (180-deg rotation)
    render_solid_vb_ui_flip_y_uv_shader_rot(pLwc, btn_bg_x_nor + (left_button_total_width + left_button_width_margin) * btn_bg_size_nor, btn_bg_y_nor, 2 * btn_bg_size_nor, 2 * btn_bg_size_nor,
                                            0, LVT_UI_LEFT_BUTTON_BG,
                                            1, 0, 0, 0, 0, 0, LWST_COLOR, (float)M_PI);
    // Register as a button
    lwbutton_append(pLwc, button_list, "seltower_right", btn_bg_x_nor + (left_button_width + left_button_right_edge_width/2 + left_button_width_margin) * btn_bg_size_nor, btn_bg_y_nor + hf / 2, wf - (left_button_right_edge_width*btn_bg_size_nor / 2), hf);
}

static void s_render_full_panel(const LWCONTEXT* pLwc, LWBUTTONLIST* button_list) {
    const float full_panel_bg_size_nor = 1.0f;
    const float full_panel_bg_x_nor = -pLwc->viewport_aspect_ratio + full_panel_width/2;
    const float full_panel_bg_y_nor = -0.25f/2;
    
    int panel_shader_index = LWST_COLOR; //LWST_PANEL;
    
    lazy_glUseProgram(pLwc, panel_shader_index);
    glUniform1f(pLwc->shader[panel_shader_index].time, (float)pLwc->app_time);
    glUniform2f(pLwc->shader[panel_shader_index].resolution, (float)pLwc->viewport_width, (float)pLwc->viewport_height);
    // Panel background
    render_solid_vb_ui_flip_y_uv_shader_rot(pLwc, full_panel_bg_x_nor, full_panel_bg_y_nor, 2 * full_panel_bg_size_nor, 2 * full_panel_bg_size_nor,
                                            0, LVT_UI_FULL_PANEL_BG,
                                            1, 0, 0, 0, 0, 0, panel_shader_index, 0);
    // Panel title
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_LEFT_TOP;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_B;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 0);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 0);
    char msg[128];
    sprintf(msg, "[$] Research");
    text_block.text = msg;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = -pLwc->viewport_aspect_ratio;
    text_block.text_block_y = 0.75f;
    text_block.multiline = 0;
    render_text_block(pLwc, &text_block);
    // Properties & Values
    text_block.align = LTBA_LEFT_CENTER;
    const char* prop_str[] = {
        "Projectile Type",
        "Detection Lag",
        "Pre Warm up",
        "Post Cool down",
        "Rotation Speed",
        "Durability",
        "Construction",
        "Max Range",
        "Min Range",
    };
    const float prop_tab_width = 0.3f;
    const float prop_width = 0.8f;
    const float button_scale = 0.125f;
    const float val_width = 0.75f;
    const float button_x = -pLwc->viewport_aspect_ratio + prop_tab_width + prop_width + val_width;
    const float upgrade_text_width_margin = 0.1f;
    for (int i = 0; i < 9; i++) {
        const float prop_y = (1.0f + full_panel_bg_y_nor * 2) - 0.05f - 0.175f * (i + 1);
        const float button_y = prop_y;
        text_block.text_block_x = -pLwc->viewport_aspect_ratio + prop_tab_width;
        text_block.text_block_y = prop_y;
        text_block.size = DEFAULT_TEXT_BLOCK_SIZE_D;
        char prop_msg[128];
        sprintf(prop_msg, "%s", prop_str[i]);
        text_block.text = prop_msg;
        text_block.text_bytelen = (int)strlen(text_block.text);
        text_block.begin_index = 0;
        text_block.end_index = text_block.text_bytelen;
        render_text_block(pLwc, &text_block);
        
        text_block.text_block_x = -pLwc->viewport_aspect_ratio + prop_tab_width + prop_width;
        text_block.size = DEFAULT_TEXT_BLOCK_SIZE_D;
        char val_msg[128];
        sprintf(val_msg, "%d", 100 + i + 1);
        text_block.text = val_msg;
        text_block.text_bytelen = (int)strlen(text_block.text);
        text_block.begin_index = 0;
        text_block.end_index = text_block.text_bytelen;
        render_text_block(pLwc, &text_block);
        
        // Upgrade button
        int shader_index = LWST_COLOR;
        const float wf = button_width * button_scale;
        const float hf = button_height * button_scale;
        render_solid_vb_ui_flip_y_uv_shader_rot(pLwc, button_x, button_y, 2.0f * button_scale, 2.0f * button_scale,
                                                0, LVT_UI_BUTTON_BG,
                                                1, 0, 0, 0, 0, 0, shader_index, 0);
        // Upgrade button text
        text_block.text_block_x = button_x - button_width/2 * button_scale + upgrade_text_width_margin;
        text_block.size = DEFAULT_TEXT_BLOCK_SIZE_D;
        char up_msg[128];
        sprintf(up_msg, "+%d [=] %d", i + 1, (i + 1) * 100);
        text_block.text = up_msg;
        text_block.text_bytelen = (int)strlen(text_block.text);
        text_block.begin_index = 0;
        text_block.end_index = text_block.text_bytelen;
        render_text_block(pLwc, &text_block);
        // Register as a button
        char btn_id[32];
        sprintf(btn_id, "upgrade%d", i);
        lwbutton_append(pLwc, button_list, btn_id, button_x - wf/2, button_y + hf/2, wf, hf);
    }
}

void lwc_render_ui(const LWCONTEXT* pLwc) {
    LW_GL_VIEWPORT();
    lw_clear_color();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Button count to zero (ignoring const-ness......)
    LWBUTTONLIST* button_list = &((LWCONTEXT*)pLwc)->button_list;
    s_render_scrap(pLwc, button_list);
    s_render_tower_button(pLwc, button_list);
    s_render_tower_page_button(pLwc, button_list);
    s_render_full_panel(pLwc, button_list);
}

void render_basic_field_ui(const LWCONTEXT* pLwc) {
    // Button count to zero (ignoring const-ness......)
    LWBUTTONLIST* button_list = &((LWCONTEXT*)pLwc)->button_list;
    s_render_scrap(pLwc, button_list);
    s_render_tower_button(pLwc, button_list);
    s_render_tower_page_button(pLwc, button_list);
}

