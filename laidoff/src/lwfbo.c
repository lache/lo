#include "lwgl.h"
#include "lwfbo.h"
#include "lwlog.h"
#include "logic.h"
#include "lwcontext.h"

// https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2
static unsigned long upper_power_of_two(unsigned long v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

void lwfbo_init(LWFBO* fbo, int width, int height) {
    // Delete GL resources before init
    lwfbo_delete(fbo);
    // Start init
    fbo->width = width;
    fbo->height = height;
    // iOS does not support arbitrary size of FBOs
#if LW_PLATFORM_IOS
    fbo->tex_width = (int)upper_power_of_two((unsigned long)width);
    fbo->tex_height = (int)upper_power_of_two((unsigned long)height);
#else
    fbo->tex_width = fbo->width;
    fbo->tex_height = fbo->height;
#endif

    glGenFramebuffers(1, &fbo->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo);

    glGenRenderbuffers(1, &fbo->depth_render_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, fbo->depth_render_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fbo->tex_width, fbo->tex_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_COMPONENT, GL_RENDERBUFFER, fbo->depth_render_buffer);

    glGenTextures(1, &fbo->color_tex);
    glBindTexture(GL_TEXTURE_2D, fbo->color_tex);
    glGetError();
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 fbo->tex_width,
                 fbo->tex_height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 NULL);
    GLenum render_texture_result = glGetError();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo->color_tex, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGEP("glCheckFramebufferStatus failed. return = %d", status);
        lwfbo_delete(fbo);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void lwfbo_delete(LWFBO* fbo) {
    if (fbo->fbo) {
        glDeleteFramebuffers(1, &fbo->fbo);
        fbo->fbo = 0;
    }

    if (fbo->depth_render_buffer) {
        glDeleteRenderbuffers(1, &fbo->depth_render_buffer);
        fbo->depth_render_buffer = 0;
    }

    if (fbo->color_tex) {
        glDeleteTextures(1, &fbo->color_tex);
        fbo->color_tex = 0;
    }
}

int lwfbo_prerender(const LWCONTEXT* pLwc, const LWFBO* fbo) {
    if (fbo->fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo);
        glDisable(GL_DEPTH_TEST);
        glViewport(0, 0, fbo->width, fbo->height);
        // alpha should be cleared to zero
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        // overwrite ui projection matrix
        logic_update_default_ui_proj_for_htmlui(fbo->width, fbo->height, ((LWCONTEXT*)pLwc)->proj);
        return 0;
    } else {
        // might awoke from sleep mode?
        LOGEP("FBO ID is zero. Prerender failed.");
        return -1;
    }
}

void lwfbo_postrender(const LWCONTEXT* pLwc, const LWFBO* fbo) {
    // revert ui projection matrix
    logic_update_default_ui_proj(pLwc->window_width, pLwc->window_height, ((LWCONTEXT*)pLwc)->proj);
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
