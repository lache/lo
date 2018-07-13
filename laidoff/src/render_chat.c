#include <string.h> // mac os strlen
#include "render_chat.h"
#include "lwchatringbuffer.h"
#include "lwcontext.h"
#include "render_text_block.h"

static void lwchat_fill_text_block(LWTEXTBLOCK* text_block, float size, const char* text, float x, float y, float r, float g, float b) {
    text_block->text_block_width = 999.0f;
    text_block->text_block_line_height = size;
    text_block->size = size;
    SET_COLOR_RGBA_FLOAT(text_block->color_normal_glyph, r,g,b,1);
    SET_COLOR_RGBA_FLOAT(text_block->color_normal_outline, 1,1,1,1);
    SET_COLOR_RGBA_FLOAT(text_block->color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block->color_emp_outline, 0, 0, 0, 1);
    text_block->text = text;
    text_block->text_bytelen = (int)strlen(text_block->text);
    text_block->begin_index = 0;
    text_block->end_index = text_block->text_bytelen;
    text_block->multiline = 1;
    text_block->pixel_perfect = 1;
    text_block->text_block_x = x;
    text_block->text_block_y = y;
    text_block->align = LTBA_LEFT_BOTTOM;
}

void lwc_render_chat(const LWCONTEXT* pLwc) {
    int chat_lines = lwchatringbuffer_count(&pLwc->chat_ring_buffer);
    LWTEXTBLOCK text_block;
    for (int i = 0; i < chat_lines; i++) {
        const LWCHATLINE* cl = lwchatringbuffer_get(&pLwc->chat_ring_buffer, i);
        lwchat_fill_text_block(&text_block, 1.0f, cl->line, 2, pLwc->window_height / 2.0f - 32 * i, 0, 0, 0);
        render_text_block(pLwc, &text_block);
        lwchat_fill_text_block(&text_block, 1.0f, cl->line, 2 - 2, pLwc->window_height / 2.0f - 32 * i + 2, 1, 1, 1);
        render_text_block(pLwc, &text_block);
    }
}
