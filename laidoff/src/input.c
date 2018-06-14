#include "lwtcp.h"
#include "lwlog.h"
#include "lwcontext.h"
#include "battle.h"
#include "laidoff.h"
#include "render_admin.h"
#include "battle_result.h"
#include "field.h"
#include "mq.h"
#include "logic.h"
#include "lwbutton.h"
#include "puckgameupdate.h"
#include "puckgame.h"
#include "lwdirpad.h"
#include "lwtcpclient.h"
#include "rmsg.h"
#include "file.h"
#include "htmlui.h"
#include "lwttl.h"
#include "input.h"

static void convert_touch_coord_to_ui_coord(LWCONTEXT* pLwc, float *x, float *y) {
	if (pLwc->viewport_aspect_ratio > 1) {
		*x *= pLwc->viewport_rt_x;
	} else {
		*y *= pLwc->viewport_rt_y;
	}
}

typedef struct _LWPINCHPOINTER {
    int id;
    float x0;
    float y0;
} LWPINCHPOINTER;
static struct {
    int count;
    LWPINCHPOINTER p[2];
	float initial_dist;
    int initial_view_scale;
} pinch_zoom;

float calculate_pinch_zoom_dist() {
	const float dx = pinch_zoom.p[0].x0 - pinch_zoom.p[1].x0;
	const float dy = pinch_zoom.p[0].y0 - pinch_zoom.p[1].y0;
	const float d = sqrtf(dx*dx + dy*dy);
    return LWCLAMP(d, 0.5f, 10.0f);
}

static int admin_pressed = 0;
static const float admin_button_size = 0.15f;

