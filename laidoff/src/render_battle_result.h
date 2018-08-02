#pragma once

typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWUIDIM LWUIDIM;

void lwc_render_battle_result(const LWCONTEXT* pLwc);
void get_player_creature_result_ui_box(int pos,
                                       float viewport_rt_x,
                                       float viewport_rt_y,
                                       float* left_top_x,
                                       float* left_top_y,
                                       float* area_width,
                                       float* area_height);
void get_battle_result_next_button_dim(LWUIDIM* d);
