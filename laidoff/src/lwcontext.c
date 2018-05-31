#include "lwcontext.h"
#include "htmlui.h"
#include <stdlib.h>
#include "lwlog.h"

const vec4 EXP_COLOR = { 90 / 255.0f, 173 / 255.0f, 255 / 255.0f, 1 };

const char* tex_font_atlas_filename[] = {
	//ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "arita-semi-bold_0.tga",
	//ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "arita-semi-bold_1.tga",
	ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "test7_0.tga",
	ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "test7_1.tga",
};

int lwcontext_safe_to_start_render(const LWCONTEXT* pLwc) {
	return pLwc->safe_to_start_render;
}

void lwcontext_set_safe_to_start_render(LWCONTEXT* pLwc, int v) {
	pLwc->safe_to_start_render = v;
}

int lwcontext_rendering(const LWCONTEXT* pLwc) {
	return pLwc->rendering;
}

void lwcontext_set_rendering(LWCONTEXT* pLwc, int v) {
	pLwc->rendering = v;
}

void* lwcontext_mq(LWCONTEXT* pLwc) {
	return pLwc->mq;
}

LWFIELD* lwcontext_field(LWCONTEXT* pLwc) {
	return pLwc->field;
}

void lwcontext_inc_rmsg_send(LWCONTEXT* pLwc) {
	pLwc->rmsg_send_count++;
}

void lwcontext_inc_rmsg_recv(LWCONTEXT* pLwc) {
	pLwc->rmsg_recv_count++;
}

void lwcontext_set_custom_puck_game_stage(LWCONTEXT* pLwc, LW_VBO_TYPE lvt, LW_ATLAS_ENUM lae) {
    pLwc->puck_game_stage_lvt = lvt;
    pLwc->puck_game_stage_lae = lae;
}

void lwcontext_set_update_frequency(LWCONTEXT* pLwc, int hz) {
    if (hz > 0) {
        pLwc->update_frequency = hz;
        pLwc->update_interval = 1.0 / pLwc->update_frequency;
    } else {
        abort();
    }
}

float lwcontext_update_interval(LWCONTEXT* pLwc) {
    return 1.0f / pLwc->update_frequency;
}

int lwcontext_update_frequency(LWCONTEXT* pLwc) {
    return pLwc->update_frequency;
}

void lwcontext_rt_corner(const float aspect_ratio, float* x, float* y) {
    if (aspect_ratio == 0) {
        LOGEP("pLwc->aspect_ratio == 0");
        *x = 1.0f;
        *y = 1.0F;
    } else if (aspect_ratio < 1.0f) {
        *x = 1.0f;
        *y = 1.0f / aspect_ratio;
    } else {
        *x = aspect_ratio;
        *y = 1.0f;
    }
}
