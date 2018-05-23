#include <stdio.h>
#include "battle_result.h"
#include "lwcontext.h"
#include "battle.h"
#include "render_battle_result.h"
#include "lwuidim.h"
#include "laidoff.h"
#include "logic.h"

float clamped_interp(float a, float b, float r) {
	if (r < 0) {
		return a;
	}

	if (r > 1) {
		return b;
	}

	return (1.0f - r) * a + r * b;
}

void update_battle_result(LWCONTEXT* pLwc) {

	

	if (pLwc->battle_state == LBS_START_PLAYER_WIN) {
		
		ARRAY_ITERATE_VALID(LWBATTLECREATURE, pLwc->player) {
			if (e->hp > 0) {
				float left_top_x = 0;
				float left_top_y = 0;
				float area_width = 0;
				float area_height = 0;

				get_player_creature_result_ui_box(i, pLwc->aspect_ratio, &left_top_x, &left_top_y, &area_width, &area_height);


				// Exp string
				char exp_str[128];
				snprintf(exp_str, ARRAY_SIZE(exp_str), "%d", e->earn_exp);
				spawn_exp_text(pLwc, left_top_x + area_width / 2, left_top_y - area_height/ 2 , 0, exp_str, LDTC_UI);
			}
			
		} ARRAY_ITERATE_VALID_END();

		pLwc->battle_state = LBS_PLAYER_WIN_IN_PROGRESS;
	}

	// Player battle creature UI
	ARRAY_ITERATE_VALID(LWBATTLECREATURE, pLwc->player) {
		e->render_exp = (int)clamped_interp((float)e->exp, (float)e->exp + e->earn_exp, (float)pLwc->scene_time);
	} ARRAY_ITERATE_VALID_END();
}

void process_touch_battle_result(LWCONTEXT* pLwc, float x, float y) {
	LWUIDIM next_button_d;
	get_battle_result_next_button_dim(&next_button_d);
	if (lwuidim_contains(&next_button_d, x, y)) {
		change_to_field(pLwc);
	}
}
