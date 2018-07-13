#include "laidoff.h"
#include "sound.h"
#include "field.h"
#include "lwlog.h"
#include "render_text_block.h"
#include "nav.h"
#include "puckgame.h"
#include "key.h"
#include "input.h"
#include "script.h"
#include "logic.h"
#include "puckgameupdate.h"
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
#include "lwimgui.h"
#endif

void toggle_font_texture_test_mode(LWCONTEXT* pLwc);

static void handle_move_key_press_release(LWCONTEXT* pLwc, int key, int action) {
	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
		lw_press_key_right(pLwc);
	}

	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
		lw_press_key_left(pLwc);
	}

	if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		lw_press_key_up(pLwc);
	}

	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		lw_press_key_down(pLwc);
	}

	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		lw_press_key_space(pLwc);
	}

    if (key == GLFW_KEY_A && action == GLFW_PRESS) {
        lw_press_key_a(pLwc);
    }

	if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
		lw_press_key_z(pLwc);
	}

	if (key == GLFW_KEY_X && action == GLFW_PRESS) {
		lw_press_key_x(pLwc);
	}

	if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
		lw_press_key_q(pLwc);
	}
    
    if (key == GLFW_KEY_W && action == GLFW_PRESS) {
        lw_press_key_w(pLwc);
    }

	if (key == GLFW_KEY_RIGHT && action == GLFW_RELEASE) {
		lw_release_key_right(pLwc);
	}

	if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE) {
		lw_release_key_left(pLwc);
	}

	if (key == GLFW_KEY_UP && action == GLFW_RELEASE) {
		lw_release_key_up(pLwc);
	}

	if (key == GLFW_KEY_DOWN && action == GLFW_RELEASE) {
		lw_release_key_down(pLwc);
	}

	if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
		lw_release_key_space(pLwc);
	}

    if (key == GLFW_KEY_A && action == GLFW_RELEASE) {
        lw_release_key_a(pLwc);
    }

	if (key == GLFW_KEY_Z && action == GLFW_RELEASE) {
		lw_release_key_z(pLwc);
	}

	if (key == GLFW_KEY_X && action == GLFW_RELEASE) {
		lw_release_key_x(pLwc);
	}

	if (key == GLFW_KEY_Q && action == GLFW_RELEASE) {
		lw_release_key_q(pLwc);
	}

}

