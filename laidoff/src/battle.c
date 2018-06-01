#include <string.h>
#include <stdio.h>
#include "lwcontext.h"
#include "lwmacro.h"
#include "lwbattlecommand.h"
#include "lwbattlecommandresult.h"
#include "battlelogic.h"
#include "battle.h"
#include "render_battle.h"
#include "laidoff.h"
#include "sound.h"
#include "logic.h"

int calculate_and_apply_attack_1_on_1(LWCONTEXT* pLwc, LWBATTLECREATURE* ca, const LWSKILL* s, LWBATTLECREATURE* cb,
	LWBATTLECOMMANDRESULT* cmd_result_a, LWBATTLECOMMANDRESULT* cmd_result_b);
void play_enemy_hp_desc_anim(LWCONTEXT* pLwc, LWENEMY* enemy, int enemy_slot,
	const LWBATTLECOMMANDRESULT* cmd_result_a, const LWBATTLECOMMANDRESULT* cmd_result_b);
int get_alive_enemy_count(LWCONTEXT* pLwc);
void exec_player_win(LWCONTEXT* pLwc);

int spawn_attack_trail(LWCONTEXT* pLwc, float x, float y, float z) {
	for (int i = 0; i < MAX_TRAIL; i++) {
		if (!pLwc->trail[i].valid) {
			pLwc->trail[i].x = x;
			pLwc->trail[i].y = y;
			pLwc->trail[i].z = z;
			pLwc->trail[i].age = 0;
			pLwc->trail[i].max_age = 1;
			pLwc->trail[i].tex_coord_speed = 5;
			pLwc->trail[i].tex_coord = -1;
			pLwc->trail[i].valid = 1;
			return i;
		}
	}

	return -1;
}

int update_next_player_turn_creature(LWCONTEXT* pLwc) {

	if (pLwc->player_turn_creature_index >= 0) {
		pLwc->player[pLwc->player_turn_creature_index].selected = 0;
		pLwc->player[pLwc->player_turn_creature_index].turn_consumed = 1;
		const int next_turn_token = pLwc->player[pLwc->player_turn_creature_index].turn_token + 1;

		for (int i = 0; i < MAX_PLAYER_SLOT; i++) {
			if (pLwc->player[i].turn_token == next_turn_token) {
				pLwc->player[i].selected = 1;
				pLwc->player_turn_creature_index = i;
				return pLwc->player_turn_creature_index;
			}
		}

		pLwc->player_turn_creature_index = -1;
	}

	return pLwc->player_turn_creature_index;
}

int update_next_enemy_turn_creature(LWCONTEXT* pLwc) {

	if (pLwc->enemy_turn_creature_index >= 0) {
		pLwc->enemy[pLwc->enemy_turn_creature_index].c.selected = 0;
		pLwc->enemy[pLwc->enemy_turn_creature_index].c.turn_consumed = 1;
		int next_turn_token = pLwc->enemy[pLwc->enemy_turn_creature_index].c.turn_token + 1;

		for (int i = 0; i < MAX_ENEMY_SLOT; i++) {
			if (pLwc->enemy[i].c.turn_token == next_turn_token) {

				if (pLwc->enemy[i].c.hp > 0) {
					pLwc->enemy[i].c.selected = 1;
					pLwc->enemy_turn_creature_index = i;
					return pLwc->enemy_turn_creature_index;
				}

				next_turn_token++;
			}
		}

		pLwc->enemy_turn_creature_index = -1;
	}

	return pLwc->enemy_turn_creature_index;
}

void revert_battle_cam_and_update_player_turn(LWCONTEXT* pLwc) {
	pLwc->battle_fov_deg = pLwc->battle_fov_deg_0;
	pLwc->battle_cam_center_x = 0;

	int next_player_turn_creature_index = update_next_player_turn_creature(pLwc);

	pLwc->selected_command_slot = 0;

	if (get_alive_enemy_count(pLwc) <= 0) {
		exec_player_win(pLwc);
	} else {
		if (next_player_turn_creature_index < 0) {
			// Player turn finished.
			pLwc->enemy_turn_command_wait_time = 2.0f;
			pLwc->battle_state = LBS_START_ENEMY_TURN;
		} else {
			// Turn goes to the Player's next creature.
			pLwc->battle_state = LBS_SELECT_COMMAND;
		}
	}
}

