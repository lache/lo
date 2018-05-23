#include <string.h>
#include "render_font_test.h"
#include "lwcontext.h"
#include "render_text_block.h"
#include "render_solid.h"
#include "laidoff.h"
#include "htmlui.h"
#include "linmath.h"
#include "lwmath.h"
#include <stdio.h>
#include "lwttl.h"
#include "lwlog.h"
#include "lwlnglat.h"

#define WATER_COLOR_R (0 / 255.f)
#define WATER_COLOR_G (94 / 255.f)
#define WATER_COLOR_B (190 / 255.f)

static void lwc_prerender_font_test_fbo(const LWCONTEXT* pLwc) {
    glBindFramebuffer(GL_FRAMEBUFFER, pLwc->shared_fbo.fbo);
    glDisable(GL_DEPTH_TEST);

    glViewport(0, 0, pLwc->shared_fbo.width, pLwc->shared_fbo.height);
    glClearColor(0, 0, 0, 0); // alpha should be cleared to zero
                              //lw_clear_color();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

static void lwc_postrender_font_test_fbo(const LWCONTEXT* pLwc) {
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void lwc_render_font_test_fbo(const LWCONTEXT* pLwc) {
    lwc_prerender_font_test_fbo(pLwc);

    LWTEXTBLOCK test_text_block;
    test_text_block.text_block_width = 999.0f;// 2.00f * aspect_ratio;
    test_text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_D;
    test_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_B;
    SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_outline, 0, 0, 0, 1);
    test_text_block.text = LWU("lqpM^_^123-45");
    test_text_block.text_bytelen = (int)strlen(test_text_block.text);
    test_text_block.begin_index = 0;
    test_text_block.end_index = test_text_block.text_bytelen;
    test_text_block.multiline = 1;

    // The first column (left aligned)

    test_text_block.text_block_x = -0.9f * pLwc->aspect_ratio;
    test_text_block.text_block_y = 0.25f;
    test_text_block.align = LTBA_LEFT_TOP;
    render_text_block(pLwc, &test_text_block);

    test_text_block.text_block_x = -0.9f * pLwc->aspect_ratio;
    test_text_block.text_block_y = 0;
    test_text_block.align = LTBA_LEFT_CENTER;
    render_text_block(pLwc, &test_text_block);

    test_text_block.text_block_x = -0.9f * pLwc->aspect_ratio;
    test_text_block.text_block_y = -0.25f;
    test_text_block.align = LTBA_LEFT_BOTTOM;
    render_text_block(pLwc, &test_text_block);

    // The second column (center aligned)

    test_text_block.text = LWU("lqpM^__^Mpql");
    test_text_block.text_bytelen = (int)strlen(test_text_block.text);
    test_text_block.begin_index = 0;
    test_text_block.end_index = test_text_block.text_bytelen;
    test_text_block.text_block_x = 0;
    test_text_block.text_block_y = 0.25f;
    test_text_block.align = LTBA_CENTER_TOP;
    render_text_block(pLwc, &test_text_block);

    test_text_block.text = LWU("가가가가가가가가가가가가가가가가가가가가");
    test_text_block.text_bytelen = (int)strlen(test_text_block.text);
    test_text_block.begin_index = 0;
    test_text_block.end_index = test_text_block.text_bytelen;
    test_text_block.text_block_x = 0;
    test_text_block.text_block_y = 0.50f;
    test_text_block.align = LTBA_CENTER_TOP;
    render_text_block(pLwc, &test_text_block);

    test_text_block.text = LWU("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    test_text_block.text_bytelen = (int)strlen(test_text_block.text);
    test_text_block.begin_index = 0;
    test_text_block.end_index = test_text_block.text_bytelen;
    test_text_block.text_block_x = 0;
    test_text_block.text_block_y = 0.75f;
    test_text_block.align = LTBA_CENTER_TOP;
    render_text_block(pLwc, &test_text_block);

    test_text_block.text = LWU("FBO pre-rendered 한글이 됩니다~~~");
    //test_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_A;
    test_text_block.text_bytelen = (int)strlen(test_text_block.text);
    test_text_block.begin_index = 0;
    test_text_block.end_index = test_text_block.text_bytelen;
    test_text_block.text_block_x = 0;
    test_text_block.text_block_y = 1.0f;
    test_text_block.align = LTBA_CENTER_TOP;
    render_text_block(pLwc, &test_text_block);

    test_text_block.text_block_x = 0;
    test_text_block.text_block_y = 0;
    test_text_block.align = LTBA_CENTER_CENTER;
    render_text_block(pLwc, &test_text_block);

    test_text_block.text = LWU("이제 진정하십시오...");
    test_text_block.text_bytelen = (int)strlen(test_text_block.text);
    test_text_block.begin_index = 0;
    test_text_block.end_index = test_text_block.text_bytelen;
    test_text_block.text_block_x = 0;
    test_text_block.text_block_y = -0.25f;
    test_text_block.align = LTBA_CENTER_BOTTOM;
    render_text_block(pLwc, &test_text_block);


    // The third Column (right aligned)

    test_text_block.text = LWU("lmqpMQ^__^ 123-45");
    //test_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_A;
    test_text_block.text_bytelen = (int)strlen(test_text_block.text);
    test_text_block.begin_index = 0;
    test_text_block.end_index = test_text_block.text_bytelen;
    test_text_block.text_block_x = 0.9f * pLwc->aspect_ratio;
    test_text_block.text_block_y = 0.25f;
    test_text_block.align = LTBA_RIGHT_TOP;
    render_text_block(pLwc, &test_text_block);

    test_text_block.text_block_x = 0.9f * pLwc->aspect_ratio;
    test_text_block.text_block_y = 0;
    test_text_block.align = LTBA_RIGHT_CENTER;
    render_text_block(pLwc, &test_text_block);

    test_text_block.text = LWU("국민 여러분!");
    test_text_block.text_bytelen = (int)strlen(test_text_block.text);
    test_text_block.begin_index = 0;
    test_text_block.end_index = test_text_block.text_bytelen;
    test_text_block.text_block_x = 0.9f * pLwc->aspect_ratio;
    test_text_block.text_block_y = -0.25f;
    test_text_block.align = LTBA_RIGHT_BOTTOM;
    render_text_block(pLwc, &test_text_block);

    lwc_postrender_font_test_fbo(pLwc);
}

void lwc_render_font_test(const LWCONTEXT* pLwc) {
    LW_GL_VIEWPORT();
    lw_clear_color();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // render FBO
    render_solid_box_ui_lvt_flip_y_uv(pLwc, 0, 0, 2 * pLwc->aspect_ratio, 2, pLwc->shared_fbo.color_tex, LVT_CENTER_CENTER_ANCHORED_SQUARE, 1);
}
