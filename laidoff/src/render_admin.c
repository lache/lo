#include <math.h>
#include <string.h>
#include "render_admin.h"
#include "lwcontext.h"
#include "render_solid.h"
#include "render_text_block.h"
#include "unicode.h"

typedef struct _ADMIN_BUTTON_LAYOUT {
    int but_col_cnt;
    int but_row_cnt;
    float aspect_ratio_x;
    float aspect_ratio_y;
    float margin_x;
    float margin_y;
    float spacing_x;
    float spacing_y;
    float but_w;
    float but_h;
} ADMIN_BUTTON_LAYOUT;

void calculate_admin_button_layout(const LWCONTEXT* pLwc, ADMIN_BUTTON_LAYOUT* abl) {
    abl->but_col_cnt = 6;
    abl->but_row_cnt = 6;
    abl->aspect_ratio_x = pLwc->viewport_rt_x;
    abl->aspect_ratio_y = pLwc->viewport_rt_y;
    abl->margin_x = 0.05f * abl->aspect_ratio_x;
    abl->margin_y = 0.05f * abl->aspect_ratio_y;
    abl->spacing_x = 0.05f * abl->aspect_ratio_x;
    abl->spacing_y = 0.05f * abl->aspect_ratio_y;
    abl->but_w = (2 * abl->aspect_ratio_x - abl->margin_x * 2 - abl->spacing_x * (abl->but_col_cnt - 1)) / abl->but_col_cnt;
    abl->but_h = (2 * abl->aspect_ratio_y - abl->margin_y * 2 - abl->spacing_y * (abl->but_row_cnt - 1)) / abl->but_row_cnt;
}

void get_but_pos(const ADMIN_BUTTON_LAYOUT* abl, int r, int c, vec2 but_pos) {
    but_pos[0] = -(abl->aspect_ratio_x) + (abl->margin_x + c * (abl->but_w + abl->spacing_x) + abl->but_w / 2);
    but_pos[1] = +(abl->aspect_ratio_y) - (abl->margin_y + r * (abl->but_h + abl->spacing_y) + abl->but_h / 2);
}

void lwc_render_admin(const LWCONTEXT* pLwc) {
    LW_GL_VIEWPORT();
    glClearColor(189 / 255.f, 82 / 255.f, 232 / 255.f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    ADMIN_BUTTON_LAYOUT _abl, *abl = &_abl;
    calculate_admin_button_layout(pLwc, abl);

    int i = 0;
    for (int r = 0; r < abl->but_row_cnt; r++) {
        for (int c = 0; c < abl->but_col_cnt; c++) {

            if (i >= ARRAY_SIZE(pLwc->admin_button_command)
                || !pLwc->admin_button_command[i].name) {
                break;
            }

            vec2 but_pos;
            get_but_pos(abl, r, c, but_pos);

            render_solid_box_ui_lvt_flip_y_uv(pLwc,
                but_pos[0],
                but_pos[1],
                abl->but_w,
                abl->but_h,
                pLwc->tex_programmed[LPT_SOLID_GRAY],
                LVT_CENTER_CENTER_ANCHORED_SQUARE,
                0);

            LWTEXTBLOCK text_block;
            text_block.align = LTBA_CENTER_CENTER;
            text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
            text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
            text_block.size = DEFAULT_TEXT_BLOCK_SIZE_F;
            SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
            SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
            SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
            SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
            text_block.text = pLwc->admin_button_command[i].name;
            text_block.text_bytelen = (int)strlen(text_block.text);
            text_block.begin_index = 0;
            text_block.end_index = text_block.text_bytelen;
            text_block.text_block_x = but_pos[0];
            text_block.text_block_y = but_pos[1];
            text_block.multiline = 1;
            text_block.pixel_perfect = 0;
            render_text_block(pLwc, &text_block);

            i++;
        }
    }
}

int calculate_admin_button_index(float x, float y, const ADMIN_BUTTON_LAYOUT* abl) {
    int c = (int)floorf((x + abl->aspect_ratio_x - abl->margin_x) / (abl->but_w + abl->spacing_x));
    int r = -(int)ceilf((y - abl->aspect_ratio_y + abl->margin_y) / (abl->but_h + abl->spacing_y));

    if (c < 0 || r < 0) {
        return -1;
    }

    vec2 but_pos;
    get_but_pos(abl, r, c, but_pos);

    if (fabs(but_pos[0] - x) < abl->but_w / 2
        && fabs(but_pos[1] - y) < abl->but_h / 2) {

        return r * abl->but_col_cnt + c;
    } else {
        return -1;
    }
}

void touch_admin(LWCONTEXT* pLwc, float x0, float y0, float x1, float y1) {

    ADMIN_BUTTON_LAYOUT _abl, *abl = &_abl;
    calculate_admin_button_layout(pLwc, abl);

    int button_index_0 = calculate_admin_button_index(x0, y0, abl);

    if (0 <= button_index_0 && button_index_0 < ARRAY_SIZE(pLwc->admin_button_command)) {

        int button_index_1 = calculate_admin_button_index(x1, y1, abl);

        if (button_index_0 == button_index_1) {
            LW_BUTTON_COMMAND_HANDLER ch = pLwc->admin_button_command[button_index_0].command_handler;
            if (ch) {
                ch(pLwc);
            }
        }
    }
}