void update_attack_trail(LWCONTEXT* pLwc) {
	for (int i = 0; i < MAX_TRAIL; i++) {
		if (pLwc->trail[i].valid) {
			pLwc->trail[i].age += (float)lwcontext_delta_time(pLwc);
			pLwc->trail[i].tex_coord += (float)(pLwc->trail[i].tex_coord_speed * lwcontext_delta_time(pLwc));

			if (pLwc->trail[i].age >= pLwc->trail[i].max_age) {

				pLwc->trail[i].valid = 0;

				revert_battle_cam_and_update_player_turn(pLwc);
			}
		}
	}
}

int reset_enemy_turn(LWCONTEXT* pLwc) {
	int turn_token_counter = 0;
	ARRAY_ITERATE_VALID(LWENEMY, pLwc->enemy) {
		if (e->c.hp > 0) {
			e->c.turn_consumed = 0;
			if (turn_token_counter == 0) {
				e->c.selected = 1;
				pLwc->enemy_turn_creature_index = i;
			} else {
				e->c.selected = 0;
			}
			e->c.turn_token = ++turn_token_counter;
		}
	} ARRAY_ITERATE_VALID_END();

	return turn_token_counter;
}

int setup_enemy_turn(LWCONTEXT* pLwc) {
	const int turn_token_counter = reset_enemy_turn(pLwc);

	return turn_token_counter;
}

int pick_target_player(LWCONTEXT* pLwc) {
	ARRAY_ITERATE_VALID(LWBATTLECREATURE, pLwc->player) {
		if (e->hp > 0) {
			return i;
		}
	} ARRAY_ITERATE_VALID_END();
	return -1;
}

const LWSKILL* pick_skill(LWBATTLECREATURE* ca) {
	ARRAY_ITERATE_PTR_VALID(const LWSKILL, ca->skill) {
		return e;
	} ARRAY_ITERATE_VALID_END();

	return 0;
}

void play_player_hp_desc_anim(LWCONTEXT* pLwc, const int player_slot,
	LWBATTLECOMMANDRESULT* cmd_result_a, LWBATTLECOMMANDRESULT* cmd_result_b) {

	float left_top_x = 0;
	float left_top_y = 0;

	float area_width = 0;
	float area_height = 0;

	
	get_player_creature_ui_box(player_slot, pLwc->viewport_aspect_ratio, &left_top_x, &left_top_y, &area_width, &area_height);
	
	char damage_str[128];

	LWBATTLECREATURE* player = &pLwc->player[player_slot];

	if (cmd_result_a->type == LBCR_MISSED) {
		snprintf(damage_str, ARRAY_SIZE(damage_str), "MISSED");

		play_sound(LWS_DASH1);

	} else {
		// 데미지는 음수이기 때문에 - 붙여서 양수로 바꿔줌
		snprintf(damage_str, ARRAY_SIZE(damage_str), "%d", -cmd_result_b->delta_hp);

		player->shake_duration = 0.15f;
		player->shake_magitude = 0.03f;

		play_sound(LWS_COLLISION);

	}

	spawn_damage_text(pLwc, left_top_x + area_width / 2, left_top_y - area_height / 2, 0, damage_str, LDTC_UI);
}

int exec_attack_e2p(LWCONTEXT* pLwc) {
	if (pLwc->battle_state == LBS_ENEMY_TURN_WAIT && pLwc->enemy_turn_creature_index >= 0) {

		const int player_slot = pick_target_player(pLwc);

		LWBATTLECREATURE* ca = &pLwc->enemy[pLwc->enemy_turn_creature_index].c;
		LWBATTLECREATURE* cb = &pLwc->player[player_slot];

		const LWSKILL* s = pick_skill(ca);

		LWBATTLECOMMANDRESULT cmd_result_a = { 0, };
		LWBATTLECOMMANDRESULT cmd_result_b = { 0, };

		const int error_code = calculate_and_apply_attack_1_on_1(pLwc, ca, s, cb, &cmd_result_a, &cmd_result_b);

		if (error_code == 0) {
			pLwc->battle_state = LBS_ENEMY_COMMAND_IN_PROGRESS;
			pLwc->command_banner_anim.t = pLwc->command_banner_anim.max_t = 1;
			pLwc->command_banner_anim.max_v = 1;

			play_player_hp_desc_anim(pLwc, player_slot, &cmd_result_a, &cmd_result_b);
		}
	}

	return -1;
}

void reset_player_turn(LWCONTEXT* pLwc) {
	int turn_token_counter = 0;
	ARRAY_ITERATE_VALID(LWBATTLECREATURE, pLwc->player) {
		if (e->hp > 0) {
			e->turn_consumed = 0;
			if (turn_token_counter == 0) {
				e->selected = 1;
				pLwc->player_turn_creature_index = i;
			} else {
				e->selected = 0;
			}
			e->turn_token = ++turn_token_counter;
		}
	} ARRAY_ITERATE_VALID_END();
}

