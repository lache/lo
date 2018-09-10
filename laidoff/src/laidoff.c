#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include "platform_detection.h"
#if LW_PLATFORM_WIN32
#include <WinSock2.h>
#endif
#include "lwgl.h"
#include "laidoff.h"
#include "lwbitmapcontext.h"
#include "constants.h"
#include "file.h"
#include "font.h"
#include "lwcontext.h"
#include "lwlog.h"
#include "ktx.h"
#include "tex.h"
#include "lwmacro.h"
#include "input.h"
#include "field.h"
#include "lwpkm.h"
#include "net.h"
#include "armature.h"
#include "lwanim.h"
#include "lwskinmesh.h"
#include "mq.h"
#include "sysmsg.h"
#include <czmq.h>
#include "logic.h"
#include "ps.h"
#include "render_ps.h"
#include "lwtimepoint.h"
#include "lwparabola.h"
#include "lwbutton.h"
#include "puckgame.h"
#include "lwudp.h"
#include "lwtcp.h"
#include "lwtcpclient.h"
#include "lwdamagetext.h"
#include "lwmath.h"
#include "puckgameupdate.h"
#include "numcomp_puck_game.h"
#include "lwvbo.h"
#include "htmlui.h"
#include "searoute.h"
#include "searoute2.h"
#include "lwttl.h"
#include "lwcountry.h"
#include "remtex.h"
#include "render_battle_result.h"
#include "render_puckgame.h"
#include "render_text_block.h"
#include "render_ttl.h"
#include "render_gazza.h"
#include "render_admin.h"
#include "render_skin.h"
#include "render_ui.h"
#include "render_splash.h"
#include "render_leaderboard.h"
#include "render_remtex.h"
#include "render_font_test.h"
#include "render_dynamic_vbo.h"
#include "render_tilemap.h"
#include "render_mocap.h"
#include "lwfbo.h"
#include "test_srp.h"
// SWIG output file
#include "lo_wrap.inl"

const float default_uv_offset[2] = { 0, 0 };
const float default_uv_scale[2] = { 1, 1 };
const float default_flip_y_uv_scale[2] = { 1, -1 };

#if LW_PLATFORM_ANDROID || LW_PLATFORM_IOS || LW_PLATFORM_IOS_SIMULATOR
#include "lwtimepoint.h"
#include "sound.h"

double glfwGetTime() {
    LWTIMEPOINT tp;
    lwtimepoint_now(&tp);
    return tp.last_time.tv_sec + (double)tp.last_time.tv_nsec / 1e9;
}

void glfwGetFramebufferSize(void *p, int *w, int *h) {
    // do nothing
}

struct GLFWwindow;
#endif

#pragma pack(push, 1)
typedef struct {
    char idlength;
    char colourmaptype;
    char datatypecode;
    short colourmaporigin;
    short colourmaplength;
    char colourmapdepth;
    short x_origin;
    short y_origin;
    short width;
    short height;
    char bitsperpixel;
    char imagedescriptor;
} TGAHEADER;
#pragma pack(pop)

void play_sound(LW_SOUND lws);
void stop_sound(LW_SOUND lws);
HRESULT init_ext_image_lib();
HRESULT init_ext_sound_lib();
void destroy_ext_sound_lib();
void create_image(const char *filename, LWBITMAPCONTEXT *pBitmapContext, int tex_atlas_index);
void release_image(LWBITMAPCONTEXT *pBitmapContext);
void lwc_render_battle(const LWCONTEXT* pLwc);
void lwc_render_dialog(const LWCONTEXT* pLwc);
void lwc_render_field(const LWCONTEXT* pLwc);
void init_load_textures(LWCONTEXT* pLwc);
void load_test_font(LWCONTEXT* pLwc);
int spawn_attack_trail(LWCONTEXT* pLwc, float x, float y, float z);
float get_battle_enemy_x_center(int enemy_slot_index);

void set_tex_filter(int min_filter, int mag_filter) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
}

//------------------------------------------------------------------------
unsigned short swap_bytes(unsigned short aData) {
    return (unsigned short)((aData & 0x00FF) << 8) | ((aData & 0xFF00) >> 8);
}

unsigned int swap_4_bytes(unsigned int num) {
    return ((num >> 24) & 0xff) | // move byte 3 to byte 0
        ((num << 8) & 0xff0000) | // move byte 1 to byte 2
        ((num >> 8) & 0xff00) | // move byte 2 to byte 1
        ((num << 24) & 0xff000000); // byte 0 to byte 3
}

void set_texture_parameter_values(const LWCONTEXT* pLwc, float x, float y, float w, float h,
                                  float atlas_w, float atlas_h, int shader_index) {

    const float offset[2] = {
        x / atlas_w,
        y / atlas_h,
    };

    const float scale[2] = {
        w / atlas_w,
        h / atlas_h,
    };

    glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, offset);
    glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, scale);
}

void
set_texture_parameter(const LWCONTEXT* pLwc, LWENUM _LW_ATLAS_ENUM lae, LWENUM _LW_ATLAS_SPRITE las) {
    set_texture_parameter_values(
        pLwc,
        (float)pLwc->sprite_data[las].x,
        (float)pLwc->sprite_data[las].y,
        (float)pLwc->sprite_data[las].w,
        (float)pLwc->sprite_data[las].h,
        (float)pLwc->tex_atlas_width[lae],
        (float)pLwc->tex_atlas_height[lae],
        0
    );
}

static void load_skin_vbo(LWCONTEXT* pLwc, const char *filename, LWVBO *pSvbo) {
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    size_t mesh_file_size = 0;
    char *mesh_vbo_data = create_binary_from_file(filename, &mesh_file_size);
    glBufferData(GL_ARRAY_BUFFER, mesh_file_size, mesh_vbo_data, GL_STATIC_DRAW);
    release_binary(mesh_vbo_data);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    pSvbo->vertex_buffer = vbo;
    pSvbo->vertex_count = (int)(mesh_file_size / skin_stride_in_bytes);
}

static void load_fan_vbo(LWCONTEXT* pLwc) {
    LWFANVERTEX fan_vertices[FAN_VERTEX_COUNT_PER_ARRAY];
    fan_vertices[0].r = 0;
    fan_vertices[0].theta = 0;
    fan_vertices[0].index = 0;
    for (int i = 1; i < FAN_VERTEX_COUNT_PER_ARRAY; i++) {
        fan_vertices[i].r = 1.0f;
        fan_vertices[i].theta = (float)((i - 1) * (2 * M_PI / FAN_SECTOR_COUNT_PER_ARRAY));
        fan_vertices[i].index = (float)i;
    }
    glGenBuffers(1, &pLwc->fan_vertex_buffer[LFVT_DEFAULT].vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, pLwc->fan_vertex_buffer[LFVT_DEFAULT].vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(LWFANVERTEX) * FAN_VERTEX_COUNT_PER_ARRAY,
                 fan_vertices, GL_STATIC_DRAW);
    pLwc->fan_vertex_buffer[LFVT_DEFAULT].vertex_count = FAN_VERTEX_COUNT_PER_ARRAY;
}

static void init_fvbo(LWCONTEXT* pLwc) {
    load_fvbo(pLwc, ASSETS_BASE_PATH "fvbo" PATH_SEPARATOR "whole-tower_cell.fvbo",
              &pLwc->fvertex_buffer[LFT_TOWER]);
}

static void init_fanim(LWCONTEXT* pLwc) {
    load_fanim(pLwc, ASSETS_BASE_PATH "fanim" PATH_SEPARATOR "whole-tower_cell.fanim",
               &pLwc->fanim[LFAT_TOWER_COLLAPSE]);
    load_fanim(pLwc, ASSETS_BASE_PATH "fanim" PATH_SEPARATOR "whole-tower_cell_octagon.fanim",
               &pLwc->fanim[LFAT_TOWER_COLLAPSE_OCTAGON]);
}

static void lwc_create_line_vbo(LWCONTEXT* pLwc) {
    if (pLwc->sea_route_vbo.vertex_buffer) {
        glDeleteBuffers(1, &pLwc->sea_route_vbo.vertex_buffer);
    }
    lw_load_vbo_data(pLwc, (const char*)sea_route_data_2, sizeof(sea_route_data_2), &pLwc->sea_route_vbo, sizeof(float) * 2);
}

static void load_morph_vbo(LWCONTEXT* pLwc, const char *filename, LWVBO *pSvbo) {
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    size_t mesh_file_size = 0;
    char *mesh_vbo_data = create_binary_from_file(filename, &mesh_file_size);
    glBufferData(GL_ARRAY_BUFFER, mesh_file_size, mesh_vbo_data, GL_STATIC_DRAW);
    release_binary(mesh_vbo_data);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    pSvbo->vertex_buffer = vbo;
    pSvbo->vertex_count = (int)(mesh_file_size / lwmorphvertex_stride_in_bytes);
}

