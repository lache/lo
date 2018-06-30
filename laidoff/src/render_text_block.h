#pragma once
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWTEXTBLOCK LWTEXTBLOCK;
typedef struct _LWTEXTBLOCKQUERYRESULT LWTEXTBLOCKQUERYRESULT;

void render_text_block(const LWCONTEXT* pLwc, const LWTEXTBLOCK* text_block);
void render_text_block_two_pass(const LWCONTEXT* pLwc, LWTEXTBLOCK* text_block);
void render_text_block_two_pass_color(const LWCONTEXT* pLwc, LWTEXTBLOCK* text_block);
void render_text_block_alpha(const LWCONTEXT* pLwc, const LWTEXTBLOCK* text_block, float ui_alpha);
void render_query_only_text_block(const LWCONTEXT* pLwc, const LWTEXTBLOCK* text_block, LWTEXTBLOCKQUERYRESULT* query_result);
void toggle_font_texture_test_mode(LWCONTEXT* pLwc);
#ifdef __cplusplus
}
#endif
