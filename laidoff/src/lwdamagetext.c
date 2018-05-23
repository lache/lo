#include "lwdamagetext.h"
#include "lwcontext.h"
#include "lwlog.h"
#include "render_text_block.h"
#include <string.h>

int spawn_damage_text_color(LWCONTEXT* pLwc, float x, float y, float z, const char *text, LW_DAMAGE_TEXT_COORD coord, float r, float g, float b) {
	for (int i = 0; i < MAX_DAMAGE_TEXT; i++) {
		if (!pLwc->damage_text[i].valid) {
			LWDAMAGETEXT *dt = &pLwc->damage_text[i];

			dt->x = x;
			dt->y = y;
			dt->z = z;
			dt->age = 0;
			dt->max_age = 1;

			dt->valid = 1;
			strncpy(dt->text, text, ARRAY_SIZE(dt->text));
			dt->text[ARRAY_SIZE(dt->text) - 1] = '\0';
			dt->coord = coord;

			LWTEXTBLOCK *tb = &dt->text_block;
			tb->text = dt->text;
			tb->text_bytelen = (int)strlen(tb->text);
			tb->begin_index = 0;
			tb->end_index = tb->text_bytelen;

			tb->text_block_x = x;
			tb->text_block_y = y;
			tb->text_block_width = 1;
			tb->text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT;
			tb->size = DEFAULT_TEXT_BLOCK_SIZE;

			SET_COLOR_RGBA_FLOAT(tb->color_normal_glyph, r, g, b, 1);
			SET_COLOR_RGBA_FLOAT(tb->color_normal_outline, 0.1f, 0.1f, 0.1f, 1);
			SET_COLOR_RGBA_FLOAT(tb->color_emp_glyph, 1, 1, 0, 1);
			SET_COLOR_RGBA_FLOAT(tb->color_emp_outline, 0, 0, 0, 1);

			return i;
		}
	}

	return -1;
}

int spawn_damage_text(LWCONTEXT* pLwc, float x, float y, float z, const char *text, LW_DAMAGE_TEXT_COORD coord) {
	return spawn_damage_text_color(pLwc, x, y, z, text, coord, 1.00f, 0.25f, 0.25f);
}

int spawn_exp_text(LWCONTEXT* pLwc, float x, float y, float z, const char *text, LW_DAMAGE_TEXT_COORD coord) {
	return spawn_damage_text_color(pLwc, x, y, z, text, coord, 90 / 255.0f, 173 / 255.0f, 255 / 255.0f);
}

void update_damage_text(LWCONTEXT* pLwc) {
	for (int i = 0; i < MAX_TRAIL; i++) {
		if (pLwc->damage_text[i].valid) {
			pLwc->damage_text[i].age += (float)lwcontext_delta_time(pLwc);

			//pLwc->damage_text[i].text_block.text_block_x += (float)(0.2 * cos(LWDEG2RAD(60)) * lwcontext_delta_time(pLwc));
			pLwc->damage_text[i].text_block.text_block_y += (float)(0.2 * sin(LWDEG2RAD(60)) *
				lwcontext_delta_time(pLwc));

			const float t = pLwc->damage_text[i].age;
			const float expand_time = 0.15f;
			const float retain_time = 0.5f;
			const float contract_time = 0.15f;
			const float max_size = 1.0f;
			if (t < expand_time) {
				pLwc->damage_text[i].text_block.size = max_size / expand_time * t;
			} else if (expand_time <= t && t < expand_time + retain_time) {
				pLwc->damage_text[i].text_block.size = max_size;
			} else if (t < expand_time + retain_time + contract_time) {
				pLwc->damage_text[i].text_block.size = -(max_size / contract_time) * (t -
					(expand_time +
						retain_time +
						contract_time));
			} else {
				pLwc->damage_text[i].text_block.size = 0;
			}

			if (pLwc->damage_text[i].age >= pLwc->damage_text[i].max_age) {
				pLwc->damage_text[i].valid = 0;
			}
		}
	}
}

static void render_damage_text_3d(
	const LWCONTEXT* pLwc,
	const LWDAMAGETEXT* damage_text,
	const mat4x4 view,
	const mat4x4 proj,
    float ui_alpha) {
    render_text_block_alpha(pLwc, &damage_text->text_block, ui_alpha);
}

void render_damage_text(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, const mat4x4 ui_proj, float ui_alpha) {
	ARRAY_ITERATE_VALID(const LWDAMAGETEXT, pLwc->damage_text) {
		if (e->coord == LDTC_3D) {
			render_damage_text_3d(pLwc, &pLwc->damage_text[i], view, proj, ui_alpha);
		} else if (e->coord == LDTC_UI) {
			mat4x4 identity;
			mat4x4_identity(identity);
			render_damage_text_3d(pLwc, &pLwc->damage_text[i], identity, ui_proj, ui_alpha);
		} else {
			LOGE("Unknown LWDAMAGETEXT coord value: %d", e->coord);
		}
	} ARRAY_ITERATE_VALID_END();
}