static void init_vbo(LWCONTEXT* pLwc) {

    // === STATIC MESHES ===
    //lw_load_all_vbo(pLwc);

    // LVT_LEFT_TOP_ANCHORED_SQUARE,    LVT_CENTER_TOP_ANCHORED_SQUARE,     LVT_RIGHT_TOP_ANCHORED_SQUARE,
    // LVT_LEFT_CENTER_ANCHORED_SQUARE, LVT_CENTER_CENTER_ANCHORED_SQUARE,  LVT_RIGHT_CENTER_ANCHORED_SQUARE,
    // LVT_LEFT_BOTTOM_ANCHORED_SQUARE, LVT_CENTER_BOTTOM_ANCHORED_SQUARE,  LVT_RIGHT_BOTTOM_ANCHORED_SQUARE,
    // 9 anchored squares...
    const float anchored_square_offset[][2] = {
        { +1, -1 },{ +0, -1 },{ -1, -1 },
        { +1, +0 },{ +0, +0 },{ -1, +0 },
        { +1, +1 },{ +0, +1 },{ -1, +1 },
    };
    for (int i = 0; i < ARRAY_SIZE(anchored_square_offset); i++) {
        LWVERTEX square_vertices[VERTEX_COUNT_PER_ARRAY];
        memcpy(square_vertices, full_square_vertices, sizeof(full_square_vertices));
        for (int r = 0; r < VERTEX_COUNT_PER_ARRAY; r++) {
            square_vertices[r].x += anchored_square_offset[i][0];
            square_vertices[r].y += anchored_square_offset[i][1];
        }

        const LW_VBO_TYPE lvt = LVT_LEFT_TOP_ANCHORED_SQUARE + i;

        glGenBuffers(1, &pLwc->vertex_buffer[lvt].vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(LWVERTEX) * VERTEX_COUNT_PER_ARRAY,
                     square_vertices, GL_STATIC_DRAW);
        pLwc->vertex_buffer[lvt].vertex_count = VERTEX_COUNT_PER_ARRAY;
    }

    // LVT_OCTAGON_PLANE
    {
        LWVERTEX vertices[1 + 8 + 1]; // center plus eight vertices plus first vertex repeated
        memset(vertices, 0, sizeof(vertices));
        vertices[0].u = 0.5f;
        vertices[0].v = 0.5f;
        float h = 1.0f / cosf((float)(M_PI / 8));
        for (int r = 0; r < 9; r++) {
            float ct = cosf((float)(M_PI / 4 * r - M_PI / 8));
            float st = sinf((float)(M_PI / 4 * r - M_PI / 8));
            vertices[r + 1].x = h * ct;
            vertices[r + 1].y = h * st;
            vertices[r + 1].u = 0.5f + vertices[r + 1].x / 2;
            vertices[r + 1].v = 0.5f + vertices[r + 1].y / 2;
        }
        const LW_VBO_TYPE lvt = LVT_OCTAGON_PLANE;
        glGenBuffers(1, &pLwc->vertex_buffer[lvt].vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        pLwc->vertex_buffer[lvt].vertex_count = ARRAY_SIZE(vertices);
    }

    // === SKIN VERTEX BUFFERS ===

    // LSVT_TRIANGLE
    load_skin_vbo(pLwc, ASSETS_BASE_PATH "svbo" PATH_SEPARATOR "Triangle.svbo",
                  &pLwc->skin_vertex_buffer[LSVT_TRIANGLE]);

    // LSVT_TREEPLANE
    load_skin_vbo(pLwc, ASSETS_BASE_PATH "svbo" PATH_SEPARATOR "TreePlane.svbo",
                  &pLwc->skin_vertex_buffer[LSVT_TREEPLANE]);

    // LSVT_HUMAN
    load_skin_vbo(pLwc, ASSETS_BASE_PATH "svbo" PATH_SEPARATOR "Human.svbo",
                  &pLwc->skin_vertex_buffer[LSVT_HUMAN]);

    // LSVT_DETACHPLANE
    load_skin_vbo(pLwc, ASSETS_BASE_PATH "svbo" PATH_SEPARATOR "DetachPlane.svbo",
                  &pLwc->skin_vertex_buffer[LSVT_DETACHPLANE]);

    // LSVT_GUNTOWER
    load_skin_vbo(pLwc, ASSETS_BASE_PATH "svbo" PATH_SEPARATOR "guntower.svbo",
                  &pLwc->skin_vertex_buffer[LSVT_GUNTOWER]);

    // LSVT_TURRET
    load_skin_vbo(pLwc, ASSETS_BASE_PATH "svbo" PATH_SEPARATOR "turret.svbo",
                  &pLwc->skin_vertex_buffer[LSVT_TURRET]);

    // LSVT_CROSSBOW
    load_skin_vbo(pLwc, ASSETS_BASE_PATH "svbo" PATH_SEPARATOR "crossbow.svbo",
                  &pLwc->skin_vertex_buffer[LSVT_CROSSBOW]);

    // LSVT_CATAPULT
    load_skin_vbo(pLwc, ASSETS_BASE_PATH "svbo" PATH_SEPARATOR "catapult.svbo",
                  &pLwc->skin_vertex_buffer[LSVT_CATAPULT]);

    // LSVT_PYRO
    load_skin_vbo(pLwc, ASSETS_BASE_PATH "svbo" PATH_SEPARATOR "pyro.svbo",
                  &pLwc->skin_vertex_buffer[LSVT_PYRO]);

    // === STATIC MESHES (FAN TYPE) ===
    load_fan_vbo(pLwc);

    lwc_create_ui_vbo(pLwc);

    lwc_create_line_vbo(pLwc);

    // === MORPH VERTEX BUFFERS ===
    // LMVT_EARTH
    load_morph_vbo(pLwc, ASSETS_BASE_PATH "mvbo" PATH_SEPARATOR "earth.mvbo",
                   &pLwc->morph_vertex_buffer[LMVT_EARTH]);
}

void set_vertex_attrib_pointer(const LWCONTEXT* pLwc, int shader_index) {
    lw_create_lazy_shader_program(pLwc, (LW_SHADER_TYPE)shader_index);
    // vertex coordinates
    if (pLwc->shader[shader_index].vpos_location >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].vpos_location);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].vpos_location, 3, GL_FLOAT, GL_FALSE,
                              lwvertex_stride_in_bytes, (void *)0);
    }
    // vertex color / normal
    if (pLwc->shader[shader_index].vcol_location >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].vcol_location);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].vcol_location, 3, GL_FLOAT, GL_FALSE,
                              lwvertex_stride_in_bytes, (void *)(sizeof(float) * 3));
    }
    // uv coordinates
    if (pLwc->shader[shader_index].vuv_location >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].vuv_location);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].vuv_location, 2, GL_FLOAT, GL_FALSE,
                              lwvertex_stride_in_bytes, (void *)(sizeof(float) * (3 + 3)));
    }
    // scale-9 coordinates
    if (pLwc->shader[shader_index].vs9_location >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].vs9_location);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].vs9_location, 2, GL_FLOAT, GL_FALSE,
                              lwvertex_stride_in_bytes, (void *)(sizeof(float) * (3 + 3 + 2)));
    }
}

void set_color_vertex_attrib_pointer(const LWCONTEXT* pLwc, int shader_index) {
    lw_create_lazy_shader_program(pLwc, (LW_SHADER_TYPE)shader_index);
    // vertex coordinates
    if (pLwc->shader[shader_index].vpos_location >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].vpos_location);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].vpos_location, 3, GL_FLOAT, GL_FALSE,
                              lwcolorvertex_stride_in_bytes, (void *)0);
    }
    // vertex normal
    if (pLwc->shader[shader_index].vnor_location >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].vnor_location);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].vnor_location, 3, GL_FLOAT, GL_FALSE,
                              lwcolorvertex_stride_in_bytes, (void *)(sizeof(float) * 3));
    }
    // vertex color
    if (pLwc->shader[shader_index].vcol_location >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].vcol_location);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].vcol_location, 3, GL_FLOAT, GL_FALSE,
                              lwcolorvertex_stride_in_bytes, (void *)(sizeof(float) * (3 + 3)));
    }
}

void set_skin_vertex_attrib_pointer(const LWCONTEXT* pLwc, int shader_index) {
    lw_create_lazy_shader_program(pLwc, (LW_SHADER_TYPE)shader_index);
    // vertex coordinates
    if (pLwc->shader[shader_index].vpos_location >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].vpos_location);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].vpos_location, 3, GL_FLOAT, GL_FALSE,
                              skin_stride_in_bytes, (void *)0);
    }
    // vertex color / normal
    if (pLwc->shader[shader_index].vcol_location >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].vcol_location);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].vcol_location, 3, GL_FLOAT, GL_FALSE,
                              skin_stride_in_bytes, (void *)(sizeof(float) * 3));
    }
    // uv coordinates
    if (pLwc->shader[shader_index].vuv_location >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].vuv_location);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].vuv_location, 2, GL_FLOAT, GL_FALSE,
                              skin_stride_in_bytes, (void *)(sizeof(float) * (3 + 3)));
    }
    // bone weights
    if (pLwc->shader[shader_index].vbweight_location >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].vbweight_location);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].vbweight_location, 4, GL_FLOAT, GL_FALSE,
                              skin_stride_in_bytes, (void *)(sizeof(float) * (3 + 3 + 2)));
    }
    // bone transformations
    if (pLwc->shader[shader_index].vbmat_location >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].vbmat_location);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].vbmat_location, 4, GL_FLOAT, GL_FALSE,
                              skin_stride_in_bytes, (void *)(sizeof(float) * (3 + 3 + 2 + 4)));
    }
}

void set_fan_vertex_attrib_pointer(const LWCONTEXT* pLwc, int shader_index) {
    lw_create_lazy_shader_program(pLwc, (LW_SHADER_TYPE)shader_index);
    // vertex coordinates
    if (pLwc->shader[shader_index].vpos_location >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].vpos_location);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].vpos_location, 3, GL_FLOAT, GL_FALSE,
                              fan_stride_in_bytes, (void *)0);
    }
}

void set_ps_vertex_attrib_pointer(const LWCONTEXT* pLwc, int shader_index) {
    lw_create_lazy_shader_program(pLwc, (LW_SHADER_TYPE)shader_index);
    if (pLwc->shader[shader_index].a_pID >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].a_pID);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].a_pID, 1, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE2), (void*)(LWOFFSETOF(LWPARTICLE2, pId)));
    }
    if (pLwc->shader[shader_index].a_pRadiusOffset >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].a_pRadiusOffset);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].a_pRadiusOffset, 1, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE2), (void*)(LWOFFSETOF(LWPARTICLE2, pRadiusOffset)));
    }
    if (pLwc->shader[shader_index].a_pVelocityOffset >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].a_pVelocityOffset);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].a_pVelocityOffset, 1, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE2), (void*)(LWOFFSETOF(LWPARTICLE2, pVelocityOffset)));
    }
    if (pLwc->shader[shader_index].a_pDecayOffset >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].a_pDecayOffset);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].a_pDecayOffset, 1, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE2), (void*)(LWOFFSETOF(LWPARTICLE2, pDecayOffset)));
    }
    if (pLwc->shader[shader_index].a_pSizeOffset >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].a_pSizeOffset);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].a_pSizeOffset, 1, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE2), (void*)(LWOFFSETOF(LWPARTICLE2, pSizeOffset)));
    }
    if (pLwc->shader[shader_index].a_pColorOffset >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].a_pColorOffset);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].a_pColorOffset, 3, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE2), (void*)(LWOFFSETOF(LWPARTICLE2, pColorOffset)));
    }
}

