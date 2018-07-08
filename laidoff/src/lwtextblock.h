#pragma once
#include "lwuialign.h"

typedef struct _LWCONTEXT LWCONTEXT;

typedef struct _LWTEXTBLOCK {
	const char* text;
	int text_bytelen;
	int begin_index;
	int end_index;
	float text_block_x;
	float text_block_y;
	float text_block_width;
	float text_block_line_height;
	float size;
	float color_normal_glyph[4];
	float color_normal_outline[4];
	float color_emp_glyph[4];
	float color_emp_outline[4];
	LW_UI_ALIGN align;
	int multiline;
    int pixel_perfect;
} LWTEXTBLOCK;

typedef struct _LWTEXTBLOCKQUERYRESULT {
	float total_glyph_width;
	float glyph_height;
} LWTEXTBLOCKQUERYRESULT;

#define DEFAULT_TEXT_BLOCK_WIDTH (4.0f)

#define DEFAULT_TEXT_BLOCK_LINE_HEIGHT (0.225f)
#define DEFAULT_TEXT_BLOCK_LINE_HEIGHT_A (DEFAULT_TEXT_BLOCK_LINE_HEIGHT*0.9f)
#define DEFAULT_TEXT_BLOCK_LINE_HEIGHT_B (DEFAULT_TEXT_BLOCK_LINE_HEIGHT*0.8f)
#define DEFAULT_TEXT_BLOCK_LINE_HEIGHT_C (DEFAULT_TEXT_BLOCK_LINE_HEIGHT*0.7f)
#define DEFAULT_TEXT_BLOCK_LINE_HEIGHT_D (DEFAULT_TEXT_BLOCK_LINE_HEIGHT*0.6f)
#define DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E (DEFAULT_TEXT_BLOCK_LINE_HEIGHT*0.5f)
#define DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F (DEFAULT_TEXT_BLOCK_LINE_HEIGHT*0.4f)

#define DEFAULT_TEXT_BLOCK_SIZE (1.0f)
#define DEFAULT_TEXT_BLOCK_SIZE_A (DEFAULT_TEXT_BLOCK_SIZE*0.90f)
#define DEFAULT_TEXT_BLOCK_SIZE_B (DEFAULT_TEXT_BLOCK_SIZE*0.80f)
#define DEFAULT_TEXT_BLOCK_SIZE_C (DEFAULT_TEXT_BLOCK_SIZE*0.70f)
#define DEFAULT_TEXT_BLOCK_SIZE_D (DEFAULT_TEXT_BLOCK_SIZE*0.60f)
#define DEFAULT_TEXT_BLOCK_SIZE_E (DEFAULT_TEXT_BLOCK_SIZE*0.55f)
#define DEFAULT_TEXT_BLOCK_SIZE_F (DEFAULT_TEXT_BLOCK_SIZE*0.40f)

#ifdef __cplusplus
extern "C" {;
#endif
void lwtextblock_query_only_text_block(const LWCONTEXT* pLwc, const LWTEXTBLOCK* text_block, LWTEXTBLOCKQUERYRESULT* query_result);
#ifdef __cplusplus
}
#endif