void lw_trigger_mouse_press(LWCONTEXT* pLwc, float nx, float ny, int pointer_id) {
	if (!pLwc) {
		return;
	}

    float x = nx;
    float y = ny;
	convert_touch_coord_to_ui_coord(pLwc, &x, &y);

	LOGIx("mouse press ui coord x=%f, y=%f", x, y);

    // Touch right top corner of the screen
    if (pLwc->game_scene != LGS_ADMIN
        && x > pLwc->viewport_rt_x - admin_button_size
        && y > pLwc->viewport_rt_y - admin_button_size
        && is_file_exist(pLwc->user_data_path, "admin")) {
        admin_pressed = 1;
    } else {
        admin_pressed = 0;
    }
    
    if (pLwc->htmlui) {
        if (pLwc->game_scene == LGS_TTL
            || (pLwc->game_scene == LGS_PUCK_GAME && pLwc->puck_game->game_state == LPGS_MAIN_MENU && pLwc->puck_game->show_html_ui && pLwc->puck_game->world_roll_dirty == 0)) {
            const float nx = (x + pLwc->viewport_rt_x) / (2.0f * pLwc->viewport_rt_x);
            const float ny = (pLwc->viewport_rt_y - y) / (2.0f * pLwc->viewport_rt_y);
            htmlui_on_lbutton_down(pLwc->htmlui, nx, ny);
            if (htmlui_over_element(pLwc->htmlui, nx, ny)) {
                return;
            }
        }
    }
    int prev_pinch_zoom_count = pinch_zoom.count;
    if (pinch_zoom.count == 0 || pinch_zoom.count == 1) {
        pinch_zoom.p[pinch_zoom.count].id = pointer_id;
        pinch_zoom.p[pinch_zoom.count].x0 = x;
        pinch_zoom.p[pinch_zoom.count].y0 = y;
        pinch_zoom.count++;
    }

    if (pinch_zoom.count == 2 && prev_pinch_zoom_count != 2) {
		pinch_zoom.initial_dist = calculate_pinch_zoom_dist();
        pinch_zoom.initial_view_scale = lwttl_view_scale(pLwc->ttl);
        LOGIx("Pinch zoom started. initial dist = %.2f, initial view scale = %d",
             pinch_zoom.initial_dist,
             pinch_zoom.initial_view_scale);
	}

    if (lw_pinch() == 0) {
        lwttl_on_press(pLwc->ttl, pLwc, nx, ny);
    }

	if (field_network(pLwc->field)) {
		mq_publish_now(pLwc, pLwc->mq, 0);
	}

	pLwc->last_mouse_press_x = x;
	pLwc->last_mouse_press_y = y;

	float w_ratio, h_ratio;
	int pressed_idx = lwbutton_press(pLwc, &pLwc->button_list, x, y, &w_ratio, &h_ratio);
	if (pressed_idx >= 0) {
		const char* id = lwbutton_id(&pLwc->button_list, pressed_idx);
		logic_emit_ui_event_async(pLwc, id, w_ratio, h_ratio);
		// Should return here to prevent calling overlapped UI element behind buttons.
		return;
	}

	if (pLwc->game_scene == LGS_FIELD || pLwc->game_scene == LGS_PUCK_GAME) {
		const float sr = get_dir_pad_size_radius();

		float left_dir_pad_center_x = 0;
		float left_dir_pad_center_y = 0;
		get_left_dir_pad_original_center(pLwc->viewport_aspect_ratio, &left_dir_pad_center_x, &left_dir_pad_center_y);
		dir_pad_press(&pLwc->left_dir_pad, x, y, pointer_id, left_dir_pad_center_x, left_dir_pad_center_y, sr);

		if (pLwc->control_flags & LCF_PUCK_GAME_RIGHT_DIR_PAD) {
			float right_dir_pad_center_x = 0;
			float right_dir_pad_center_y = 0;
			get_right_dir_pad_original_center(pLwc->viewport_aspect_ratio, &right_dir_pad_center_x, &right_dir_pad_center_y);
			dir_pad_press(&pLwc->right_dir_pad, x, y, pointer_id, right_dir_pad_center_x, right_dir_pad_center_y, sr);
		}
    } else if (pLwc->game_scene == LGS_TTL) {
        const float sr = 2.0f;

        float left_dir_pad_center_x = 0;
        float left_dir_pad_center_y = 0;
        dir_pad_press(&pLwc->left_dir_pad, x, y, pointer_id, left_dir_pad_center_x, left_dir_pad_center_y, sr);
    }

	const float fist_button_w = 0.75f;
	const float fist_button_h = 0.75f;
	const float fist_button_x_center = +pLwc->viewport_rt_x - 0.3f - fist_button_w / 2;
	const float fist_button_y_center = -pLwc->viewport_rt_y + fist_button_h / 2;

	const float top_button_w = fist_button_w;
	const float top_button_h = fist_button_h;
	const float top_button_x_center = fist_button_x_center;
	const float top_button_y_center = -fist_button_y_center;


	if (pLwc->game_scene == LGS_PUCK_GAME
		&& fabs(fist_button_x_center - x) < fist_button_w
		&& fabs(fist_button_y_center - y) < fist_button_h
		&& (!pLwc->left_dir_pad.dragging || (pLwc->left_dir_pad.pointer_id != pointer_id))) {
		// this event handled by lua script
		//puck_game_dash(pLwc, pLwc->puck_game);
	}

	if (pLwc->game_scene == LGS_FIELD
		&& fabs(fist_button_x_center - x) < fist_button_w
		&& fabs(fist_button_y_center - y) < fist_button_h) {
		//field_attack(pLwc);

		//pLwc->hide_field = !pLwc->hide_field;

		//LOGI("atk_pad_dragging ON");

		// Player combat mode...
		//pLwc->atk_pad_dragging = 1;
	}

	if (pLwc->game_scene == LGS_FIELD
		&& fabs(top_button_x_center - x) < top_button_w
		&& fabs(top_button_y_center - y) < top_button_h) {
		//pLwc->fps_mode = !pLwc->fps_mode;
	}

	if (pLwc->game_scene == LGS_PUCK_GAME
		&& fabs(top_button_x_center - x) < top_button_w
		&& fabs(top_button_y_center - y) < top_button_h) {
		// controlled by LWBUTTONLIST and lua script
		//puck_game_pull_puck_start(pLwc, pLwc->puck_game);
	}

	if (pLwc->game_scene == LGS_BATTLE && pLwc->battle_state != LBS_COMMAND_IN_PROGRESS && pLwc->player_turn_creature_index >= 0) {
		const float command_palette_pos = -0.5f;

		if (y > command_palette_pos) {
			exec_attack_p2e_with_screen_point(pLwc, x, y);
		} else {
			// command palette area
			int command_slot = (int)((x + pLwc->viewport_rt_x) / (2.0f / 10 * pLwc->viewport_rt_x)) - 2;
			if (command_slot >= 0 && command_slot < 6) {
				const LWSKILL* skill = pLwc->player[pLwc->player_turn_creature_index].skill[command_slot];
				if (skill && skill->valid) {
					pLwc->selected_command_slot = command_slot;
				}
			}

			//printf("mouse press command slot %d\n", command_slot);
		}
	}
}

