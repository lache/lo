#include "lwatlasenum.h"
#include "lwcontext.h"
#include "lwgl.h"
#include "ktx.h"
#include "lwlog.h"
#include "laidoff.h"
#include <memory.h>
#include "rmsg.h"

void lw_load_tex_async(const LWCONTEXT* _pLwc, int lae) {
    LWCONTEXT* pLwc = (LWCONTEXT*)_pLwc;
    rmsg_loadtex(pLwc, lae);
}

void lw_calculate_all_tex_atlas_hash(LWCONTEXT* pLwc) {
    for (int lae = 0; lae < LAE_COUNT; lae++) {
        size_t tex_atlas_filename_len = (int)strlen(tex_atlas_filename[lae]);
        size_t filename_index = tex_atlas_filename_len;
        // take only filename
        while (tex_atlas_filename[lae][filename_index - 1] != PATH_SEPARATOR[0]) {
            filename_index--;
        }
        pLwc->tex_atlas_hash[lae] = hash((const unsigned char*)&tex_atlas_filename[lae][filename_index]);
    }
}

void lw_load_tex(const LWCONTEXT* _pLwc, int lae) {
    LWCONTEXT* pLwc = (LWCONTEXT*)_pLwc;
    if (pLwc->tex_atlas_ready[lae]) {
        return;
    }
    GLenum last_gl_error = glGetError(); // ignore this result
    glBindTexture(GL_TEXTURE_2D, pLwc->tex_atlas[lae]);
    last_gl_error = glGetError();
    size_t tex_atlas_filename_len = (int)strlen(tex_atlas_filename[lae]);
    size_t filename_index = tex_atlas_filename_len;
    // take only filename
    while (tex_atlas_filename[lae][filename_index - 1] != PATH_SEPARATOR[0]) {
        filename_index--;
    }
    // ktx? png? pkm?
    if (strcmp(tex_atlas_filename[lae] + tex_atlas_filename_len - 4, ".ktx") == 0) {
        int width = 0, height = 0;
        if (load_ktx_hw_or_sw(tex_atlas_filename[lae], &width, &height) < 0) {
            LOGE("load_tex_files: load_ktx_hw_or_sw failure - %s", tex_atlas_filename[lae]);
        } else {
            pLwc->tex_atlas_width[lae] = width;
            pLwc->tex_atlas_height[lae] = height;
        }
    } else if (strcmp(tex_atlas_filename[lae] + tex_atlas_filename_len - 4, ".png") == 0) {
        // Software decoding of PNG
        load_png_pkm_sw_decoding(pLwc, lae);
    } else if (strcmp(tex_atlas_filename[lae] + tex_atlas_filename_len - 4, ".pkm") == 0) {
#if LW_SUPPORT_ETC1_HARDWARE_DECODING
        load_pkm_hw_decoding(tex_atlas_filename[lae]);
#else
        load_png_pkm_sw_decoding(pLwc, lae);
#endif
    } else {
        LOGE("load_tex_files: unknown tex file extension - %s", tex_atlas_filename[lae]);
    }
    last_gl_error = glGetError();
    pLwc->tex_atlas_ready[lae] = 1;
}

void lazy_tex_atlas_glBindTexture(const LWCONTEXT* _pLwc, int lae) {
    LWCONTEXT* pLwc = (LWCONTEXT*)_pLwc;
    lw_load_tex(pLwc, lae);
    glBindTexture(GL_TEXTURE_2D, pLwc->tex_atlas[lae]);
}
