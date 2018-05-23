#include "laidoff.h"

static void handle_move_key_press_release(LWCONTEXT* pLwc, int key, int action)
{
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	LWCONTEXT* pLwc = (LWCONTEXT*)window;
	switch (key) {
		case 1: // ESCAPE
			lw_app_quit(pLwc);
			break;
		case 59: // F1
		change_to_field(window);
		break;
		case 60: // F2
		change_to_dialog(window);
		break;
		case 61: // F3
		change_to_battle(window);
		break;
		case 62: // F4
		change_to_font_test(window);
		break;
		case 105: // LEFT
		lw_trigger_key_left(window);
		break;
		case 106: // RIGHT
		lw_trigger_key_right(window);
		break;
		case 28:
		lw_trigger_key_enter(window);
		break;
	}
}