void lw_trigger_mouse_move(LWCONTEXT* pLwc, float nx, float ny, int pointer_id) {
	if (!pLwc) {
		return;
	}

    float x = nx;
    float y = ny;
	convert_touch_coord_to_ui_coord(pLwc, &x, &y);

	if (pinch_zoom.count == 2) {
		LWPINCHPOINTER* p = pinch_zoom.p[0].id == pointer_id ? &pinch_zoom.p[0] : pinch_zoom.p[1].id == pointer_id ? &pinch_zoom.p[1] : 0;
		if (p) {
			p->x0 = x;
			p->y0 = y;
			const float dist = calculate_pinch_zoom_dist();
            const float zoom_factor = dist / pinch_zoom.initial_dist;
            LOGIx("Pinch zoom factor: %.2f", zoom_factor);
            const int view_scale_max = lwttl_view_scale_max(pLwc->ttl);
            if (zoom_factor > 1.0f) {
                // zoom in
                const int zoom_factor_int = (int)(1.5f * zoom_factor + 0.5f);
                lwttl_set_view_scale(pLwc->ttl, LWCLAMP(pinch_zoom.initial_view_scale >> (zoom_factor_int - 1), 1, view_scale_max));
                lwttl_udp_send_ttlping(pLwc->ttl, lwttl_sea_udp(pLwc->ttl), 0);
            } else if (0.0f < zoom_factor && zoom_factor < 1.0f) {
                // zoom out
                const int zoom_factor_int = (int)(1.5f * 1.0f / zoom_factor + 0.5f);
                lwttl_set_view_scale(pLwc->ttl, LWCLAMP(pinch_zoom.initial_view_scale << (zoom_factor_int - 1), 1, view_scale_max));
                lwttl_udp_send_ttlping(pLwc->ttl, lwttl_sea_udp(pLwc->ttl), 0);
            }
		}
	}

    if (pLwc->game_scene == LGS_TTL) {
        // TOO SLOW ON MANY HTML TAGS

	/*htmlui_on_over(pLwc->htmlui,
		(x + pLwc->viewport_aspect_ratio) / (2.0f * pLwc->viewport_aspect_ratio),
						 (1.0f - y) / 2.0f);*/
    }

    if (lw_pinch() == 0) {
        lwttl_on_move(pLwc->ttl, pLwc, nx, ny);
    }
    
	pLwc->last_mouse_move_delta_x = x - pLwc->last_mouse_move_x;
	pLwc->last_mouse_move_delta_y = y - pLwc->last_mouse_move_y;

	pLwc->last_mouse_move_x = x;
	pLwc->last_mouse_move_y = y;

	if (pLwc->game_scene == LGS_FIELD || pLwc->game_scene == LGS_PUCK_GAME) {
		const float sr = get_dir_pad_size_radius();

		float left_dir_pad_center_x = 0;
		float left_dir_pad_center_y = 0;
		get_left_dir_pad_original_center(pLwc->viewport_aspect_ratio, &left_dir_pad_center_x, &left_dir_pad_center_y);
		dir_pad_move(&pLwc->left_dir_pad, x, y, pointer_id, left_dir_pad_center_x, left_dir_pad_center_y, sr);

		if (pLwc->control_flags & LCF_PUCK_GAME_RIGHT_DIR_PAD) {
			float right_dir_pad_center_x = 0;
			float right_dir_pad_center_y = 0;
			get_right_dir_pad_original_center(pLwc->viewport_aspect_ratio, &right_dir_pad_center_x, &right_dir_pad_center_y);
			dir_pad_move(&pLwc->right_dir_pad, x, y, pointer_id, right_dir_pad_center_x, right_dir_pad_center_y, sr);
		}
	} else if (pLwc->game_scene == LGS_TTL) {
        const float sr = 2.0f;

        float left_dir_pad_center_x = 0;
        float left_dir_pad_center_y = 0;
        dir_pad_move(&pLwc->left_dir_pad, x, y, pointer_id, left_dir_pad_center_x, left_dir_pad_center_y, sr);
    }
}

