#include "render_remtex.h"
#include "lwgl.h"
#include "lwcontext.h"
#include "laidoff.h"
#include "render_solid.h"
#include "render_text_block.h"
#include "lwmacro.h"
#include "remtex.h"
#include <string.h>

static void s_render_box(const LWCONTEXT* pLwc) {
    GLuint tex = remtex_load_tex(pLwc->remtex, "world-map");
    render_solid_vb_ui(pLwc,
                       0,
                       0,
                       2,
                       2,
                       tex,
                       LVT_CENTER_CENTER_ANCHORED_SQUARE,
                       1,
                       0,
                       0,
                       0,
                       0);
}

static void render_loading_text(const LWCONTEXT* pLwc, float x_center, float y_top, float ui_alpha) {
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
    char page_str[1024];
    remtex_loading_str(pLwc->remtex, page_str, ARRAY_SIZE(page_str));
    text_block.text = page_str;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = x_center;
    text_block.text_block_y = y_top;
    render_text_block(pLwc, &text_block);
}

void lwc_render_remtex(const LWCONTEXT* pLwc) {
    LW_GL_VIEWPORT();
    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    s_render_box(pLwc);
    render_loading_text(pLwc, 0, 1.0f, 1.0f);
}
