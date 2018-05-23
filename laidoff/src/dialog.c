#include <string.h>
#include "dialog.h"
#include "lwcontext.h"
#include "laidoff.h"

static int parse_dialog_command(const char* dialog_instruction, const char* dialog, size_t dialog_command_len, char* first_arg)
{
	const size_t dialog_instruction_len = (int)strlen(dialog_instruction);

	if (strncmp(dialog, dialog_instruction, dialog_instruction_len) == 0)
	{
		const int arg_len = (int)(dialog_command_len - (dialog_instruction_len + 1));
		if (arg_len < 0)
		{
            first_arg[0] = '\0';
			return 0;
		}
		else
		{
			// minimized wcstombs (wcstombs not working correctly on Android)

			const size_t first_arg_len = dialog_command_len - (dialog_instruction_len + 1);

			for (size_t i = dialog_instruction_len + 1; i < dialog_command_len; i++)
			{
				first_arg[i - (dialog_instruction_len + 1)] = (unsigned char)(dialog[i] & 0xff);
			}
			first_arg[first_arg_len] = '\0';

			return (int)first_arg_len;
		}
	}

	return -1;
}

void update_dialog(LWCONTEXT* pLwc)
{
	// dialog commands
	if (pLwc->dialog && pLwc->dialog[pLwc->dialog_start_index] == '/')
	{
		int command_begin_index = pLwc->dialog_start_index;

		// skip dialog commands
		while (pLwc->dialog[pLwc->dialog_start_index] != '\r' && pLwc->dialog[pLwc->dialog_start_index] != '\n')
		{
			pLwc->dialog_start_index++;
		}

		int command_end_index = pLwc->dialog_start_index;

		// also skip the trailing new line
		while (pLwc->dialog[pLwc->dialog_start_index] == '\r' || pLwc->dialog[pLwc->dialog_start_index] == '\n')
		{
			pLwc->dialog_start_index++;
		}

		// process dialog commands...
		// [1] background change command
		int dialog_command_length = command_end_index - command_begin_index;

		char first_arg[128];
        //first_arg[0] = 0;

		if (parse_dialog_command("/bg", &pLwc->dialog[command_begin_index], dialog_command_length, first_arg) >= 0)
		{
			pLwc->dialog_bg_tex_index = get_tex_index_by_hash_key(pLwc, first_arg);
		}
		else if (parse_dialog_command("/p", &pLwc->dialog[command_begin_index], dialog_command_length, first_arg) >= 0)
		{
			pLwc->dialog_portrait_tex_index = get_tex_index_by_hash_key(pLwc, first_arg);
		}
	}

	if (pLwc->last_render_char_incr_time + 1 / 30.0f < pLwc->app_time) {
		pLwc->last_render_char_incr_time = pLwc->app_time;

		if (pLwc->dialog_start_index + pLwc->render_char >= pLwc->dialog_bytelen) {
		}
		else {
			if (pLwc->dialog[pLwc->dialog_start_index + pLwc->render_char] == '\n') {
				if (pLwc->dialog_move_next) {
					pLwc->dialog_move_next = 0;

					pLwc->dialog_start_index += pLwc->render_char + 1;
					pLwc->render_char = 0;
				}
			}
			else {
				if (pLwc->dialog_move_next) {
					pLwc->dialog_move_next = 0;
					while (pLwc->dialog[pLwc->dialog_start_index + (++pLwc->render_char)] !=
						'\n') {
					}
				}
				else {
					pLwc->render_char++;
					if (pLwc->render_char > pLwc->dialog_bytelen) {
						pLwc->render_char = pLwc->dialog_bytelen;
					}
				}
			}
		}
	}
}