void lw_trigger_mouse_release(LWCONTEXT* pLwc, float nx, float ny, int pointer_id) {
	if (!pLwc) {
		return;
	}

    float x = nx;
    float y = ny;
	convert_touch_coord_to_ui_coord(pLwc, &x, &y);

    LOGIx("mouse release ui coord x=%f, y=%f (last press ui coord x=%f, y=%f) (width %f) (height %f)\n",
		  x, y,
		  pLwc->last_mouse_press_x, pLwc->last_mouse_press_y,
		  fabsf(x - pLwc->last_mouse_press_x),
		  fabsf(y - pLwc->last_mouse_press_y));

    if (pLwc->htmlui) {
        if (pLwc->game_scene == LGS_TTL
            || (pLwc->game_scene == LGS_PUCK_GAME && pLwc->puck_game->show_html_ui)) {
            const float nx = (x + pLwc->viewport_rt_x) / (2.0f * pLwc->viewport_rt_x);
            const float ny = (pLwc->viewport_rt_y - y) / (2.0f * pLwc->viewport_rt_y);
            htmlui_on_lbutton_up(pLwc->htmlui, nx, ny);
        }
    }

    int prev_pinch_zoom_count = pinch_zoom.count;
    for (int i = pinch_zoom.count - 1; i >= 0; i--) {
        if (pinch_zoom.p[i].id == pointer_id) {
            for (int j = i; j < pinch_zoom.count - 1; j++) {
                pinch_zoom.p[j] = pinch_zoom.p[j + 1];
            }
            pinch_zoom.count--;
        }
    }

    if (prev_pinch_zoom_count == 2 && pinch_zoom.count != 2) {
        LOGIx("Pinch zoom aborted.");
    }

    if (lw_pinch() == 0) {
        lwttl_on_release(pLwc->ttl, pLwc, nx, ny);
    }

    if (field_network(pLwc->field)) {
		mq_publish_now(pLwc, pLwc->mq, 1);
	}

	const float fist_button_x_center = pLwc->viewport_aspect_ratio - 0.3f - 0.75f / 2;
	const float fist_button_y_center = -1 + 0.75f / 2;

	const float top_button_x_center = fist_button_x_center;
	const float top_button_y_center = -fist_button_y_center;
	const float top_button_w = 0.75f;
	const float top_button_h = 0.75f;

	// Touch right top corner of the screen
    if (pLwc->game_scene != LGS_ADMIN
        && x > +pLwc->viewport_rt_x - admin_button_size
		&& y > +pLwc->viewport_rt_y - admin_button_size) {
		if (admin_pressed && is_file_exist(pLwc->user_data_path, "admin")) {
			change_to_admin(pLwc);
		} else {
			static int admin_count = 0;
			admin_count++;
			if (admin_count > 10) {
				touch_file(pLwc->user_data_path, "admin");
			}
		}
	}
    admin_pressed = 0;

	if (pLwc->game_scene == LGS_PUCK_GAME
		&& fabs(top_button_x_center - x) < top_button_w
		&& fabs(top_button_y_center - y) < top_button_h) {
		// controlled by LWBUTTONLIST and lua script
		//puck_game_pull_puck_stop(pLwc, pLwc->puck_game);
	}

	dir_pad_release(&pLwc->left_dir_pad, pointer_id);
	float puck_fire_dx, puck_fire_dy, puck_fire_dlen;
	lw_get_normalized_dir_pad_input(pLwc, &pLwc->right_dir_pad, &puck_fire_dx, &puck_fire_dy, &puck_fire_dlen);
	if (pLwc->puck_game->player_no != 2) {
		puck_fire_dx *= -1;
		puck_fire_dy *= -1;
	}
	if (dir_pad_release(&pLwc->right_dir_pad, pointer_id)) {
		puck_game_fire(pLwc, pLwc->puck_game, puck_fire_dx, puck_fire_dy, puck_fire_dlen);
	}

	if (pLwc->game_scene == LGS_ADMIN) {
		touch_admin(pLwc, pLwc->last_mouse_press_x, pLwc->last_mouse_press_y, x, y);
	}
}