void set_ps0_vertex_attrib_pointer(const LWCONTEXT* pLwc, int shader_index) {
    lw_create_lazy_shader_program(pLwc, (LW_SHADER_TYPE)shader_index);
    if (pLwc->shader[shader_index].theta_location >= 0) {
        glEnableVertexAttribArray(pLwc->shader[shader_index].theta_location);
        glVertexAttribPointer(pLwc->shader[shader_index].theta_location, 1, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE), (void*)LWOFFSETOF(LWPARTICLE, theta));
    }
    if (pLwc->shader[shader_index].shade_location >= 0) {
        glEnableVertexAttribArray(pLwc->shader[shader_index].shade_location);
        glVertexAttribPointer(pLwc->shader[shader_index].shade_location, 3, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE), (void*)LWOFFSETOF(LWPARTICLE, shade));
    }
}

void set_line_vertex_attrib_pointer(const LWCONTEXT* pLwc, int shader_index) {
    lw_create_lazy_shader_program(pLwc, (LW_SHADER_TYPE)shader_index);
    // vertex coordinates
    if (pLwc->shader[shader_index].vpos_location >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].vpos_location);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].vpos_location, 2 /* 2D points */, GL_FLOAT, GL_FALSE,
                              line_stride_in_bytes, (void *)0);
    }
}

void set_morph_vertex_attrib_pointer(const LWCONTEXT* pLwc, int shader_index) {
    lw_create_lazy_shader_program(pLwc, (LW_SHADER_TYPE)shader_index);
    // vertex coordinates 0
    if (pLwc->shader[shader_index].vpos_location >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].vpos_location);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].vpos_location, 3, GL_FLOAT, GL_FALSE,
                              lwmorphvertex_stride_in_bytes, (void *)0);
    }
    // vertex coordinates 1
    if (pLwc->shader[shader_index].vpos2_location >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].vpos2_location);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].vpos2_location, 3, GL_FLOAT, GL_FALSE,
                              lwmorphvertex_stride_in_bytes, (void *)(sizeof(float) * 3));
    }
    // uv coordinates
    if (pLwc->shader[shader_index].vuv_location >= 0) {
        glEnableVertexAttribArray((GLuint)pLwc->shader[shader_index].vuv_location);
        glVertexAttribPointer((GLuint)pLwc->shader[shader_index].vuv_location, 2, GL_FLOAT, GL_FALSE,
                              lwmorphvertex_stride_in_bytes, (void *)(sizeof(float) * (3 + 3)));
    }
}

static void gen_all_vao(LWCONTEXT* pLwc) {
    // Vertex Array Objects
#if LW_SUPPORT_VAO
    glGenVertexArrays(VERTEX_BUFFER_COUNT, pLwc->vao);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
#endif
}