void setup_player_turn(LWCONTEXT* pLwc) {
	reset_player_turn(pLwc);
}

int get_alive_enemy_count(LWCONTEXT* pLwc) {
	int count = 0;
	ARRAY_ITERATE_VALID(const LWENEMY, pLwc->enemy) {
		if (e->c.hp > 0) {
			++count;
		}
	} ARRAY_ITERATE_VALID_END();
	return count;
}

void exec_player_win(LWCONTEXT* pLwc) {

	pLwc->battle_state = LBS_START_PLAYER_WIN;

	// Distribute exp
	ARRAY_ITERATE_VALID(LWBATTLECREATURE, pLwc->player) {
		if (e->hp > 0) {
			e->earn_exp = 10;
		}
	} ARRAY_ITERATE_VALID_END();

	// Remove enemy
	field_remove_field_object(pLwc->field, pLwc->field_event_id);
	
	change_to_battle_result(pLwc);
}

void update_enemy_turn(LWCONTEXT* pLwc) {
	if (pLwc->battle_state == LBS_START_ENEMY_TURN) {

		const int turn_token_counter = setup_enemy_turn(pLwc);

		if (turn_token_counter > 0) {
			pLwc->center_image_anim.t = pLwc->center_image_anim.max_t = 1.0f;
			pLwc->center_image_anim.max_v = 1.0f;
			pLwc->center_image = LAE_U_ENEMY_TURN_KTX;

			pLwc->battle_state = LBS_ENEMY_TURN_WAIT;
		} else {
			exec_player_win(pLwc);
		}
	}

	if (pLwc->battle_state == LBS_ENEMY_TURN_WAIT) {
		if (pLwc->enemy_turn_command_wait_time > 0) {
			pLwc->enemy_turn_command_wait_time -= (float)lwcontext_delta_time(pLwc);
		} else {
			pLwc->enemy_turn_command_wait_time = 0;

			exec_attack_e2p(pLwc);
		}
	}

	if (pLwc->battle_state == LBS_ENEMY_COMMAND_IN_PROGRESS) {
		
		// Wait for command banner anim finished.
		if (pLwc->command_banner_anim.t <= 0) {
			// Wait for the next enemy creature.
			pLwc->enemy_turn_command_wait_time = 1.0f;
			pLwc->battle_state = LBS_ENEMY_TURN_WAIT;

			const int next_enemy_turn_creature_index = update_next_enemy_turn_creature(pLwc);

			if (next_enemy_turn_creature_index < 0) {
				// Enemy turn finished.

				pLwc->center_image_anim.t = pLwc->center_image_anim.max_t = 1.0f;
				pLwc->center_image_anim.max_v = 1.0f;
				pLwc->center_image = LAE_U_PLAYER_TURN_KTX;

				
				pLwc->battle_state = LBS_SELECT_COMMAND;

				setup_player_turn(pLwc);
			}
		}
	}
}

int exec_attack_p2e(LWCONTEXT* pLwc, int enemy_slot) {
	if (pLwc->battle_state == LBS_SELECT_TARGET && pLwc->player_turn_creature_index >= 0) {

		LWBATTLECREATURE* ca = &pLwc->player[pLwc->player_turn_creature_index];
		LWBATTLECREATURE* cb = &pLwc->enemy[enemy_slot].c;
		const LWSKILL* s = ca->skill[pLwc->selected_command_slot];

		LWBATTLECOMMANDRESULT cmd_result_a = { 0, };
		LWBATTLECOMMANDRESULT cmd_result_b = { 0, };

		const int error_code = calculate_and_apply_attack_1_on_1(pLwc, ca, s, cb, &cmd_result_a, &cmd_result_b);

		if (error_code == 0) {
			pLwc->battle_state = LBS_COMMAND_IN_PROGRESS;
			pLwc->command_banner_anim.t = pLwc->command_banner_anim.max_t = 1;
			pLwc->command_banner_anim.max_v = 1;

			play_enemy_hp_desc_anim(pLwc, &pLwc->enemy[enemy_slot], enemy_slot, &cmd_result_a, &cmd_result_b);
		}
	}

	return -1;
}

