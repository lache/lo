#include "render_leaderboard.h"
#include "lwcontext.h"
#include "lwlog.h"
#include "render_text_block.h"
#include <string.h>
#include <stdio.h>

static void render_item(const LWCONTEXT* pLwc,
                        int index,
                        const char* rank,
                        const char* nickname,
                        const char* score,
                        int header,
                        float x0,
                        float y0,
                        float ui_alpha,
                        float r,
                        float g,
                        float b) {
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_LEFT_BOTTOM;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_E;
    text_block.multiline = 1;
    text_block.pixel_perfect = 0;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, r, g, b, ui_alpha);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, ui_alpha);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, ui_alpha);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, ui_alpha);
    text_block.text_block_y = y0 - 0.1f * index;

    text_block.text = rank;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = x0;
    render_text_block(pLwc, &text_block);

    text_block.text = nickname;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = x0 + 0.3f;
    render_text_block(pLwc, &text_block);

    text_block.text = score;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = x0 + 1.2f;
    render_text_block(pLwc, &text_block);
}

static void render_title(const LWCONTEXT* pLwc) {
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_LEFT_TOP;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_A;
    text_block.multiline = 1;
    text_block.pixel_perfect = 0;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
    const char* title_str = "LEADERBOARD";
    text_block.text = title_str;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = -pLwc->viewport_rt_x;
    text_block.text_block_y = +pLwc->viewport_rt_y;
    render_text_block(pLwc, &text_block);
}

static void render_leaderboard_page(const LWCONTEXT* pLwc, float x_center, float y_top, float ui_alpha) {
    if (pLwc->last_leaderboard.Current_page == 0 || pLwc->last_leaderboard.Total_page == 0) {
        return;
    }
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_CENTER_TOP;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_E;
    text_block.multiline = 1;
    text_block.pixel_perfect = 0;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, ui_alpha);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, ui_alpha);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, ui_alpha);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, ui_alpha);
    char page_str[32];
    sprintf(page_str, "%d / %d", pLwc->last_leaderboard.Current_page, pLwc->last_leaderboard.Total_page);
    text_block.text = page_str;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = x_center;
    text_block.text_block_y = y_top;
    render_text_block(pLwc, &text_block);
}

static void render_leaderboard_page_button(const LWCONTEXT* pLwc, float x_center, float y_bottom, float ui_alpha) {
    const int sprite_lae = LAE_UI_BUTTON_ATLAS;
    const int sprite_lac = LAC_UI_BUTTON;
    const char* sprite_name = "leaderboard-page.png";
    const LWATLASSPRITE* sprite = atlas_sprite_name(pLwc, sprite_lac, sprite_name);
    const float sprite_aspect_ratio = (float)sprite->width / sprite->height;
    const float page_button_w = pLwc->viewport_aspect_ratio > 1 ? (pLwc->viewport_rt_x * 0.7f) : (pLwc->viewport_rt_x * 1.0f);
    const float page_button_h = page_button_w / sprite_aspect_ratio;
    lwbutton_lae_append_atlas_additive(pLwc,
                                       &(((LWCONTEXT*)pLwc)->button_list),
                                       "leaderboard_page_button",
                                       x_center - page_button_w / 2,
                                       y_bottom + page_button_h * 1.25f,
                                       page_button_w,
                                       page_button_h,
                                       sprite_lae,
                                       sprite_lac,
                                       sprite_name,
                                       ui_alpha,
                                       1.0f,
                                       1.0f,
                                       1.0f);
}

void render_leaderboard_table(const LWCONTEXT* pLwc, float x0, float y0, float ui_alpha) {
    // Render leaderboard table header
    render_item(pLwc, 0, "#", "Nickname", "Score", 1, x0, y0, ui_alpha, 1, 1, 0);
    // Render leaderboard table
    const LWPLEADERBOARD* p = &pLwc->last_leaderboard;
    int rank = p->First_item_rank;
    int tieCount = 1;
    for (int i = 0; i < p->Count; i++) {
        char rank_str[64];
        sprintf(rank_str, "%d", rank + 1);
        char score_str[64];
        sprintf(score_str, "%d", p->Score[i]);
        float r = 1, g = 1, b = 1;
        if (p->Reveal_index == i) {
            r = 0.0f, g = 1.0f, b = 0.3f;
        }
        render_item(pLwc, i + 1, rank_str, p->Nickname[i], score_str, 0, x0, y0, ui_alpha, r, g, b);
        if (i < p->Count - 1) {
            if (p->Score[i] == p->Score[i + 1]) {
                tieCount++;
            } else {
                if (rank == p->First_item_rank) {
                    rank += p->First_item_tie_count;
                } else {
                    rank += tieCount;
                }
                tieCount = 1;
            }
        }
    }
    // render page buttons
    if (pLwc->viewport_aspect_ratio > 1) {
        // render page texts (i.e. '23 / 1023')
        render_leaderboard_page(pLwc,
                                -pLwc->viewport_rt_x / 2,
                                -pLwc->viewport_rt_y + 0.3f,
                                ui_alpha);
        render_leaderboard_page_button(pLwc,
                                       -pLwc->viewport_rt_x / 2,
                                       -pLwc->viewport_rt_y,
                                       ui_alpha);
    } else {
        // render page texts (i.e. '23 / 1023')
        render_leaderboard_page(pLwc,
                                0,
                                0.1f + 0.3f,
                                ui_alpha);
        render_leaderboard_page_button(pLwc,
                                       0,
                                       0.1f,
                                       ui_alpha);
    }
}

void lwc_render_leaderboard(const LWCONTEXT* pLwc) {
    // Clear all
    LW_GL_VIEWPORT();
    glClearColor(0.2f, 0.4f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Render title
    render_title(pLwc);
    const float back_button_size = 0.35f * 1.5f;
    const float x0 = -pLwc->viewport_rt_x + back_button_size + 0.1f;
    const float y0 = +pLwc->viewport_rt_y - 0.25f;
    render_leaderboard_table(pLwc, x0, y0, 1.0f);
    lwbutton_lae_append(pLwc,
                        &(((LWCONTEXT*)pLwc)->button_list),
                        "back_button",
                        -pLwc->viewport_rt_x,
                        +pLwc->viewport_rt_y - 0.2f,
                        back_button_size,
                        back_button_size,
                        LAE_UI_BACK_BUTTON,
                        LAE_UI_BACK_BUTTON,
                        1.0f,
                        1.0f,
                        1.0f,
                        1.0f);
    // render buttons (shared)
    render_lwbutton(pLwc, &pLwc->button_list);
}