void lw_trigger_touch(LWCONTEXT* pLwc, float nx, float ny, int pointer_id) {
	if (!pLwc) {
		return;
	}

    float x = nx;
    float y = ny;
	convert_touch_coord_to_ui_coord(pLwc, &x, &y);

	pLwc->dialog_move_next = 1;

	if (pLwc->game_scene == LGS_FIELD) {

	} else if (pLwc->game_scene == LGS_DIALOG) {

	} else if (pLwc->game_scene == LGS_BATTLE) {

	} else if (pLwc->game_scene == LGS_ADMIN) {

	} else if (pLwc->game_scene == LGS_BATTLE_RESULT) {
		process_touch_battle_result(pLwc, x, y);
	}
}

void lw_trigger_reset(LWCONTEXT* pLwc) {
	reset_runtime_context_async(pLwc);
}

void lw_trigger_play_sound(LWCONTEXT* pLwc) {
}

void lw_trigger_key_right(LWCONTEXT* pLwc) {

	// battle

	if (pLwc->battle_state == LBS_SELECT_COMMAND) {
		if (pLwc->selected_command_slot != -1 && pLwc->player_turn_creature_index >= 0) {
			int new_selected_command_slot = -1;
			for (int i = pLwc->selected_command_slot + 1; i < MAX_COMMAND_SLOT; i++) {
				const LWSKILL* s = pLwc->player[pLwc->player_turn_creature_index].skill[i];
				if (s && s->valid) {

					new_selected_command_slot = i;
					break;
				}
			}

			if (new_selected_command_slot != -1) {
				pLwc->selected_command_slot = new_selected_command_slot;
			}
		}
	} else if (pLwc->battle_state == LBS_SELECT_TARGET) {

		if (pLwc->selected_enemy_slot != -1) {
			int new_selected_enemy_slot = -1;
			for (int i = pLwc->selected_enemy_slot + 1; i < MAX_ENEMY_SLOT; i++) {
				if (pLwc->enemy[i].valid
					&& pLwc->enemy[i].c.hp > 0) {

					new_selected_enemy_slot = i;
					break;
				}
			}

			if (new_selected_enemy_slot != -1) {
				pLwc->selected_enemy_slot = new_selected_enemy_slot;
			}
		}
	}
}

