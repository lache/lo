#include "lwime.h"
#include <string.h>
// Global scope shared storage for native IME input text
#define LW_IME_TEXT_INPUT_MAX (512)
static char text_input[LW_IME_TEXT_INPUT_MAX];
static int text_input_seq;
static int text_input_tag;

char* lw_get_text_input_for_writing() {
	memset(text_input, 0, sizeof(text_input));
    return text_input;
}

const char* lw_get_text_input() {
    return text_input;
}

int lw_get_text_input_seq() {
    return text_input_seq;
}

void lw_increase_text_input_seq() {
    text_input_seq++;
}

int lw_text_input_max() {
    return LW_IME_TEXT_INPUT_MAX;
}

void lw_set_text_input_tag(int tag) {
    text_input_tag = tag;
}

int lw_text_input_tag() {
    return text_input_tag;
}
