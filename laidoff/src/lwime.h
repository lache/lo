#pragma once
#ifdef __cplusplus
extern "C" {;
#endif
enum LW_IME_TEXT_INPUT {
    LITI_NICKNAME = 100,
    LITI_SERVER_ADDR = 200,
};
char* lw_get_text_input_for_writing();
const char* lw_get_text_input();
int lw_get_text_input_seq();
void lw_increase_text_input_seq();
int lw_text_input_max();
void lw_set_text_input_tag(int tag);
int lw_text_input_tag();
#ifdef __cplusplus
};
#endif