void lw_trigger_key_left(LWCONTEXT* pLwc) {

	// battle

	if (pLwc->battle_state == LBS_SELECT_COMMAND) {
		if (pLwc->selected_command_slot != -1 && pLwc->player_turn_creature_index >= 0) {
			int new_selected_command_slot = -1;
			for (int i = pLwc->selected_command_slot - 1; i >= 0; i--) {
				const LWSKILL* s = pLwc->player[pLwc->player_turn_creature_index].skill[i];
				if (s && s->valid) {

					new_selected_command_slot = i;
					break;
				}
			}

			if (new_selected_command_slot != -1) {
				pLwc->selected_command_slot = new_selected_command_slot;
			}
		}
	} else if (pLwc->battle_state == LBS_SELECT_TARGET) {
		if (pLwc->selected_enemy_slot != -1) {
			int new_selected_enemy_slot = -1;
			for (int i = pLwc->selected_enemy_slot - 1; i >= 0; i--) {
				if (pLwc->enemy[i].valid
					&& pLwc->enemy[i].c.hp > 0) {

					new_selected_enemy_slot = i;
					break;
				}
			}

			if (new_selected_enemy_slot != -1) {
				pLwc->selected_enemy_slot = new_selected_enemy_slot;
			}
		}
	}
}

void lw_trigger_key_enter(LWCONTEXT* pLwc) {

	// battle

	if (pLwc->battle_state == LBS_SELECT_COMMAND && pLwc->player_turn_creature_index >= 0) {
		pLwc->battle_state = LBS_SELECT_TARGET;

		for (int i = 0; i < ARRAY_SIZE(pLwc->enemy); i++) {
			if (pLwc->enemy[i].valid && pLwc->enemy[i].c.hp > 0) {
				pLwc->selected_enemy_slot = i;
				break;
			}
		}
	} else if (pLwc->battle_state == LBS_SELECT_TARGET) {

		exec_attack_p2e(pLwc, pLwc->selected_enemy_slot);
	}
}

static void simulate_dir_pad_touch_input(LWCONTEXT* pLwc) {
	const int simulate_pointer_id = 10;
	pLwc->left_dir_pad.pointer_id = simulate_pointer_id;
	pLwc->left_dir_pad.dragging = pLwc->player_move_left || pLwc->player_move_right || pLwc->player_move_down || pLwc->player_move_up;

	float dir_pad_center_x = 0;
	float dir_pad_center_y = 0;
	get_left_dir_pad_original_center(pLwc->viewport_aspect_ratio, &dir_pad_center_x, &dir_pad_center_y);

	pLwc->left_dir_pad.x = dir_pad_center_x + (pLwc->player_move_right - pLwc->player_move_left) / 5.0f;
	pLwc->left_dir_pad.y = dir_pad_center_y + (pLwc->player_move_up - pLwc->player_move_down) / 5.0f;
	pLwc->left_dir_pad.start_x = dir_pad_center_x;
	pLwc->left_dir_pad.start_y = dir_pad_center_y;
	pLwc->left_dir_pad.touch_began_x = dir_pad_center_x;
	pLwc->left_dir_pad.touch_began_y = dir_pad_center_y;
}

void lw_press_key_left(LWCONTEXT* pLwc) {
	pLwc->player_move_left = 1;
	simulate_dir_pad_touch_input(pLwc);
}

void lw_press_key_right(LWCONTEXT* pLwc) {
	pLwc->player_move_right = 1;
	simulate_dir_pad_touch_input(pLwc);
}

void lw_press_key_up(LWCONTEXT* pLwc) {
	pLwc->player_move_up = 1;
	simulate_dir_pad_touch_input(pLwc);
}

void lw_press_key_down(LWCONTEXT* pLwc) {
	pLwc->player_move_down = 1;
	simulate_dir_pad_touch_input(pLwc);
}

void lw_press_key_space(LWCONTEXT* pLwc) {
	pLwc->player_space = 1;
}