static void init_fvao(LWCONTEXT* pLwc, int shader_index) {
    // Vertex Array Objects for FVBO
#if LW_SUPPORT_VAO
    glGenVertexArrays(LFT_COUNT, pLwc->fvao);
    for (int i = 0; i < LFT_COUNT; i++) {
        glBindVertexArray(pLwc->fvao[i]);
        glBindBuffer(GL_ARRAY_BUFFER, pLwc->fvertex_buffer[i].vertex_buffer);
        set_vertex_attrib_pointer(pLwc, shader_index);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
#endif
}

static void init_skin_vao(LWCONTEXT* pLwc, int shader_index) {
    // Skin Vertex Array Objects
#if LW_SUPPORT_VAO
    glGenVertexArrays(SKIN_VERTEX_BUFFER_COUNT, pLwc->skin_vao);
    for (int i = 0; i < SKIN_VERTEX_BUFFER_COUNT; i++) {
        glBindVertexArray(pLwc->skin_vao[i]);
        glBindBuffer(GL_ARRAY_BUFFER, pLwc->skin_vertex_buffer[i].vertex_buffer);
        set_skin_vertex_attrib_pointer(pLwc, shader_index);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
#endif
}

static void init_fan_vao(LWCONTEXT* pLwc, int shader_index) {
    // Skin Vertex Array Objects
#if LW_SUPPORT_VAO
    glGenVertexArrays(FAN_VERTEX_BUFFER_COUNT, pLwc->fan_vao);
    for (int i = 0; i < FAN_VERTEX_BUFFER_COUNT; i++) {
        glBindVertexArray(pLwc->fan_vao[i]);
        glBindBuffer(GL_ARRAY_BUFFER, pLwc->fan_vertex_buffer[i].vertex_buffer);
        set_fan_vertex_attrib_pointer(pLwc, shader_index);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
#endif
}

static void init_ps_vao(LWCONTEXT* pLwc, int shader_index) {
    // Particle System Vertex Array Objects
#if LW_SUPPORT_VAO
    glGenVertexArrays(PS_VERTEX_BUFFER_COUNT, pLwc->ps_vao);
    for (int i = 0; i < PS_VERTEX_BUFFER_COUNT; i++) {
        glBindVertexArray(pLwc->ps_vao[i]);
        glBindBuffer(GL_ARRAY_BUFFER, pLwc->particle_buffer2);
        set_ps_vertex_attrib_pointer(pLwc, shader_index);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
#endif
}

static void init_ps0_vao(LWCONTEXT* pLwc, int shader_index) {
    // Particle System 0 (rose emitter) Vertex Array Objects
#if LW_SUPPORT_VAO
    assert(PS0_VERTEX_BUFFER_COUNT == 1);
    glGenVertexArrays(PS0_VERTEX_BUFFER_COUNT, pLwc->ps0_vao);
    for (int i = 0; i < PS0_VERTEX_BUFFER_COUNT; i++) {
        glBindVertexArray(pLwc->ps0_vao[i]);
        glBindBuffer(GL_ARRAY_BUFFER, pLwc->particle_buffer);
        set_ps0_vertex_attrib_pointer(pLwc, shader_index);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
#endif
}

static void init_line_vao(LWCONTEXT* pLwc, int shader_index) {
    // Particle System 0 (rose emitter) Vertex Array Objects
#if LW_SUPPORT_VAO
    assert(LINE_VERTEX_BUFFER_COUNT == 1);
    glGenVertexArrays(1, pLwc->line_vao);
    for (int i = 0; i < LINE_VERTEX_BUFFER_COUNT; i++) {
        glBindVertexArray(pLwc->line_vao[i]);
        glBindBuffer(GL_ARRAY_BUFFER, pLwc->sea_route_vbo.vertex_buffer);
        set_line_vertex_attrib_pointer(pLwc, shader_index);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
#endif
}

static void init_morph_vao(LWCONTEXT* pLwc, int shader_index) {
    // Skin Vertex Array Objects
#if LW_SUPPORT_VAO
    glGenVertexArrays(LMVT_COUNT, pLwc->morph_vao);
    for (int i = 0; i < LMVT_COUNT; i++) {
        glBindVertexArray(pLwc->morph_vao[i]);
        glBindBuffer(GL_ARRAY_BUFFER, pLwc->morph_vertex_buffer[i].vertex_buffer);
        set_morph_vertex_attrib_pointer(pLwc, shader_index);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
#endif
}

void lw_clear_color() {
    // Alpha component should be 1 in RPI platform.
    glClearColor(0x44 / 255.f, 0x4c / 255.f, 0x50 / 255.f, 1);
}

void init_ps(LWCONTEXT* pLwc) {
    pLwc->ps_context = ps_new_context(pLwc);
    ps_load_emitter(pLwc, pLwc->ps_context);
    ps_load_particles(pLwc, pLwc->ps_context);
}

static void init_gl_context(LWCONTEXT* pLwc) {
    init_vbo(pLwc);
    init_fvbo(pLwc);
    init_fanim(pLwc);
    // Particle system's VAOs are configured here. Should be called before setting VAOs.
    init_ps(pLwc);
    // Vertex Array Objects (used only when LW_SUPPORT_VAO is set)
    gen_all_vao(pLwc);
    init_fvao(pLwc, LWST_DEFAULT_NORMAL);
    init_skin_vao(pLwc, LWST_SKIN);
    init_fan_vao(pLwc, LWST_FAN);
    init_ps_vao(pLwc, LWST_EMITTER2);
    init_ps0_vao(pLwc, LWST_EMITTER);
    init_line_vao(pLwc, LWST_LINE);
    init_morph_vao(pLwc, LWST_MORPH);
    // load all textures
    init_load_textures(pLwc);
    // load font metadata
    pLwc->pFnt = load_fnt(ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "test7.fnt");
    //pLwc->pFnt = load_fnt(ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "neo.fnt");
    // Enable culling (CCW is default)
    glEnable(GL_CULL_FACE);
    // set default clear color
    lw_clear_color();
    // set default blend mode
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // enable depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
}

// http://www.cse.yorku.ca/~oz/hash.html
unsigned long hash(const unsigned char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

int get_tex_index_by_hash_key(const LWCONTEXT* pLwc, const char *hash_key) {
    if (hash_key[0] == 0) {
        return 0;
    }
    unsigned long h = hash((const unsigned char *)hash_key);
    for (int i = 0; i < MAX_TEX_ATLAS; i++) {
        if (pLwc->tex_atlas_hash[i] == h) {
            return i;
        }
    }
    LOGEP("hash_key '%s' not found on tex_atlas_hash array. returning 0", hash_key);
    return 0;
}

static void render_stat(const LWCONTEXT* pLwc) {
    if (pLwc->show_stat == 0) {
        return;
    }
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_CENTER_TOP;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH / 2;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_F;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
    char msg[128];
    sprintf(msg,
            "L:%.1f R:%.1f RMSG:%d(%d-%d) v%s",
            (float)(1.0 / deltatime_history_avg(pLwc->update_dt)),
            (float)(1.0 / deltatime_history_avg(pLwc->render_dt)),
            pLwc->rmsg_send_count - pLwc->rmsg_recv_count,
            pLwc->rmsg_send_count,
            pLwc->rmsg_recv_count,
            package_version());
    text_block.text = msg;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = 0;
    text_block.text_block_y = +pLwc->viewport_rt_y;
    text_block.multiline = 1;
    text_block.pixel_perfect = 0;
    render_text_block(pLwc, &text_block);
}

void render_addr(const LWCONTEXT* pLwc) {
    if (pLwc->show_stat == 0) {
        return;
    }
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_CENTER_BOTTOM;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_F;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
    char msg[128];
    sprintf(msg, "T:%s:%d / U:%s:%d",
            lw_tcp_addr(pLwc), lw_tcp_port(pLwc),
            lw_udp_addr(pLwc), lw_udp_port(pLwc)
    );
    text_block.text = msg;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = 0;
    text_block.text_block_y = -pLwc->viewport_rt_y;
    text_block.multiline = 1;
    text_block.pixel_perfect = 0;
    render_text_block(pLwc, &text_block);
}

void handle_rmsg_spawn(LWCONTEXT* pLwc, const LWFIELDRENDERCOMMAND* cmd) {
    for (int i = 0; i < MAX_RENDER_QUEUE_CAPACITY; i++) {
        if (!pLwc->render_command[i].key) {
            pLwc->render_command[i] = *cmd;
            return;
        }
    }
    LOGEP("maximum capacity exceeded");
    abort();
}

void handle_rmsg_anim(LWCONTEXT* pLwc, const LWFIELDRENDERCOMMAND* cmd) {
    for (int i = 0; i < MAX_RENDER_QUEUE_CAPACITY; i++) {
        if (pLwc->render_command[i].key == cmd->key) {
            pLwc->render_command[i].animstarttime = lwtimepoint_now_seconds();
            pLwc->render_command[i].actionid = cmd->actionid;
            pLwc->render_command[i].anim_marker_search_begin = 0;
            return;
        }
    }
    LOGEP("object key %d not exist", cmd->key);
    abort();
}

void handle_rmsg_despawn(LWCONTEXT* pLwc, const LWFIELDRENDERCOMMAND* cmd) {
    for (int i = 0; i < MAX_RENDER_QUEUE_CAPACITY; i++) {
        if (pLwc->render_command[i].key == cmd->key) {
            pLwc->render_command[i].key = 0;
            return;
        }
    }
    LOGEP("object key %d not exist", cmd->key);
    abort();
}

void handle_rmsg_pos(LWCONTEXT* pLwc, const LWFIELDRENDERCOMMAND* cmd) {
    for (int i = 0; i < MAX_RENDER_QUEUE_CAPACITY; i++) {
        if (pLwc->render_command[i].key == cmd->key) {
            memcpy(pLwc->render_command[i].pos, cmd->pos, sizeof(vec3));
            return;
        }
    }
    LOGEP("object key %d not exist", cmd->key);
    abort();
}

void handle_rmsg_turn(LWCONTEXT* pLwc, const LWFIELDRENDERCOMMAND* cmd) {
    for (int i = 0; i < MAX_RENDER_QUEUE_CAPACITY; i++) {
        if (pLwc->render_command[i].key == cmd->key) {
            pLwc->render_command[i].angle = cmd->angle;
            return;
        }
    }
    LOGEP("object key %d not exist", cmd->key);
    abort();
}

void handle_rmsg_rparams(LWCONTEXT* pLwc, const LWFIELDRENDERCOMMAND* cmd) {
    for (int i = 0; i < MAX_RENDER_QUEUE_CAPACITY; i++) {
        if (pLwc->render_command[i].key == cmd->key) {
            if (pLwc->render_command[i].objtype == 1) {
                // Skinned mesh
                pLwc->render_command[i].atlas = cmd->atlas;
                pLwc->render_command[i].skin_vbo = cmd->skin_vbo;
                pLwc->render_command[i].armature = cmd->armature;
            } else if (pLwc->render_command[i].objtype == 2) {
                // Static mesh
                pLwc->render_command[i].atlas = cmd->atlas;
                pLwc->render_command[i].vbo = cmd->skin_vbo;
                memcpy(pLwc->render_command[i].scale, cmd->scale, sizeof(vec3));
            }
            return;
        }
    }
    LOGEP("object key %d not exist", cmd->key);
    abort();
}

void handle_rmsg_bulletspawnheight(LWCONTEXT* pLwc, const LWFIELDRENDERCOMMAND* cmd) {
    for (int i = 0; i < MAX_RENDER_QUEUE_CAPACITY; i++) {
        if (pLwc->render_command[i].key == cmd->key) {
            pLwc->render_command[i].bullet_spawn_height = cmd->bullet_spawn_height;
            return;
        }
    }
    LOGEP("object key %d not exist", cmd->key);
    abort();
}

void lw_flag_logic_actor_to_quit_and_wait(LWCONTEXT* pLwc) {
    // set quit request flag to 1 to make notify logic loop quit
    pLwc->quit_request = 1;
    // wait for logic loop exit
    zsock_wait(pLwc->logic_actor);
}

void handle_rmsg_quitapp(LWCONTEXT* pLwc, const LWFIELDRENDERCOMMAND* cmd) {
    // quit logic actor(loop)
    lw_flag_logic_actor_to_quit_and_wait(pLwc);
    // execute platform-dependent quit process at the end
    lw_app_quit(pLwc, cmd->native_context);
}

void handle_rmsg_loadtex(LWCONTEXT* pLwc, const LWFIELDRENDERCOMMAND* cmd) {
    lw_load_tex(pLwc, cmd->atlas);
}

#if LW_PLATFORM_IOS
void lw_start_text_input_activity_ios(LWCONTEXT* pLwc, int tag);
#endif

void handle_rmsg_starttextinputactivity(LWCONTEXT* pLwc, const LWFIELDRENDERCOMMAND* cmd) {
#if LW_PLATFORM_IOS
    lw_start_text_input_activity_ios(pLwc, cmd->tag);
#else
    LOGEP("Not supported");
#endif
}

void handle_rmsg_redrawuifbo(LWCONTEXT* pLwc, const LWFIELDRENDERCOMMAND* cmd) {
    htmlui_load_redraw_fbo(pLwc->htmlui);
}

void delete_all_rmsgs(LWCONTEXT* pLwc) {
    zmq_msg_t rmsg;
    while (1) {
        zmq_msg_init(&rmsg);
        int rc = zmq_msg_recv(&rmsg, mq_rmsg_reader(pLwc->mq), ZMQ_DONTWAIT);
        zmq_msg_close(&rmsg);
        if (rc == -1) {
            break;
        }
    }
}
static void read_all_rmsgs(LWCONTEXT* pLwc) {
    if (pLwc->mq == 0) {
        return;
    }
    zmq_msg_t rmsg;
    while (1) {
        zmq_msg_init(&rmsg);
        int rc = zmq_msg_recv(&rmsg, mq_rmsg_reader(pLwc->mq), ZMQ_DONTWAIT);
        if (rc == -1) {
            zmq_msg_close(&rmsg);
            break;
        }
        lwcontext_inc_rmsg_recv(pLwc);
        // Process command here
        LWFIELDRENDERCOMMAND* cmd = zmq_msg_data(&rmsg);
        switch (cmd->cmdtype) {
        case LRCT_SPAWN:
            handle_rmsg_spawn(pLwc, cmd);
            break;
        case LRCT_ANIM:
            handle_rmsg_anim(pLwc, cmd);
            break;
        case LRCT_DESPAWN:
            handle_rmsg_despawn(pLwc, cmd);
            break;
        case LRCT_POS:
            handle_rmsg_pos(pLwc, cmd);
            break;
        case LRCT_TURN:
            handle_rmsg_turn(pLwc, cmd);
            break;
        case LRCT_RPARAMS:
            handle_rmsg_rparams(pLwc, cmd);
            break;
        case LRCT_BULLETSPAWNHEIGHT:
            handle_rmsg_bulletspawnheight(pLwc, cmd);
            break;
        case LRCT_QUITAPP:
            handle_rmsg_quitapp(pLwc, cmd);
            break;
        case LRCT_LOADTEX:
            handle_rmsg_loadtex(pLwc, cmd);
            break;
        case LRCT_STARTTEXTINPUTACTIVITY:
            handle_rmsg_starttextinputactivity(pLwc, cmd);
            break;
        case LRCT_REDRAWUIFBO:
            handle_rmsg_redrawuifbo(pLwc, cmd);
            break;
        }
        zmq_msg_close(&rmsg);
    }
}

void vec3_lerp(vec3 vm, vec3 va, vec3 vb, float t) {
    for (int i = 0; i < 3; i++) {
        vm[i] = va[i] * (1.0f - t) + vb[i] * t;
    }
}

void slerp(quat qm, const quat qa, const quat qb, float t) {
    // Calculate angle between them.
    float cosHalfTheta = qa[0] * qb[0] + qa[1] * qb[1] + qa[2] * qb[2] + qa[3] * qb[3];
    // if qa=qb or qa=-qb then theta = 0 and we can return qa
    if (fabsf(cosHalfTheta) >= 1.0) {
        qm[0] = qa[0]; qm[1] = qa[1]; qm[2] = qa[2]; qm[3] = qa[3];
        return;
    }
    // Calculate temporary values.
    float halfTheta = acosf(cosHalfTheta);
    float sinHalfTheta = sqrtf(1.0f - cosHalfTheta * cosHalfTheta);
    // if theta = 180 degrees then result is not fully defined
    // we could rotate around any axis normal to qa or qb
    if (fabsf(sinHalfTheta) < 0.001f) { // fabs is floating point absolute
        qm[0] = (qa[0] * 0.5f + qb[0] * 0.5f);
        qm[1] = (qa[1] * 0.5f + qb[1] * 0.5f);
        qm[2] = (qa[2] * 0.5f + qb[2] * 0.5f);
        qm[3] = (qa[3] * 0.5f + qb[3] * 0.5f);
        return;
    }
    float ratioA = sinf((1 - t) * halfTheta) / sinHalfTheta;
    float ratioB = sinf(t * halfTheta) / sinHalfTheta;
    //calculate Quaternion.
    qm[0] = (qa[0] * ratioA + qb[0] * ratioB);
    qm[1] = (qa[1] * ratioA + qb[1] * ratioB);
    qm[2] = (qa[2] * ratioA + qb[2] * ratioB);
    qm[3] = (qa[3] * ratioA + qb[3] * ratioB);
}

void linear_interpolate_state(LWPSTATE* p, LWPSTATE* state_buffer, int state_buffer_len, double sample_update_tick) {
    int sample1_idx = 0;
    double sample1_diff = DBL_MAX;
    int sample2_idx = 0;
    double sample2_diff = DBL_MAX;

    for (int i = 0; i < state_buffer_len; i++) {
        double d = state_buffer[i].update_tick - sample_update_tick;
        if (d >= 0) {
            if (sample2_diff > d) {
                sample2_diff = d;
                sample2_idx = i;
            }
        } else {
            if (sample1_diff > fabs(d)) {
                sample1_diff = fabs(d);
                sample1_idx = i;
            }
        }
    }
    if (sample1_idx != sample2_idx
        && state_buffer[sample1_idx].update_tick <= sample_update_tick && sample_update_tick <= state_buffer[sample2_idx].update_tick) {

        int update_tick_diff = state_buffer[sample2_idx].update_tick - state_buffer[sample1_idx].update_tick;
        double dist = sample_update_tick - state_buffer[sample1_idx].update_tick;

        double ratio = dist / update_tick_diff;

        quat player_quat1, puck_quat1, target_quat1;
        quat_from_mat4x4_skin(player_quat1, state_buffer[sample1_idx].player_rot);
        quat_from_mat4x4_skin(puck_quat1, state_buffer[sample1_idx].puck_rot);
        quat_from_mat4x4_skin(target_quat1, state_buffer[sample1_idx].target_rot);
        quat player_quat2, puck_quat2, target_quat2;
        quat_from_mat4x4_skin(player_quat2, state_buffer[sample2_idx].player_rot);
        quat_from_mat4x4_skin(puck_quat2, state_buffer[sample2_idx].puck_rot);
        quat_from_mat4x4_skin(target_quat2, state_buffer[sample2_idx].target_rot);
        quat player_quatm, puck_quatm, target_quatm;
        slerp(player_quatm, player_quat1, player_quat2, (float)ratio);
        slerp(puck_quatm, puck_quat1, puck_quat2, (float)ratio);
        slerp(target_quatm, target_quat1, target_quat2, (float)ratio);
        mat4x4_from_quat_skin(p->player_rot, player_quatm);
        mat4x4_from_quat_skin(p->puck_rot, puck_quatm);
        mat4x4_from_quat_skin(p->target_rot, target_quatm);
        vec3_lerp(p->player, state_buffer[sample1_idx].player, state_buffer[sample2_idx].player, (float)ratio);
        vec3_lerp(p->puck, state_buffer[sample1_idx].puck, state_buffer[sample2_idx].puck, (float)ratio);
        vec3_lerp(p->target, state_buffer[sample1_idx].target, state_buffer[sample2_idx].target, (float)ratio);
        //LOGI("Interpolate state ratio: %.3f", ratio);
    } else {
        //LOGE("Error in logic");
    }
}

static void dequeue_puck_game_state_and_apply(LWCONTEXT* pLwc) {
    LWPSTATE new_state;
    if (ringbuffer_dequeue(&pLwc->udp->state_ring_buffer, &new_state) == 0) {
        //double client_elapsed = lwtimepoint_now_seconds() - pLwc->udp->puck_state_sync_client_timepoint;
        //double sample_update_tick = (pLwc->udp->puck_state_sync_server_timepoint + client_elapsed) * state_sync_hz;
        //LWPSTATE sampled_state;
        //linear_interpolate_state(&sampled_state, pLwc->udp->state_buffer, LW_STATE_RING_BUFFER_CAPACITY, sample_update_tick);
        //memcpy(&pLwc->puck_game_state, &sampled_state, sizeof(LWPSTATE));

        // 'player' is currently playing player
        // 'target' is currently opponent player
        const int player_damage = pLwc->puck_game_state.bf.player_current_hp - new_state.bf.player_current_hp;
        if (player_damage > 0) {
            puck_game_shake_player(pLwc->puck_game, &pLwc->puck_game->pg_player[0]);
            LWPUCKGAMETOWER* tower = &pLwc->puck_game->tower[pLwc->puck_game->player_no == 2 ? 1 : 0/*player*/];
            puck_game_spawn_tower_damage_text(pLwc,
                                              pLwc->puck_game,
                                              tower,
                                              player_damage);
            // on_death...
            if (new_state.bf.player_current_hp == 0) {
                tower->collapsing = 1;
                tower->collapsing_time = 0;
                play_sound(LWS_COLLAPSE);
                play_sound(LWS_VICTORY);
            }
            tower->last_damaged_at = lwtimepoint_now_seconds();
        }
        const int target_damage = pLwc->puck_game_state.bf.target_current_hp - new_state.bf.target_current_hp;
        if (target_damage > 0) {
            puck_game_shake_player(pLwc->puck_game, &pLwc->puck_game->pg_target[0]);
            LWPUCKGAMETOWER* tower = &pLwc->puck_game->tower[pLwc->puck_game->player_no == 2 ? 0 : 1/*target*/];
            puck_game_spawn_tower_damage_text(pLwc,
                                              pLwc->puck_game,
                                              tower,
                                              target_damage);
            // on_death...
            if (new_state.bf.target_current_hp == 0) {
                tower->collapsing = 1;
                tower->collapsing_time = 0;
                play_sound(LWS_COLLAPSE);
                play_sound(LWS_DEFEAT);
            }
            tower->last_damaged_at = lwtimepoint_now_seconds();
        }
        // If the battle is finished (called once)
        if (puck_game_state_phase_finished(pLwc->puck_game_state.bf.phase) == 0
            && puck_game_state_phase_finished(new_state.bf.phase) == 1) {
            LOGIP("Battle finished. Destroying UDP context...");
            destroy_udp(&pLwc->udp);
            //puck_game_roll_to_main_menu(pLwc->puck_game);
            pLwc->puck_game->battle_control_ui_alpha = 0.0f;
        }
        // Overwrite old game state with a new one
        memcpy(&pLwc->puck_game_state, &new_state, sizeof(LWPSTATE));
        // Overwrite various sync state variables
        pLwc->puck_game->battle_phase = new_state.bf.phase;
        pLwc->puck_game->update_tick = new_state.update_tick;
        pLwc->puck_game->wall_hit_bit = new_state.bf.wall_hit_bit;
        pLwc->puck_game->puck_owner_player_no = new_state.bf.puck_owner_player_no;
        pLwc->puck_game->go[LPGO_PUCK].speed = new_state.puck_speed;
        memcpy(pLwc->puck_game->go[LPGO_PUCK].pos, new_state.puck, sizeof(pLwc->puck_game->go[LPGO_PUCK].pos));
    } else {
        LOGE("State buffer dequeue failed.");
    }
}

static void convert_state2_to_state(LWPSTATE* state, const LWPSTATE2* state2, const LWNUMCOMPPUCKGAME* numcomp) {
    state->type = LPGP_LWPSTATE;
    state->update_tick = state2->update_tick;
    numcomp_decompress_vec3(state->puck, state2->go[0].pos, &numcomp->v[LNVT_POS]);
    numcomp_decompress_vec3(state->player, state2->go[1].pos, &numcomp->v[LNVT_POS]);
    LOGIx("player z = %f", state->player[2]);
    numcomp_decompress_vec3(state->target, state2->go[2].pos, &numcomp->v[LNVT_POS]);
    numcomp_decompress_mat4x4(state->puck_rot, state2->go[0].rot, &numcomp->q[LNQT_ROT]);
    numcomp_decompress_mat4x4(state->player_rot, state2->go[1].rot, &numcomp->q[LNQT_ROT]);
    numcomp_decompress_mat4x4(state->target_rot, state2->go[2].rot, &numcomp->q[LNQT_ROT]);
    state->puck_speed = numcomp_decompress_float(state2->go[0].speed, &numcomp->f[LNFT_PUCK_SPEED]);
    state->player_speed = numcomp_decompress_float(state2->go[1].speed, &numcomp->f[LNFT_PUCK_SPEED]);
    state->target_speed = numcomp_decompress_float(state2->go[2].speed, &numcomp->f[LNFT_PUCK_SPEED]);
    state->puck_move_rad = numcomp_decompress_float(state2->go[0].move_rad, &numcomp->f[LNFT_PUCK_MOVE_RAD]);
    state->player_move_rad = numcomp_decompress_float(state2->go[1].move_rad, &numcomp->f[LNFT_PUCK_MOVE_RAD]);
    state->target_move_rad = numcomp_decompress_float(state2->go[2].move_rad, &numcomp->f[LNFT_PUCK_MOVE_RAD]);
    state->puck_reflect_size = numcomp_decompress_float(state2->puck_reflect_size, &numcomp->f[LNFT_PUCK_REFLECT_SIZE]);
    state->bf = state2->bf;
}

static void dequeue_puck_game_state2_and_push_to_state_queue(LWCONTEXT* pLwc) {
    LWPSTATE2 new_state2;
    if (ringbuffer_dequeue(&pLwc->udp->state2_ring_buffer, &new_state2) == 0) {
        LWPSTATE new_state;
        convert_state2_to_state(&new_state, &new_state2, &pLwc->udp->numcomp);
        ringbuffer_queue(&pLwc->udp->state_ring_buffer, &new_state);
    } else {
        LOGE("State2 buffer dequeue failed.");
    }
}

static void dequeue_from_state2_queue(LWCONTEXT* pLwc) {
    int size = ringbuffer_size(&pLwc->udp->state2_ring_buffer);
    if (size >= 1) {
        while (ringbuffer_size(&pLwc->udp->state2_ring_buffer) >= 6) {
            LWPSTATE2 pout_unused;
            ringbuffer_dequeue(&pLwc->udp->state2_ring_buffer, &pout_unused);
        }
        dequeue_puck_game_state2_and_push_to_state_queue(pLwc);
        dequeue_puck_game_state_and_apply(pLwc);
    } else {
        LOGIx("Puck game state2 buffer underrun");
    }
}

void lwc_prerender_mutable_context(LWCONTEXT* pLwc) {
    // update world roll angle
    if (pLwc->puck_game) {
        puck_game_update_world_roll(pLwc->puck_game);
        puck_game_follow_cam(pLwc, pLwc->puck_game);
    }
    if (pLwc->htmlui) {
        if (pLwc->game_scene == LGS_TTL
            || pLwc->game_scene == LGS_GAZZA
            || pLwc->game_scene == LGS_MOCAP
            || (pLwc->game_scene == LGS_PUCK_GAME && pLwc->puck_game->show_html_ui)) {
            htmlui_update_on_render_thread(pLwc->htmlui);
        }
    }
    if (pLwc->game_scene == LGS_TTL) {

    }
    if (pLwc->udp == 0 || pLwc->udp->ready == 0) {
        return;
    }
    // using LWPSTATE
    //dequeue_from_state_queue(pLwc);
    // using LWPSTATE2
    dequeue_from_state2_queue(pLwc);
}

void lwc_render(const LWCONTEXT* pLwc) {
    // Busy wait for rendering okay sign
    while (!lwcontext_safe_to_start_render(pLwc)) {}
    // Set rendering flag to 1 (ignoring const-ness.......)
    lwcontext_set_rendering((LWCONTEXT*)pLwc, 1);
    // Button count to zero (ignoring const-ness......)
    memset((void*)&pLwc->button_list, 0, sizeof(pLwc->button_list));
    // Tick rendering thread
    deltatime_tick(pLwc->render_dt);
    // Process all render messages (ignoring const-ness.......)
    read_all_rmsgs((LWCONTEXT*)pLwc);
    // Rendering function w.r.t. game scene dispatched here
    if (pLwc->game_scene == LGS_BATTLE) {
        lwc_render_battle(pLwc);
    } else if (pLwc->game_scene == LGS_DIALOG) {
        lwc_render_dialog(pLwc);
    } else if (pLwc->game_scene == LGS_FIELD) {
        if (pLwc->field) {
            lwc_render_field(pLwc);
        }
    } else if (pLwc->game_scene == LGS_FONT_TEST) {
        lwc_render_font_test(pLwc);
    } else if (pLwc->game_scene == LGS_TTL) {
        lwc_render_ttl(pLwc);
    } else if (pLwc->game_scene == LGS_GAZZA) {
        lwc_render_gazza(pLwc);
    } else if (pLwc->game_scene == LGS_MOCAP) {
        lwc_render_mocap(pLwc);
    } else if (pLwc->game_scene == LGS_ADMIN) {
        lwc_render_admin(pLwc);
    } else if (pLwc->game_scene == LGS_BATTLE_RESULT) {
        lwc_render_battle_result(pLwc);
    } else if (pLwc->game_scene == LGS_SKIN) {
        lwc_render_skin(pLwc);
    } else if (pLwc->game_scene == LGS_PUCK_GAME) {
        lwc_render_puck_game(pLwc, pLwc->puck_game_view, pLwc->puck_game_proj);
    } else if (pLwc->game_scene == LGS_PARTICLE_SYSTEM) {
        lwc_render_ps(pLwc, pLwc->ps_context);
    } else if (pLwc->game_scene == LGS_UI) {
        lwc_render_ui(pLwc);
    } else if (pLwc->game_scene == LGS_SPLASH) {
        lwc_render_splash(pLwc);
    } else if (pLwc->game_scene == LGS_LEADERBOARD) {
        lwc_render_leaderboard(pLwc);
    } else if (pLwc->game_scene == LGS_REMTEX) {
        lwc_render_remtex(pLwc);
    } else if (pLwc->game_scene == LGS_DYNAMIC_VBO) {
        lwc_render_dynamic_vbo(pLwc);
    } else if (pLwc->game_scene == LGS_TILEMAP) {
        lwc_render_tilemap(pLwc);
    }
    // Rendering a system message
    render_sys_msg(pLwc, pLwc->def_sys_msg);
    // Rendering stats
    glDisable(GL_DEPTH_TEST);
    render_stat(pLwc);
    render_addr(pLwc);
    glEnable(GL_DEPTH_TEST);
    htmlui_load_next_html_path(pLwc->htmlui);
    htmlui_load_next_html_body(pLwc->htmlui);
    remtex_render(pLwc->remtex, pLwc->htmlui);
    // Set rendering flag to 0 (ignoring const-ness......)
    lwcontext_set_rendering((LWCONTEXT*)pLwc, 0);
}

static void bind_all_fvertex_attrib_shader(const LWCONTEXT* pLwc, int shader_index, int fvbo_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
    glBindVertexArray(pLwc->fvao[fvbo_index]);
#else
    set_vertex_attrib_pointer(pLwc, shader_index);
#endif
}

static void bind_all_vertex_attrib_shader(const LWCONTEXT* pLwc, int shader_index, int vbo_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
    glBindVertexArray(pLwc->vao[vbo_index]);
#else
    if (shader_index != LWST_DEFAULT_NORMAL_COLOR) {
        set_vertex_attrib_pointer(pLwc, shader_index);
    } else {
        set_color_vertex_attrib_pointer(pLwc, shader_index);
    }
#endif
}

static void bind_all_skin_vertex_attrib_shader(const LWCONTEXT* pLwc, int shader_index, int vbo_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
    glBindVertexArray(pLwc->skin_vao[vbo_index]);
#else
    set_skin_vertex_attrib_pointer(pLwc, shader_index);
#endif
}

static void bind_all_fan_vertex_attrib_shader(const LWCONTEXT* pLwc, int shader_index, int vbo_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
    glBindVertexArray(pLwc->fan_vao[vbo_index]);
#else
    set_fan_vertex_attrib_pointer(pLwc, shader_index);
#endif
}

static void bind_all_ps_vertex_attrib_shader(const LWCONTEXT* pLwc, int shader_index, int vbo_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
    glBindVertexArray(pLwc->ps_vao[vbo_index]);
#else
    set_ps_vertex_attrib_pointer(pLwc, shader_index);
#endif
}

static void bind_all_ps0_vertex_attrib_shader(const LWCONTEXT* pLwc, int shader_index, int vbo_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
    glBindVertexArray(pLwc->ps0_vao[vbo_index]);
#else
    set_ps0_vertex_attrib_pointer(pLwc, shader_index);
#endif
}

static void bind_all_line_vertex_attrib_shader(const LWCONTEXT* pLwc, int shader_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
    glBindVertexArray(pLwc->line_vao[0]);
#else
    set_line_vertex_attrib_pointer(pLwc, shader_index);
#endif
}

static void bind_all_morph_vertex_attrib_shader(const LWCONTEXT* pLwc, int shader_index, int vbo_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
    glBindVertexArray(pLwc->morph_vao[vbo_index]);
#else
    set_morph_vertex_attrib_pointer(pLwc, shader_index);
#endif
}

void bind_all_fvertex_attrib(const LWCONTEXT* pLwc, int fvbo_index) {
    bind_all_fvertex_attrib_shader(pLwc, LWST_DEFAULT_NORMAL, fvbo_index);
}

void bind_all_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index) {
    bind_all_vertex_attrib_shader(pLwc, LWST_DEFAULT, vbo_index);
}

void bind_all_color_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index) {
    bind_all_vertex_attrib_shader(pLwc, LWST_DEFAULT_NORMAL_COLOR, vbo_index);
}

void bind_all_vertex_attrib_font(const LWCONTEXT* pLwc, int vbo_index, int shader_index) {
    bind_all_vertex_attrib_shader(pLwc, shader_index, vbo_index);
}

void bind_all_vertex_attrib_etc1_with_alpha(const LWCONTEXT* pLwc, int vbo_index) {
    bind_all_vertex_attrib_shader(pLwc, LWST_ETC1, vbo_index);
}

void bind_all_skin_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index) {
    bind_all_skin_vertex_attrib_shader(pLwc, LWST_SKIN, vbo_index);
}

void bind_all_fan_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index) {
    bind_all_fan_vertex_attrib_shader(pLwc, LWST_FAN, vbo_index);
}

void bind_all_ps_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index) {
    bind_all_ps_vertex_attrib_shader(pLwc, LWST_EMITTER2, vbo_index);
}

void bind_all_ps0_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index) {
    bind_all_ps0_vertex_attrib_shader(pLwc, LWST_EMITTER, vbo_index);
}

void bind_all_line_vertex_attrib(const LWCONTEXT* pLwc) {
    bind_all_line_vertex_attrib_shader(pLwc, LWST_LINE);
}

void bind_all_morph_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index) {
    bind_all_morph_vertex_attrib_shader(pLwc, LWST_MORPH, vbo_index);
}

void load_pkm_hw_decoding(const char *tex_atlas_filename) {
    size_t file_size = 0;
    char *b = create_binary_from_file(tex_atlas_filename, &file_size);
    if (!b) {
        LOGE("load_pkm_hw_decoding: create_binary_from_file null, filename %s", tex_atlas_filename);
        return;
    }
    LWPKM *pPkm = (LWPKM *)b;

    GLenum error_enum;

    short extended_width = swap_bytes(pPkm->extended_width);
    short extended_height = swap_bytes(pPkm->extended_height);

    // TODO: iOS texture
#if LW_SUPPORT_ETC1_HARDWARE_DECODING
    // calculate size of data with formula (extWidth / 4) * (extHeight / 4) * 8
    u32 dataLength = ((extended_width >> 2) * (extended_height >> 2)) << 3;

    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_ETC1_RGB8_OES, extended_width, extended_height, 0,
                           dataLength, b + sizeof(LWPKM));
#else
    LWBITMAPCONTEXT bitmap_context;
    create_image(tex_atlas_filename, &bitmap_context, 0);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 bitmap_context.width,
                 bitmap_context.height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 bitmap_context.data);
    error_enum = glGetError();
    LOGI("glTexImage2D (ETC1 software decompression) result (%dx%d): %d", bitmap_context.width, bitmap_context.height,
         error_enum);

    release_image(&bitmap_context);