static void run_additional_client(const char* name, int idx) {
#if LW_PLATFORM_OSX
    char run_this[512];
    sprintf(run_this, "%s %d &", name, idx);
    system(run_this);
#elif LW_PLATFORM_WIN32
    char run_this[512];
    sprintf(run_this, "start %s %d", name, idx);
    system(run_this);
#else
    LOGEP("Not supported");
#endif
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
	lwimgui_key_callback(window, key, scancode, action, mods);
	if (lwimgui_want_capture_keyboard() || lwimgui_want_text_input()) {
		return;
	}
#endif
	LWCONTEXT* pLwc = (LWCONTEXT*)glfwGetWindowUserPointer(window);

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        //lw_go_back(pLwc, 0);
        // thread-safety.. do not use 'script_emit_ui_event' directly
        //script_emit_ui_event(pLwc->L, "back_button");
        logic_emit_ui_event_async(pLwc, "back_button", 0, 0);
		return;
	}

	if (key == GLFW_KEY_T && action == GLFW_PRESS) {
		lw_trigger_touch(pLwc, 0, 0, 0);
	}

	if (key == GLFW_KEY_S && action == GLFW_PRESS && pLwc->game_scene == LGS_FIELD) {
		float player_x = 0, player_y = 0, player_z = 0;
		get_field_player_position(pLwc->field, &player_x, &player_y, &player_z);
		nav_set_path_query_spos(field_nav(pLwc->field), player_x, player_y, player_z);
		float p[3];
		nav_path_query_spos(field_nav(pLwc->field), p);
		LOGI("Nav: start pos set to (%.2f, %.2f, %.2f) [nav coordinates]",
			p[0],
			p[1],
			p[2]);
	}

	if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		float player_x = 0, player_y = 0, player_z = 0;
		get_field_player_position(pLwc->field, &player_x, &player_y, &player_z);
		nav_set_path_query_epos(field_nav(pLwc->field), player_x, player_y, player_z);
		float p[3];
		nav_path_query_epos(field_nav(pLwc->field), p);
		LOGI("Nav: end pos set to (%.2f, %.2f, %.2f) [nav coordinates]",
			p[0],
			p[1],
			p[2]);
	}

	if (key == GLFW_KEY_F && action == GLFW_PRESS) {
		if (pLwc->field) {
			field_nav_query(pLwc->field);
			LOGI("Nav: path query result - %d points", nav_path_query_n_smooth_path(field_nav(pLwc->field)));
		}
		else {
			puck_game_push(pLwc->puck_game);
		}
		
	}

	if (key == GLFW_KEY_R && action == GLFW_PRESS) {
		lw_trigger_reset(pLwc);
	}

	if (key == GLFW_KEY_U && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		lwc_update(pLwc, 1.0 / 60);
	}

	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
		lw_trigger_play_sound(pLwc);
	}

	if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		lw_trigger_key_right(pLwc);
	}

	if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		lw_trigger_key_left(pLwc);
	}

	handle_move_key_press_release(pLwc, key, action);

	if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
		lw_trigger_key_enter(pLwc);
	}

	if (key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS) {
		change_to_admin(pLwc);
	}

	if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
		change_to_field(pLwc);
	}

	if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
		change_to_dialog(pLwc);
	}

	if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
		change_to_battle(pLwc);
	}

	if (key == GLFW_KEY_F4 && action == GLFW_PRESS) {
		change_to_font_test(pLwc);
	}

	if (key == GLFW_KEY_F5 && action == GLFW_PRESS) {
		change_to_battle_result(pLwc);
	}

	if (key == GLFW_KEY_F6 && action == GLFW_PRESS) {
		change_to_skin(pLwc);
	}

	if (key == GLFW_KEY_F7 && action == GLFW_PRESS) {
		change_to_puck_game(pLwc);
	}

	if (key == GLFW_KEY_F8 && action == GLFW_PRESS) {
		change_to_particle_system(pLwc);
	}

	if (key == GLFW_KEY_F9 && action == GLFW_PRESS) {
		toggle_font_texture_test_mode(pLwc);
	}

    // World Roll West
    if (key == GLFW_KEY_KP_4 && action == GLFW_PRESS) {
        puck_game_roll_world(pLwc->puck_game, 1, 1, (float)LWDEG2RAD(180));
    }
    // World Roll East
    if (key == GLFW_KEY_KP_6 && action == GLFW_PRESS) {
        puck_game_roll_world(pLwc->puck_game, 0, 1, (float)LWDEG2RAD(180));
    }
    // World Roll Center (puck battle scene)
    if (key == GLFW_KEY_KP_5 && action == GLFW_PRESS) {
        puck_game_roll_to_battle(pLwc->puck_game);
    }
    // World Roll North
    if (key == GLFW_KEY_KP_8 && action == GLFW_PRESS) {
        puck_game_roll_world(pLwc->puck_game, 0, 0, (float)LWDEG2RAD(180));
    }
    // World Roll South
    if (key == GLFW_KEY_KP_2 && action == GLFW_PRESS) {
        puck_game_roll_world(pLwc->puck_game, 1, 0, (float)LWDEG2RAD(180));
    }
    // Start second client
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        run_additional_client(pLwc->argv[0], 1);
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
        run_additional_client(pLwc->argv[0], 2);
    }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
        run_additional_client(pLwc->argv[0], 3);
    }
}

void char_callback(GLFWwindow* window, unsigned int c) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
	lwimgui_char_callback(window, c);
	if (lwimgui_want_capture_keyboard() || lwimgui_want_text_input()) {
		return;
	}
#endif
}
