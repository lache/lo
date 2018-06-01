#include <stdlib.h>
#include <string.h>
#include "sysmsg.h"
#include "lwmacro.h"
#include "lwcontext.h"
#include "render_solid.h"
#include "render_text_block.h"

#define LW_SYS_MSG_MAX_LEN (256)

typedef struct _LWSYSMSG {
	char msg[LW_SYS_MSG_MAX_LEN];
	float remain_time;
} LWSYSMSG;

void* init_sys_msg() {
	LWSYSMSG* sm = (LWSYSMSG*)malloc(sizeof(LWSYSMSG));
	sm->msg[0] = '\0';
	sm->remain_time = 0;
	return sm;
}

void deinit_sys_msg(void* _sm) {
	LWSYSMSG* sm = (LWSYSMSG*)_sm;
	free(sm);
}

void show_sys_msg(void* _sm, const char* msg) {
	LWSYSMSG* sm = (LWSYSMSG*)_sm;
	strncpy(sm->msg, msg, LW_SYS_MSG_MAX_LEN);
	sm->remain_time = 5.0f;
}

void update_sys_msg(void* _sm, float delta_time) {
	LWSYSMSG* sm = (LWSYSMSG*)_sm;

	sm->remain_time = LWMAX(0, sm->remain_time - delta_time);
}

static const float header_y_center = 0.9f;
static const float header_height = 0.1f;

static void s_render(const LWCONTEXT* pLwc, const char* msg) {
	render_solid_vb_ui_flip_y_uv(pLwc, 0, -header_y_center, 2 * pLwc->viewport_aspect_ratio, header_height,
		pLwc->tex_programmed[LPT_BOTH_END_GRADIENT_HORIZONTAL], LVT_CENTER_CENTER_ANCHORED_SQUARE,
		1, 39 / 255.0f, 74 / 255.0f, 110 / 255.0f, 1.0f, 0);

	LWTEXTBLOCK text_block;
	text_block.align = LTBA_CENTER_CENTER;
	text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
	text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
	text_block.size = DEFAULT_TEXT_BLOCK_SIZE_D;
	SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
	text_block.text = msg;
	text_block.text_bytelen = (int)strlen(text_block.text);
	text_block.begin_index = 0;
	text_block.end_index = text_block.text_bytelen;
	text_block.text_block_x = 0;
	text_block.text_block_y = -header_y_center;
	text_block.multiline = 1;
	render_text_block(pLwc, &text_block);
}

void render_sys_msg(const LWCONTEXT* pLwc, void* _sm) {
	LWSYSMSG* sm = (LWSYSMSG*)_sm;
	if (sm->remain_time <= 0) {
		return;
	}

	s_render(pLwc, sm->msg);
}