#endif

    error_enum = glGetError();
    LOGI("glCompressedTexImage2D result (%dx%d): %d", extended_width, extended_height,
         error_enum);

    release_binary(b);
}

void load_png_pkm_sw_decoding(LWCONTEXT* pLwc, int i) {
    LWBITMAPCONTEXT bitmap_context;

    create_image(tex_atlas_filename[i], &bitmap_context, i);

    if (bitmap_context.width > 0 && bitmap_context.height > 0) {
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     bitmap_context.width,
                     bitmap_context.height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     bitmap_context.data);
        GLenum error_enum = glGetError();
        LOGI("glTexImage2D result (%dx%d): %d", bitmap_context.width, bitmap_context.height,
             error_enum);

        release_image(&bitmap_context);

        // all atlas sizes should be equal
        pLwc->tex_atlas_width[i] = bitmap_context.width;
        pLwc->tex_atlas_height[i] = bitmap_context.height;

        glGenerateMipmap(GL_TEXTURE_2D);
        error_enum = glGetError();
        LOGI("glGenerateMipmap result: %d", error_enum);
    } else {
        LOGE("create_image: %s not loaded. Width=%d, height=%d", tex_atlas_filename[i],
             bitmap_context.width, bitmap_context.height);
    }
}

static void gen_all_tex(LWCONTEXT* pLwc) {
    glGenTextures(MAX_TEX_ATLAS, pLwc->tex_atlas);
}