void lw_press_key_a(LWCONTEXT* pLwc) {
	puck_game_jump(pLwc, pLwc->puck_game);
}

void lw_press_key_z(LWCONTEXT* pLwc) {
	puck_game_dash_and_send(pLwc, pLwc->puck_game);
}

void lw_press_key_x(LWCONTEXT* pLwc) {
	puck_game_pull_puck_start(pLwc, pLwc->puck_game);
}

void lw_press_key_w(LWCONTEXT* pLwc) {
	if (pLwc->puck_game) {
		pLwc->puck_game->update_tick = (int)(pLwc->update_frequency * (pLwc->puck_game->total_time - 3.0f));
	}
}

void lw_press_key_q(LWCONTEXT* pLwc) {
	if (pLwc->puck_game) {
		if (pLwc->tcp) {
			tcp_send_suddendeath(pLwc->tcp, pLwc->puck_game->battle_id, pLwc->puck_game->token);
		}
		pLwc->puck_game->pg_player[0].current_hp = 1;
		pLwc->puck_game->pg_target[0].current_hp = 1;
		for (int i = 0; i < LW_PUCK_GAME_TOWER_COUNT; i++) {
			pLwc->puck_game->tower[i].collapsing = 0;
		}
	}
}

void lw_release_key_left(LWCONTEXT* pLwc) {
	pLwc->player_move_left = 0;
	simulate_dir_pad_touch_input(pLwc);
}

void lw_release_key_right(LWCONTEXT* pLwc) {
	pLwc->player_move_right = 0;
	simulate_dir_pad_touch_input(pLwc);
}

void lw_release_key_up(LWCONTEXT* pLwc) {
	pLwc->player_move_up = 0;
	simulate_dir_pad_touch_input(pLwc);
}

void lw_release_key_down(LWCONTEXT* pLwc) {
	pLwc->player_move_down = 0;
	simulate_dir_pad_touch_input(pLwc);
}

void lw_release_key_space(LWCONTEXT* pLwc) {
	pLwc->player_space = 0;
}

void lw_release_key_a(LWCONTEXT* pLwc) {
}

void lw_release_key_z(LWCONTEXT* pLwc) {
}

void lw_release_key_x(LWCONTEXT* pLwc) {
	puck_game_pull_puck_stop(pLwc, pLwc->puck_game);
}

void lw_release_key_q(LWCONTEXT* pLwc) {
}

void lw_go_back(LWCONTEXT* pLwc, void* native_context) {
	if (pLwc->puck_game) {
		if (pLwc->puck_game->game_state == LPGS_PRACTICE) {
			puck_game_roll_to_main_menu(pLwc->puck_game);
		} else if (pLwc->puck_game->game_state == LPGS_TUTORIAL) {
			if (puck_game_is_tutorial_stoppable(pLwc->puck_game)) {
				puck_game_roll_to_main_menu(pLwc->puck_game);
			}
		} else if (pLwc->puck_game->game_state == LPGS_MAIN_MENU) {
			// if lw_go_back called in logic thread, call to 'lw_app_quit()' will block app forever.
			// not good.
			//lw_app_quit(pLwc, native_context);
			rmsg_quitapp(pLwc, native_context);
		} else if (pLwc->puck_game->game_state == LPGS_BATTLE && pLwc->puck_game->battle_control_ui_alpha == 0) {
			// retrieve updated score and rank for main menu top bar
			tcp_send_querynick(pLwc->tcp, &pLwc->tcp->user_id);
			puck_game_roll_to_main_menu(pLwc->puck_game);
		}
	} else {
		rmsg_quitapp(pLwc, native_context);
	}
}

void lw_trigger_scroll(LWCONTEXT* pLwc, float xoffset, float yoffset) {
    if (!pLwc) {
        return;
    }
    lwttl_scroll_view_scale(pLwc->ttl, yoffset);
}

int lw_pinch() {
    return pinch_zoom.count == 2;
}