void play_enemy_hp_desc_anim(LWCONTEXT* pLwc, LWENEMY* enemy, int enemy_slot,
	const LWBATTLECOMMANDRESULT* cmd_result_a, const LWBATTLECOMMANDRESULT* cmd_result_b) {
	if (enemy->c.hp <= 0) {
		enemy->death_anim.v0[4] = 1; // Phase 0 (last): alpha remove max
		enemy->death_anim.v1[0] = 1; enemy->death_anim.v1[3] = 1; // Phase 1 (middle): full red
		enemy->death_anim.v2[3] = 1; // Phase 2 (start): full black
		enemy->death_anim.anim_1d.t = enemy->death_anim.anim_1d.max_t = 0.45f;
	}

	const float enemy_x = get_battle_enemy_x_center(enemy_slot);

	char damage_str[128];

	if (cmd_result_a->type == LBCR_MISSED) {
		snprintf(damage_str, ARRAY_SIZE(damage_str), "MISSED");

		enemy->evasion_anim.t = enemy->evasion_anim.max_t = 0.25f;
		enemy->evasion_anim.max_v = 0.15f;

		play_sound(LWS_DASH1);

	} else {
		// 데미지는 음수이기 때문에 - 붙여서 양수로 바꿔줌
		snprintf(damage_str, ARRAY_SIZE(damage_str), "%d", -cmd_result_b->delta_hp);

		enemy->c.shake_duration = 0.15f;
		enemy->c.shake_magitude = 0.03f;

		play_sound(LWS_COLLISION);
	}

	// TODO: MISSED 일 때 트레일을 그리지 않으면 전투가 도중에 멈추는 문제가 있어서 무조건 그려줌
	spawn_attack_trail(pLwc, enemy_x, -0.1f, 0.5f);



	spawn_damage_text(pLwc, 0, 0, 0, damage_str, LDTC_3D);

	pLwc->battle_fov_deg = pLwc->battle_fov_mag_deg_0;

	pLwc->battle_cam_center_x = enemy_x;
}

int calculate_and_apply_attack_1_on_1(LWCONTEXT* pLwc, LWBATTLECREATURE* ca, const LWSKILL* s, LWBATTLECREATURE* cb,
	LWBATTLECOMMANDRESULT* cmd_result_a, LWBATTLECOMMANDRESULT* cmd_result_b) {
	if (s && s->valid) {
		if (s->consume_hp > ca->hp) {
			return -1;
		}

		if (s->consume_mp > ca->mp) {
			return -2;
		}

		ca->hp -= s->consume_hp;
		ca->mp -= s->consume_mp;
	} else {
		return -3;
	}

	LWBATTLECOMMAND cmd;
	cmd.skill = s;

	calculate_battle_command_result(ca, cb, &cmd, cmd_result_a, cmd_result_b);

	apply_battle_command_result(ca, cmd_result_a);
	apply_battle_command_result(cb, cmd_result_b);

	return 0;
}

void exec_attack_p2e_with_screen_point(LWCONTEXT* pLwc, float x, float y) {

	for (int i = 0; i < MAX_ENEMY_SLOT; i++) {
		if (pLwc->enemy[i].valid && pLwc->enemy[i].c.hp > 0) {
			if (pLwc->enemy[i].left_top_ui_point[0] <= x
				&& x <= pLwc->enemy[i].right_bottom_ui_point[0]
				&& y <= pLwc->enemy[i].left_top_ui_point[1]
				&& pLwc->enemy[i].right_bottom_ui_point[1] <= y) {

				pLwc->battle_state = LBS_SELECT_TARGET;
				pLwc->selected_enemy_slot = i;
				exec_attack_p2e(pLwc, i);
			}
		}
	}
}

void update_battle(LWCONTEXT* pLwc) {

	

	mat4x4_perspective(pLwc->battle_proj, (float)(LWDEG2RAD(pLwc->battle_fov_deg) / pLwc->viewport_aspect_ratio), pLwc->viewport_aspect_ratio, 0.1f, 1000.0f);

	vec3 eye = { 0, -5.0f, 0.9f };
	vec3 center = { pLwc->battle_cam_center_x, 0, 0.3f };
	vec3 up = { 0, 0, 1 };
	mat4x4_look_at(pLwc->battle_view, eye, center, up);

	pLwc->command_banner_anim.t = (float)LWMAX(0, pLwc->command_banner_anim.t - (float)lwcontext_delta_time(pLwc));

	update_enemy_turn(pLwc);

	ARRAY_ITERATE_VALID(LWENEMY, pLwc->enemy) {
		update_enemy(pLwc, i, e);
	}
	ARRAY_ITERATE_VALID_END();

	ARRAY_ITERATE_VALID(LWBATTLECREATURE, pLwc->player) {
		update_player(pLwc, i, e);
	}
	ARRAY_ITERATE_VALID_END();

	pLwc->center_image_anim.t = (float)LWMAX(0, pLwc->center_image_anim.t - (float)lwcontext_delta_time(pLwc));
}