void init_load_textures(LWCONTEXT* pLwc) {
    // Sprites
    gen_all_tex(pLwc);
    //load_tex_files(pLwc);
    // Fonts
    load_test_font(pLwc);
    // Textures generated by program
    load_tex_program(pLwc);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void load_test_font(LWCONTEXT* pLwc) {
    glGenTextures(MAX_TEX_FONT_ATLAS, pLwc->tex_font_atlas);

    for (int i = 0; i < MAX_TEX_FONT_ATLAS; i++) {
        glBindTexture(GL_TEXTURE_2D, pLwc->tex_font_atlas[i]);

        size_t file_size = 0;
        char *b = create_binary_from_file(tex_font_atlas_filename[i], &file_size);
        if (!b) {
            LOGE("load_test_font: create_binary_from_file null, filename %s", tex_font_atlas_filename[i]);
            continue;
        }

        LOGI("load_test_font %s...", tex_font_atlas_filename[i]);

        const TGAHEADER *tga_header = (const TGAHEADER *)b;

        char *tex_data = (char *)malloc(tga_header->width * tga_header->height * 4);
        for (int j = 0; j < tga_header->width * tga_header->height; j++) {
            char v = *(b + sizeof(TGAHEADER) + j);

            tex_data[4 * j + 0] = v;
            tex_data[4 * j + 1] = v;
            tex_data[4 * j + 2] = v;
            tex_data[4 * j + 3] = v;
        }

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     tga_header->width,
                     tga_header->height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     tex_data);

        free(tex_data);

        GLenum error_enum = glGetError();
        LOGI("font glTexImage2D result (%dx%d): %d", tga_header->width, tga_header->height,
             error_enum);
        glGenerateMipmap(GL_TEXTURE_2D);
        error_enum = glGetError();
        LOGI("font glGenerateMipmap result: %d", error_enum);

        release_binary(b);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

void init_armature(LWCONTEXT* pLwc) {
    for (int i = 0; i < LWAR_COUNT; i++) {
        load_armature(armature_filename[i], &pLwc->armature[i]);
    }
}

void init_action(LWCONTEXT* pLwc) {
    for (int i = 0; i < LWAC_COUNT; i++) {
        load_action(action_filename[i], &pLwc->action[i]);
    }
}

LWCONTEXT* lw_init_initial_size(int width, int height) {
    // run on logic thread in script...
    // test_srp_main();

    init_ext_image_lib();

    init_ext_sound_lib();

    //test_image();

    LWCONTEXT* pLwc = (LWCONTEXT *)calloc(1, sizeof(LWCONTEXT));
    pLwc->puck_game_stage_lvt = LVT_DONTCARE;
    pLwc->puck_game_stage_lae = LAE_DONTCARE;
    pLwc->control_flags = LCF_PUCK_GAME_DASH /*| LCF_PUCK_GAME_JUMP | LCF_PUCK_GAME_PULL*/;

    pLwc->viewport_width = width;
    pLwc->viewport_height = height;

    setlocale(LC_ALL, "");

    lw_calculate_all_tex_atlas_hash(pLwc);

    init_gl_context(pLwc);

    pLwc->def_sys_msg = init_sys_msg();

    pLwc->update_dt = deltatime_new();

    deltatime_set_to_now(pLwc->update_dt);

    pLwc->render_dt = deltatime_new();

    deltatime_set_to_now(pLwc->render_dt);

    init_net(pLwc);

    pLwc->mq = init_mq(logic_server_addr(pLwc->server_index), pLwc->def_sys_msg);

    init_armature(pLwc);
    init_action(pLwc);

    lwparabola_test();

    lwcontext_set_update_frequency(pLwc, 125);

    pLwc->puck_game = new_puck_game(lwcontext_update_frequency(pLwc), LPGM_SQUARE);
    puck_game_set_static_default_values_client(pLwc->puck_game);
    pLwc->puck_game->pLwc = pLwc;

    float dir_pad_origin_x, dir_pad_origin_y;
    get_left_dir_pad_original_center(pLwc->viewport_aspect_ratio, &dir_pad_origin_x, &dir_pad_origin_y);
    dir_pad_init(&pLwc->left_dir_pad,
                 dir_pad_origin_x,
                 dir_pad_origin_y,
                 0.1f,
                 0.2f);
    get_right_dir_pad_original_center(pLwc->viewport_aspect_ratio, &dir_pad_origin_x, &dir_pad_origin_y);
    dir_pad_init(&pLwc->right_dir_pad,
                 dir_pad_origin_x,
                 dir_pad_origin_y,
                 0.1f,
                 0.2f);

    pLwc->htmlui = htmlui_new(pLwc);

    pLwc->ttl = lwttl_new(pLwc->viewport_aspect_ratio);

    lwttl_fill_world_seaports_bookmarks(pLwc->htmlui);

    pLwc->remtex = remtex_new(pLwc->tcp_host_addr.host);

    return pLwc;
}

LWCONTEXT* lw_init(void) {
    return lw_init_initial_size(0, 0);
}

int prefix(const char* pre, const char* str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

void lw_set_device_model(LWCONTEXT* pLwc, const char* model) {
    strcpy(pLwc->device_model, model);
    if (prefix("iPhone 4", model) || prefix("iPhone 3", model) || prefix("iPhone 1", model)) {
        pLwc->lowend_device = 1;
    }
}

void lw_set_window_size(LWCONTEXT* pLwc, int w, int h) {
    LOGIP("Trying to set window size old (%d, %d) --> new (%d, %d)",
          pLwc->window_width,
          pLwc->window_height,
          w,
          h);
    if (pLwc->window_width == w && pLwc->window_height == h) {
        LOGIP("Skipping to set window size (already set)");
        return;
    }
    pLwc->window_width = w;
    pLwc->window_height = h;
    if (pLwc->window_width > 0 && pLwc->window_height > 0) {
        pLwc->window_aspect_ratio = (float)pLwc->window_width / pLwc->window_height;
        lwcontext_rt_corner(pLwc->window_aspect_ratio, &pLwc->window_rt_x, &pLwc->window_rt_y);

        get_left_dir_pad_original_center(pLwc->window_aspect_ratio,
                                         &pLwc->left_dir_pad.origin_x,
                                         &pLwc->left_dir_pad.origin_y);
        get_right_dir_pad_original_center(pLwc->window_aspect_ratio,
                                          &pLwc->right_dir_pad.origin_x,
                                          &pLwc->right_dir_pad.origin_y);

        // Update default UI projection matrix (pLwc->proj)
        logic_update_default_ui_proj(pLwc->window_width, pLwc->window_height, pLwc->proj);

        // Reset dir pad input state
        reset_dir_pad_position(&pLwc->left_dir_pad);
        reset_dir_pad_position(&pLwc->right_dir_pad);

        lwttl_set_window_size(pLwc->ttl,
                              pLwc->window_width,
                              pLwc->window_height,
                              pLwc->window_aspect_ratio);

        if (pLwc->game_scene == LGS_PUCK_GAME || pLwc->game_scene == LGS_FONT_TEST || pLwc->game_scene == LGS_TTL) {
            // Resize FBO
            lwfbo_init(&pLwc->shared_fbo, pLwc->window_width, pLwc->window_height);

            if (pLwc->game_scene == LGS_PUCK_GAME || pLwc->game_scene == LGS_TTL) {
                // Rerender HTML UI
                htmlui_set_client_size(pLwc->htmlui, pLwc->window_width, pLwc->window_height);
                htmlui_load_redraw_fbo(pLwc->htmlui);
            } else {
                // Render font FBO using render-to-texture
                lwc_render_font_test_fbo(pLwc);
            }
        }
    } else {
        LOGE("Invalid window size detected!");
        if (pLwc->window_width <= 0) {
            LOGE("Window width is 0 or below! (%d) Width set to 1.0f", pLwc->window_width);
            pLwc->window_width = 1;
        }
        if (pLwc->window_height <= 0) {
            LOGE("Window height is 0 or below! (%d) Height set to 1.0f", pLwc->window_height);
            pLwc->window_height = 1;
        }
        pLwc->window_aspect_ratio = (float)pLwc->window_width / pLwc->window_height;
        lwcontext_rt_corner(pLwc->window_aspect_ratio, &pLwc->window_rt_x, &pLwc->window_rt_y);
    }
    LOGIP("New window size (%d, %d) [aspect ratio %f]", pLwc->window_width, pLwc->window_height, pLwc->window_aspect_ratio);
}

void lw_set_viewport_size(LWCONTEXT* pLwc, int w, int h) {
    // this function may called in every frame
    pLwc->viewport_width = w;
    pLwc->viewport_height = h;
    if (pLwc->viewport_width > 0 && pLwc->viewport_height > 0) {
        pLwc->viewport_aspect_ratio = (float)pLwc->viewport_width / pLwc->viewport_height;
        lwcontext_rt_corner(pLwc->viewport_aspect_ratio, &pLwc->viewport_rt_x, &pLwc->viewport_rt_y);

        // Update default UI projection matrix (pLwc->proj)
        logic_update_default_ui_proj(pLwc->viewport_width, pLwc->viewport_height, pLwc->proj);

        puck_game_reset_view_proj(pLwc, pLwc->puck_game);

        //lwttl_update_aspect_ratio(pLwc->ttl, pLwc->viewport_aspect_ratio);
    } else {
        LOGE("Invalid viewport size detected!");
        if (pLwc->viewport_width <= 0) {
            LOGE("Viewport width is 0 or below! (%d) Width set to 1.0f", pLwc->viewport_width);
            pLwc->viewport_width = 1;
        }
        if (pLwc->viewport_height <= 0) {
            LOGE("Viewport height is 0 or below! (%d) Height set to 1.0f", pLwc->viewport_height);
            pLwc->viewport_height = 1;
        }
        pLwc->viewport_aspect_ratio = (float)pLwc->viewport_width / pLwc->viewport_height;
        lwcontext_rt_corner(pLwc->viewport_aspect_ratio, &pLwc->viewport_rt_x, &pLwc->viewport_rt_y);
    }
}

void lw_set_window(LWCONTEXT* pLwc, struct GLFWwindow *window) {
    pLwc->window = window;
}

struct GLFWwindow *lw_get_window(const LWCONTEXT* pLwc) {
    return pLwc->window;
}

int lw_get_game_scene(LWCONTEXT* pLwc) {
    return pLwc->game_scene;
}

int lw_get_update_count(LWCONTEXT* pLwc) {
    return pLwc->update_count;
}

int lw_get_render_count(LWCONTEXT* pLwc) {
    return pLwc->render_count;
}

void lw_deinit(LWCONTEXT* pLwc) {
    lwc_destroy_tilemap();

    for (int i = 0; i < LAC_COUNT; i++) {
        if (pLwc->atlas_conf[i].first) {
            free(pLwc->atlas_conf[i].first);
            pLwc->atlas_conf[i].first = 0;
            pLwc->atlas_conf[i].count = 0;
        }
    }

    for (int i = 0; i < LVT_COUNT; i++) {
        glDeleteBuffers(1, &pLwc->vertex_buffer[i].vertex_buffer);
    }

    for (int i = 0; i < LFT_COUNT; i++) {
        glDeleteBuffers(1, &pLwc->fvertex_buffer[i].vertex_buffer);
    }

    for (int i = 0; i < LFAT_COUNT; i++) {
        release_binary(pLwc->fanim[i].data);
        memset(&pLwc->fanim[i], 0, sizeof(pLwc->fanim[i]));
    }

    for (int i = 0; i < LSVT_COUNT; i++) {
        glDeleteBuffers(1, &pLwc->skin_vertex_buffer[i].vertex_buffer);
    }

    for (int i = 0; i < LFVT_COUNT; i++) {
        glDeleteBuffers(1, &pLwc->fan_vertex_buffer[i].vertex_buffer);
    }

    if (pLwc->sea_route_vbo.vertex_buffer) {
        glDeleteBuffers(1, &pLwc->sea_route_vbo.vertex_buffer);
    }

    for (int i = 0; i < LMVT_COUNT; i++) {
        glDeleteBuffers(1, &pLwc->morph_vertex_buffer[i].vertex_buffer);
    }

    glDeleteTextures(1, &pLwc->shared_fbo.color_tex);
    glDeleteTextures(MAX_TEX_ATLAS, pLwc->tex_atlas);
    glDeleteTextures(MAX_TEX_FONT_ATLAS, pLwc->tex_font_atlas);
    glDeleteTextures(MAX_TEX_PROGRAMMED, pLwc->tex_programmed);

#if LW_SUPPORT_VAO
    glDeleteVertexArrays(VERTEX_BUFFER_COUNT, pLwc->vao);
    glDeleteVertexArrays(LFT_COUNT, pLwc->fvao);
    glDeleteVertexArrays(SKIN_VERTEX_BUFFER_COUNT, pLwc->skin_vao);
    glDeleteVertexArrays(FAN_VERTEX_BUFFER_COUNT, pLwc->fan_vao);
    glDeleteVertexArrays(PS_VERTEX_BUFFER_COUNT, pLwc->ps_vao);
    glDeleteVertexArrays(PS0_VERTEX_BUFFER_COUNT, pLwc->ps0_vao);
    glDeleteVertexArrays(LINE_VERTEX_BUFFER_COUNT, pLwc->line_vao);
#endif

    lw_delete_all_shader_program(pLwc);
    lw_delete_all_vertex_shader(pLwc);
    lw_delete_all_frag_shader(pLwc);

    for (int i = 0; i < LWAC_COUNT; i++) {
        unload_action(&pLwc->action[i]);
    }

    for (int i = 0; i < LWAR_COUNT; i++) {
        unload_armature(&pLwc->armature[i]);
    }

    deinit_mq(pLwc->mq);

    unload_field(pLwc->field);

    deinit_sys_msg(pLwc->def_sys_msg);

    deltatime_destroy(&pLwc->update_dt);

    delete_puck_game(&pLwc->puck_game);

    htmlui_destroy(&pLwc->htmlui);

    ps_destroy_context(&pLwc->ps_context);

    lwttl_destroy(&pLwc->ttl);

    remtex_destroy(&pLwc->remtex);

    free(pLwc->country_array.first);

    free(pLwc);
}

void lw_on_destroy(LWCONTEXT* pLwc) {
    LOGI("%s", __func__);
    lwttl_write_last_state(pLwc->ttl, pLwc);
    release_font(pLwc->pFnt);
    release_string(pLwc->dialog);
    deinit_net(pLwc);
    zactor_destroy((zactor_t**)&pLwc->logic_actor);
    lw_deinit(pLwc);
    mq_shutdown();
}

void lw_set_kp(LWCONTEXT* pLwc, int kp) {
    pLwc->kp = kp;
}

double lwcontext_delta_time(const LWCONTEXT* pLwc) {
    return deltatime_delta_time(pLwc->update_dt);
}

void lw_set_push_token(LWCONTEXT* pLwc, int domain, const char* token) {
    tcp_send_push_token(pLwc->tcp, 300, domain, token);
}

static char packageVersion[64];
void set_package_version(const char* package_version) {
    strcpy(packageVersion, package_version);
}
const char* package_version() {
    return packageVersion;
}

void write_user_data_file_string(const LWCONTEXT* pLwc, const char* filename, const char* str) {
    write_file_string(pLwc->user_data_path,
                      filename,
                      str);
}

void write_user_data_file_binary(const LWCONTEXT* pLwc, const char* filename, const unsigned char* dat, int dat_len) {
    write_file_binary(pLwc->user_data_path,
                      filename,
                      (const char*)dat,
                      (size_t)dat_len);
}

int read_user_data_file_string(const LWCONTEXT* pLwc, const char* filename, const char** str) {
    char* out;
    int ret_code = read_file_string_all(pLwc->user_data_path,
                                        filename,
                                        &out);
    *str = out;
    return ret_code;
}

int read_user_data_file_binary(const LWCONTEXT* pLwc, const char* filename, const unsigned char** dat, int* dat_len) {
    char* out;
    size_t out_len;
    int ret_code = read_file_binary_all(pLwc->user_data_path,
                                        filename,
                                        &out_len,
                                        &out);
    *dat = out;
    *dat_len = (int)out_len;
    return ret_code;
}

int srpwrap_user_delete(lua_State* L) {
    struct SRPUser *usr = (struct SRPUser *) 0;
    if (!SWIG_isptrtype(L, 1)) {
        LOGE("Not a swig pointer type!");
        return -1;
    }
    if (!SWIG_IsOK(SWIG_ConvertPtr(L, 1, (void**)&usr, SWIGTYPE_p_SRPUser, 0))) {
        LOGE("Not a SRPUser type!");
        return -2;
    }
    if (SWIG_Lua_class_is_own(L)) {
        SWIG_Lua_class_disown(L);
        srp_user_delete(usr);
    }
    return 0;
}

int srpwrap_verifier_delete(lua_State* L) {
    struct SRPVerifier *ver = (struct SRPVerifier *) 0;
    if (!SWIG_isptrtype(L, 1)) {
        LOGE("Not a swig pointer type!");
        return -1;
    }
    if (!SWIG_IsOK(SWIG_ConvertPtr(L, 1, (void**)&ver, SWIGTYPE_p_SRPVerifier, 0))) {
        LOGE("Not a SRPVerifier type!");
        return -2;
    }
    if (SWIG_Lua_class_is_own(L)) {
        SWIG_Lua_class_disown(L);
        srp_verifier_delete(ver);
    }
    return 0;
}

int lwasfwrap_delete(lua_State* L) {
    LWASF* asf = 0;
    if (!SWIG_isptrtype(L, 1)) {
        LOGE("Not a swig pointer type!");
        return -1;
    }
    if (!SWIG_IsOK(SWIG_ConvertPtr(L, 1, (void**)&asf, SWIGTYPE_p__LWASF, 0))) {
        LOGE("Not a LWASF type!");
        return -2;
    }
    if (SWIG_Lua_class_is_own(L)) {
        SWIG_Lua_class_disown(L);
        lwasf_delete(asf);
    }
    return 0;
}

int lwamcwrap_delete(lua_State* L) {
    LWAMC* amc = 0;
    if (!SWIG_isptrtype(L, 1)) {
        LOGE("Not a swig pointer type!");
        return -1;
    }
    if (!SWIG_IsOK(SWIG_ConvertPtr(L, 1, (void**)&amc, SWIGTYPE_p__LWAMC, 0))) {
        LOGE("Not a LWAMC type!");
        return -2;
    }
    if (SWIG_Lua_class_is_own(L)) {
        SWIG_Lua_class_disown(L);
        lwamc_delete(amc);
    }
    return 0;
}

int lwsgwrap_delete(lua_State* L) {
    LWSG* sg = 0;
    if (!SWIG_isptrtype(L, 1)) {
        LOGE("Not a swig pointer type!");
        return -1;
    }
    if (!SWIG_IsOK(SWIG_ConvertPtr(L, 1, (void**)&sg, SWIGTYPE_p__LWSG, 0))) {
        LOGE("Not a LWSG type!");
        return -2;
    }
    // delete reference to this 'sg' from pLwc.
    lua_getglobal(L, "pLwc");
    if (lua_islightuserdata(L, -1)) {
        LWCONTEXT* pLwc = lua_touserdata(L, -1);
        // Stop new frame of rendering
        lwcontext_set_safe_to_start_render(pLwc, 0);
        // Busy wait for current frame of rendering to be completed
        while (lwcontext_rendering(pLwc)) {}
        if (pLwc->sg == sg) {
            pLwc->sg = 0;
        }
        // Start rendering
        lwcontext_set_safe_to_start_render(pLwc, 1);
    }
    lua_pop(L, 1);
    // delete it
    if (SWIG_Lua_class_is_own(L)) {
        SWIG_Lua_class_disown(L);
        lwsg_delete(sg);
    }
    return 0;
}
