#include <string.h>
#include "render_ttl.h"
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
#include <assert.h>
#include "render_morph.h"
#include "platform_detection.h"
#include <stdlib.h>
#if LW_PLATFORM_IOS
#include <alloca.h>
#endif
#define MAX_VISIBILITY_ENTRY_COUNT (32)
#define MAX_VISIBILITY_ENTRY_NAME_LENGTH (32)
static char visibility[MAX_VISIBILITY_ENTRY_COUNT][MAX_VISIBILITY_ENTRY_NAME_LENGTH];

#define WATER_COLOR_R (0 / 255.f)
#define WATER_COLOR_G (94 / 255.f)
#define WATER_COLOR_B (190 / 255.f)

#define WAYPOINT_COLOR_R (0.2f)
#define WAYPOINT_COLOR_G (0.2f)
#define WAYPOINT_COLOR_B (0.7f)

#define LAND_WAYPOINT2_COLOR_R (147 / 255.0f)
#define LAND_WAYPOINT2_COLOR_G (143 / 255.0f)
#define LAND_WAYPOINT2_COLOR_B (134 / 255.0f)

#define WATER_WAYPOINT2_COLOR_R (96 / 255.0f)
#define WATER_WAYPOINT2_COLOR_G (180 / 255.0f)
#define WATER_WAYPOINT2_COLOR_B (230 / 255.0f)

#define CELL_DRAGGING_LINE_COLOR_R (0.7f)
#define CELL_DRAGGING_LINE_COLOR_G (0.9f)
#define CELL_DRAGGING_LINE_COLOR_B (0.7f)

#define UI_SCREEN_EDGE_MARGIN (0.01f)

typedef struct _LWTTLFIELDVIEWPORT LWTTLFIELDVIEWPORT;

void lwc_render_ttl_fbo_body(const LWCONTEXT* pLwc, const char* html_body) {
    glBindFramebuffer(GL_FRAMEBUFFER, pLwc->shared_fbo.fbo);
    glDisable(GL_DEPTH_TEST);

    glViewport(0, 0, pLwc->shared_fbo.width, pLwc->shared_fbo.height);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    htmlui_load_render_draw_body(pLwc->htmlui, html_body);

    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int lwc_prerender_ttl_fbo(const LWCONTEXT* pLwc) {
    if (pLwc->shared_fbo.fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, pLwc->shared_fbo.fbo);
        glDisable(GL_DEPTH_TEST);

        glViewport(0, 0, pLwc->shared_fbo.width, pLwc->shared_fbo.height);
        glClearColor(0, 0, 0, 0); // alpha should be cleared to zero
        //lw_clear_color();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        return 0;
    } else {
        // might awoke from sleep mode?
        return -1;
    }
}

void lwc_postrender_ttl_fbo(const LWCONTEXT* pLwc) {
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void lwc_render_ttl_fbo(const LWCONTEXT* pLwc, const char* html_path) {
    if (lwc_prerender_ttl_fbo(pLwc) == 0) {
        htmlui_load_render_draw(pLwc->htmlui, html_path);
        lwc_postrender_ttl_fbo(pLwc);
    }
}

typedef struct _LWWAVE {
    int valid;
    float y;
    float age;
} LWWAVE;
static LWWAVE wave[5];
float last_wave_spawn = 0;

static void render_port(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, float port_y) {
    int shader_index = LWST_DEFAULT_NORMAL_COLOR;
    const LWSHADER* shader = &pLwc->shader[shader_index];
    lazy_glUseProgram(pLwc, shader_index);
    mat4x4 rot;
    mat4x4_identity(rot);

    float sx = 1, sy = 1, sz = 1;
    float x = 0, y = port_y, z = 0;
    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_mul(model, model, rot);
    mat4x4_scale_aniso(model, model, sx, sy, sz);

    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, z);

    mat4x4_mul(model, model_translate, model);

    mat4x4 view_model;
    mat4x4_mul(view_model, view, model);

    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);

    const LW_VBO_TYPE lvt = LVT_PORT;
    lazy_glBindBuffer(pLwc, lvt);
    bind_all_color_vertex_attrib(pLwc, lvt);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model);
    glUniform1f(shader->vertex_color_ratio, 0);
    glUniform3f(shader->vertex_color, 0, 0, 0);
    //glShadeModel(GL_FLAT);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

static void render_sea_city(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj) {
    int shader_index = LWST_DEFAULT_NORMAL_COLOR;
    const LWSHADER* shader = &pLwc->shader[shader_index];
    lazy_glUseProgram(pLwc, shader_index);
    mat4x4 rot;
    mat4x4_identity(rot);

    float sx = 1, sy = 1, sz = 1;
    float x = 0, y = 25, z = 0;
    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_mul(model, model, rot);
    mat4x4_scale_aniso(model, model, sx, sy, sz);

    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, z);

    mat4x4_mul(model, model_translate, model);

    mat4x4 view_model;
    mat4x4_mul(view_model, view, model);

    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);

    const LW_VBO_TYPE lvt = LVT_SEA_CITY;
    lazy_glBindBuffer(pLwc, lvt);
    bind_all_color_vertex_attrib(pLwc, lvt);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model);
    glUniform1f(shader->vertex_color_ratio, 0);
    glUniform3f(shader->vertex_color, 0, 0, 0);
    //glShadeModel(GL_FLAT);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

static void render_vehicle(const LWCONTEXT* pLwc,
                           const LWTTLFIELDVIEWPORT* vp,
                           float x,
                           float y,
                           float z,
                           float rot_z,
                           int shader_index,
                           LW_VBO_TYPE lvt,
                           LW_ATLAS_ENUM lae,
                           float scale) {
    const LWSHADER* shader = &pLwc->shader[shader_index];
    lazy_glUseProgram(pLwc, shader_index);
    mat4x4 rot;
    mat4x4_identity(rot);
    mat4x4_rotate_Z(rot, rot, rot_z);
    const float sx = lwttl_vehicle_render_scale(pLwc->ttl, vp, scale);
    const float sy = sx;
    const float sz = sx;
    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_mul(model, model, rot);
    mat4x4_scale_aniso(model, model, sx, sy, sz);

    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, z);

    mat4x4_mul(model, model_translate, model);

    mat4x4 view_model;
    mat4x4_mul(view_model, lwttl_viewport_view(vp), model);

    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, lwttl_viewport_proj(vp), view_model);

    mat4x4 model_normal_transform;
    mat4x4_identity(model_normal_transform);

    lazy_glBindBuffer(pLwc, lvt);
    if (shader_index == LWST_DEFAULT) {
        bind_all_vertex_attrib(pLwc, lvt);
    } else if (shader_index == LWST_DEFAULT_NORMAL_COLOR) {
        bind_all_color_vertex_attrib(pLwc, lvt);
    } else {
        LOGEP("Not supported shader index: %d", shader_index);
    }
    if (lae != LAE_DONTCARE) {
        glActiveTexture(GL_TEXTURE0);
        lazy_tex_atlas_glBindTexture(pLwc, lae);
        set_tex_filter(GL_LINEAR, GL_LINEAR);
    }
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model_normal_transform);
    glUniform1f(shader->vertex_color_ratio, 0);
    glUniform3f(shader->vertex_color, 0, 0, 0);
    glUniform1i(shader->diffuse_location, 0); // 0 means GL_TEXTURE0
    glUniform1i(shader->alpha_only_location, 1); // 1 means GL_TEXTURE1
    glUniform1f(shader->overlay_color_ratio_location, 0);
    //glShadeModel(GL_FLAT);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

static void render_ship(const LWCONTEXT* pLwc, const LWTTLFIELDVIEWPORT* vp, float x, float y, float z, float rot_z) {
    render_vehicle(pLwc, vp, x, y, z, rot_z, LWST_DEFAULT_NORMAL_COLOR, LVT_SHIP, LAE_DONTCARE, 0.075f);
}

static void render_truck(const LWCONTEXT* pLwc, const LWTTLFIELDVIEWPORT* vp, float x, float y, float z, float rot_z) {
    render_vehicle(pLwc, vp, x, y, z, rot_z + (float)LWDEG2RAD(90), LWST_DEFAULT, LVT_OIL_TRUCK, LAE_3D_OIL_TRUCK_TEX_KTX, 0.2f);
}

static void render_terminal_icon(const LWCONTEXT* pLwc,
                                 const mat4x4 view,
                                 const mat4x4 proj,
                                 float x,
                                 float y,
                                 float z,
                                 float w,
                                 float h,
                                 LW_ATLAS_ENUM lae,
                                 LW_ATLAS_ENUM lae_alpha) {
    mat4x4 proj_view;
    mat4x4_identity(proj_view);
    mat4x4_mul(proj_view, proj, view);
    const vec4 obj_pos_vec4 = {
        x,
        y,
        z,
        1,
    };
    vec2 ui_point;
    calculate_ui_point_from_world_point(pLwc->viewport_aspect_ratio, proj_view, obj_pos_vec4, ui_point);
    mat4x4 identity_view;
    mat4x4_identity(identity_view);
    lazy_tex_atlas_glBindTexture(pLwc, lae);
    lazy_tex_atlas_glBindTexture(pLwc, lae_alpha);
    render_solid_vb_ui_alpha_uv_shader_view_proj(pLwc,
                                                 ui_point[0],
                                                 ui_point[1],
                                                 w * 0.075f,
                                                 h * 0.075f,
                                                 pLwc->tex_atlas[lae],
                                                 pLwc->tex_atlas[lae_alpha],
                                                 LVT_CENTER_BOTTOM_ANCHORED_SQUARE,
                                                 1.0f,
                                                 1.0f,
                                                 1.0f,
                                                 1.0f,
                                                 0.0f,
                                                 default_uv_offset,
                                                 default_uv_scale,
                                                 LWST_ETC1,
                                                 identity_view,
                                                 pLwc->proj);
}

static void render_seaport_icon(const LWCONTEXT* pLwc,
                                const mat4x4 view,
                                const mat4x4 proj,
                                float x,
                                float y,
                                float z,
                                float w,
                                float h) {
    render_terminal_icon(pLwc,
                         view,
                         proj,
                         x,
                         y,
                         z,
                         w,
                         h,
                         LAE_TTL_PORT,
                         LAE_TTL_PORT_ALPHA);
}

static void render_truck_terminal_icon(const LWCONTEXT* pLwc,
                                       const mat4x4 view,
                                       const mat4x4 proj,
                                       float x,
                                       float y,
                                       float z,
                                       float w,
                                       float h) {
    render_terminal_icon(pLwc,
                         view,
                         proj,
                         x,
                         y,
                         z,
                         w,
                         h,
                         LAE_TTL_CONTAINER_WHITE,
                         LAE_TTL_CONTAINER_WHITE_ALPHA);
}

static void render_city_icon(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, float x, float y, float z, float w, float h, unsigned char population_level) {
    mat4x4 proj_view;
    mat4x4_identity(proj_view);
    mat4x4_mul(proj_view, proj, view);
    const vec4 obj_pos_vec4 = {
        x,
        y,
        z,
        1,
    };
    vec2 ui_point;
    calculate_ui_point_from_world_point(pLwc->viewport_aspect_ratio, proj_view, obj_pos_vec4, ui_point);
    mat4x4 identity_view;
    mat4x4_identity(identity_view);
    lazy_tex_atlas_glBindTexture(pLwc, LAE_TTL_CITY);
    lazy_tex_atlas_glBindTexture(pLwc, LAE_TTL_CITY_ALPHA);
    render_solid_vb_ui_alpha_uv_shader_view_proj(pLwc,
                                                 ui_point[0],
                                                 ui_point[1],
                                                 w * (2 + (population_level >> 4)) * 0.05f,
                                                 h * (2 + (population_level >> 4)) * 0.05f,
                                                 pLwc->tex_atlas[LAE_TTL_CITY],
                                                 pLwc->tex_atlas[LAE_TTL_CITY_ALPHA],
                                                 LVT_CENTER_BOTTOM_ANCHORED_SQUARE,
                                                 1.0f,
                                                 1.0f,
                                                 1.0f,
                                                 1.0f,
                                                 0.0f,
                                                 default_uv_offset,
                                                 default_uv_scale,
                                                 LWST_ETC1,
                                                 identity_view,
                                                 pLwc->proj);
}

static void render_salvage_icon(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, float x, float y, float z, float w, float h) {
    mat4x4 proj_view;
    mat4x4_identity(proj_view);
    mat4x4_mul(proj_view, proj, view);
    const vec4 obj_pos_vec4 = {
        x,
        y,
        z,
        1,
    };
    vec2 ui_point;
    calculate_ui_point_from_world_point(pLwc->viewport_aspect_ratio, proj_view, obj_pos_vec4, ui_point);
    mat4x4 identity_view;
    mat4x4_identity(identity_view);
    lazy_tex_atlas_glBindTexture(pLwc, LAE_TTL_CONTAINER_ORANGE);
    lazy_tex_atlas_glBindTexture(pLwc, LAE_TTL_CONTAINER_ORANGE_ALPHA);
    render_solid_vb_ui_alpha_uv_shader_view_proj(pLwc,
                                                 ui_point[0],
                                                 ui_point[1],
                                                 w * 0.075f,
                                                 h * 0.075f,
                                                 pLwc->tex_atlas[LAE_TTL_CONTAINER_ORANGE],
                                                 pLwc->tex_atlas[LAE_TTL_CONTAINER_ORANGE_ALPHA],
                                                 LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                                 1.0f,
                                                 1.0f,
                                                 1.0f,
                                                 1.0f,
                                                 0.0f,
                                                 default_uv_offset,
                                                 default_uv_scale,
                                                 LWST_ETC1,
                                                 identity_view,
                                                 pLwc->proj);
}

static void render_seaport_icon_xxx(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, float x, float y, float z, float w, float h) {
    int shader_index = LWST_DEFAULT_NORMAL_COLOR;
    const LWSHADER* shader = &pLwc->shader[shader_index];
    lazy_glUseProgram(pLwc, shader_index);
    mat4x4 rot;
    mat4x4_identity(rot);

    mat4x4 model_normal_transform;
    mat4x4_identity(model_normal_transform);

    float sx = w, sy = h, sz = 1;
    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_mul(model, model, rot);
    mat4x4_scale_aniso(model, model, sx, sy, sz);

    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, z);

    mat4x4_mul(model, model_translate, model);

    mat4x4 view_model;
    mat4x4_mul(view_model, view, model);

    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);

    const LW_VBO_TYPE lvt = LVT_SEAPORT_ICON;
    lazy_glBindBuffer(pLwc, lvt);
    bind_all_color_vertex_attrib(pLwc, lvt);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model_normal_transform);
    glUniform1f(shader->vertex_color_ratio, 0);
    glUniform3f(shader->vertex_color, 0, 0, 0);
    //glShadeModel(GL_FLAT);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

static void render_cell_color(const LWCONTEXT* pLwc,
                              const mat4x4 view,
                              const mat4x4 proj,
                              float x,
                              float y,
                              float z,
                              float w,
                              float h,
                              float d,
                              LW_VBO_TYPE lvt,
                              float vertex_color_ratio,
                              float vertex_color_r,
                              float vertex_color_g,
                              float vertex_color_b) {
    int shader_index = LWST_DEFAULT_NORMAL_COLOR;
    const LWSHADER* shader = &pLwc->shader[shader_index];
    lazy_glUseProgram(pLwc, shader_index);
    mat4x4 rot;
    mat4x4_identity(rot);

    mat4x4 model_normal_transform;
    mat4x4_identity(model_normal_transform);

    float sx = w, sy = h, sz = d;
    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_mul(model, model, rot);
    mat4x4_scale_aniso(model, model, sx, sy, sz);

    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, z);

    mat4x4_mul(model, model_translate, model);

    mat4x4 view_model;
    mat4x4_mul(view_model, view, model);

    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);

    lazy_glBindBuffer(pLwc, lvt);
    bind_all_color_vertex_attrib(pLwc, lvt);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model_normal_transform);
    glUniform1f(shader->vertex_color_ratio, vertex_color_ratio);
    glUniform3f(shader->vertex_color, vertex_color_r, vertex_color_g, vertex_color_b);
    //glShadeModel(GL_FLAT);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}


static void render_cell(const LWCONTEXT* pLwc,
                        const mat4x4 view,
                        const mat4x4 proj,
                        float x,
                        float y,
                        float z,
                        float w,
                        float h,
                        LW_VBO_TYPE lvt) {
    render_cell_color(pLwc,
                      view,
                      proj,
                      x,
                      y,
                      z,
                      w,
                      h,
                      1.0f,
                      lvt,
                      0,
                      0,
                      0,
                      0);
}

static void render_land_cell(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, float x, float y, float z, float w, float h) {
    const LW_VBO_TYPE lvt = LVT_LAND_CELL;
    render_cell(pLwc, view, proj, x, y, z, w, h, lvt);
}

static int bitmap_land(const unsigned char bitmap[LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT_WITH_MARGIN*LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS][LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG_WITH_MARGIN*LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS],
                       size_t bx,
                       size_t by) {
    if (by >= LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT_WITH_MARGIN * LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS) {
        return 0;
    }
    if (bx >= LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG_WITH_MARGIN * LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS) {
        return 0;
    }
    return bitmap[by][bx];
}

#define TILEMAP_GAP (0) //(0.005f)
#define TILEMAP_TILE_COUNT (4)

static const float tilemap_uv_offset[16][2] = {
    { 1.0f / TILEMAP_TILE_COUNT * 2 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT * 3 + TILEMAP_GAP }, // 0000 [all water]
    { 1.0f / TILEMAP_TILE_COUNT * 0 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT * 0 + TILEMAP_GAP }, // 0001
    { 1.0f / TILEMAP_TILE_COUNT * 2 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT * 0 + TILEMAP_GAP }, // 0010
    { 1.0f / TILEMAP_TILE_COUNT * 1 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT * 0 + TILEMAP_GAP }, // 0011
    { 1.0f / TILEMAP_TILE_COUNT * 0 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT * 2 + TILEMAP_GAP }, // 0100
    { 1.0f / TILEMAP_TILE_COUNT * 0 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT * 1 + TILEMAP_GAP }, // 0101
    { 1.0f / TILEMAP_TILE_COUNT * 1 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT * 3 + TILEMAP_GAP }, // 0110
    { 1.0f / TILEMAP_TILE_COUNT * 3 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT * 3 + TILEMAP_GAP }, // 0111
    { 1.0f / TILEMAP_TILE_COUNT * 2 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT * 2 + TILEMAP_GAP }, // 1000
    { 1.0f / TILEMAP_TILE_COUNT * 0 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT * 3 + TILEMAP_GAP }, // 1001
    { 1.0f / TILEMAP_TILE_COUNT * 2 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT * 1 + TILEMAP_GAP }, // 1010
    { 1.0f / TILEMAP_TILE_COUNT * 3 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT * 1 + TILEMAP_GAP }, // 1011
    { 1.0f / TILEMAP_TILE_COUNT * 1 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT * 2 + TILEMAP_GAP }, // 1100
    { 1.0f / TILEMAP_TILE_COUNT * 3 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT * 2 + TILEMAP_GAP }, // 1101
    { 1.0f / TILEMAP_TILE_COUNT * 3 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT * 0 + TILEMAP_GAP }, // 1110
    { 1.0f / TILEMAP_TILE_COUNT * 1 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT * 1 + TILEMAP_GAP }, // 1111 [all land]
};

static const float tilemap_uv_scale[] = {
    (1.0f - 2 * TILEMAP_TILE_COUNT * TILEMAP_GAP) / TILEMAP_TILE_COUNT,
    (1.0f - 2 * TILEMAP_TILE_COUNT * TILEMAP_GAP) / TILEMAP_TILE_COUNT
};

static void render_land_cell_bitmap(const LWTTL* ttl,
                                    const LWCONTEXT* pLwc,
                                    const LWTTLFIELDVIEWPORT* vp,
                                    const int bound_xc0,
                                    const int bound_yc0,
                                    const unsigned char bitmap[LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT_WITH_MARGIN*LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS][LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG_WITH_MARGIN*LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS]) {
    const float tw = 1.0f / 2;
    const float th = 1.0f / 2;

    LW_ATLAS_ENUM tile_lae = LAE_WATER_SAND_TILE;
    const int clamped_view_scale = lwttl_viewport_clamped_view_scale(vp);
    if (lwttl_cell_grid(ttl)) {
        if (clamped_view_scale == 1) {
            tile_lae = LAE_WATER_SAND_TILE_GRID_1X;
        } else if (clamped_view_scale == 2) {
            tile_lae = LAE_WATER_SAND_TILE_GRID_2X;
        } else if (clamped_view_scale == 4) {
            tile_lae = LAE_WATER_SAND_TILE_GRID_4X;
        } else if (clamped_view_scale == 8) {
            tile_lae = LAE_WATER_SAND_TILE_GRID_8X;
        } else if (clamped_view_scale == 16) {
            tile_lae = LAE_WATER_SAND_TILE_GRID_16X;
        } else {
            tile_lae = LAE_WATER_SAND_TILE;
        }
    }

    lazy_tex_atlas_glBindTexture(pLwc, tile_lae);
    for (int by = 0; by < LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT_WITH_MARGIN*LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS; by++) {
        for (int bx = 0; bx < LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG_WITH_MARGIN*LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS; bx++) {
            float x0, y0;
            float cell_x0, cell_y0, cell_z0;
            float cell_w, cell_h;
            if (lwttl_viewport_cell_render_info(vp,
                                                bound_xc0,
                                                bound_yc0,
                                                bx,
                                                by,
                                                &x0,
                                                &y0,
                                                &cell_x0,
                                                &cell_y0,
                                                &cell_z0,
                                                &cell_w,
                                                &cell_h) != 0) {
                continue;
            }
            int dx0, dy0;
            LW_VBO_TYPE lvt = LVT_LEFT_TOP_ANCHORED_SQUARE;
            if (lwttl_is_selected_cell_diff(ttl, (int)x0, (int)y0, &dx0, &dy0)) {
                if (dx0 == 0 && dy0 == -1) {
                    lvt = LVT_TILE_SEL_0_1;
                } else if (dx0 == -1 && dy0 == 0) {
                    lvt = LVT_TILE_SEL_1_0;
                } else if (dx0 == -1 && dy0 == -1) {
                    lvt = LVT_TILE_SEL_1_1;
                } else if (dx0 == 0 && dy0 == 0) {
                    lvt = LVT_TILE_SEL_0_0;
                }
            }
            const int uv_offset_index =
                bitmap_land(bitmap, bx - 1, by - 1) << 3
                | bitmap_land(bitmap, bx - 0, by - 1) << 2
                | bitmap_land(bitmap, bx - 1, by - 0) << 1
                | bitmap_land(bitmap, bx - 0, by - 0) << 0;
            const float sx = cell_w / 2;
            const float sy = cell_h / 2;
            const float sz = lwttl_selected_cell_popup_height(pLwc->ttl, vp) / clamped_view_scale;
            render_solid_vb_uv_shader_rot_view_proj(pLwc,
                                                    cell_x0,
                                                    cell_y0,
                                                    cell_z0,
                                                    sx,
                                                    sy,
                                                    sz,
                                                    pLwc->tex_atlas[tile_lae],
                                                    lvt,
                                                    1.0f,
                                                    0,
                                                    0,
                                                    0,
                                                    0,
                                                    tilemap_uv_offset[uv_offset_index],
                                                    tilemap_uv_scale,
                                                    LWST_DEFAULT,
                                                    0,
                                                    lwttl_viewport_view(vp),
                                                    lwttl_viewport_proj(vp));
        }
    }
}

static void render_sea_cell_debug(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, float x, float y, float z, float w, float h) {
    const LW_VBO_TYPE lvt = LVT_SEA_CELL_DEBUG;
    render_cell(pLwc, view, proj, x, y, z, w, h, lvt);
}


static void render_timer(const LWCONTEXT* pLwc,
                         const mat4x4 view,
                         const mat4x4 proj,
                         const float x,
                         const float y,
                         const float w,
                         const float h,
                         const float remain_sec,
                         const float total_sec,
                         const float ui_alpha) {
    float remain_ratio = 1.0f;
    if (total_sec != 0) {
        remain_ratio = remain_sec / total_sec;
    }
    const LWSHADER* shader = &pLwc->shader[LWST_RINGGAUGE];
    lazy_glUseProgram(pLwc, LWST_RINGGAUGE);
    glUniform3f(shader->full_color, 0, 1, 0);
    glUniform3f(shader->empty_color, 1, 0, 0);
    glUniform1f(shader->gauge_ratio, remain_ratio);
    // text origin point debug indicator
    render_solid_vb_ui_uv_shader_rot_view_proj(pLwc,
                                               x,
                                               y,
                                               w,
                                               h,
                                               0,//pLwc->tex_atlas[LVT_RINGGAUGE],
                                               LVT_RINGGAUGETHICK,
                                               ui_alpha,
                                               0,
                                               1,
                                               0,
                                               1,
                                               default_uv_offset,
                                               default_uv_scale,
                                               LWST_RINGGAUGE,
                                               (float)M_PI,
                                               view,
                                               proj);
}

static void render_cell_pixel_selector(const LWTTL* ttl,
                                       const LWCONTEXT* pLwc,
                                       const mat4x4 view,
                                       const mat4x4 proj,
                                       const float x,
                                       const float y,
                                       const float z,
                                       const float w,
                                       const float h,
                                       const float d) {
    const LW_VBO_TYPE lvt = LVT_CELL_PIXEL_SELECTOR;
    render_cell_color(pLwc,
                      view,
                      proj,
                      x,
                      y,
                      z,
                      w / 2,
                      h / 2,
                      d,
                      lvt,
                      0.0f,
                      0.0f,
                      0.0f,
                      0.0f);
    float press_menu_gauge_current;
    float press_menu_gauge_total;
    float app_time = (float)pLwc->app_time;
    if (lwttl_press_ring_info(ttl,
                              app_time,
                              &press_menu_gauge_current,
                              &press_menu_gauge_total)) {
        render_timer(pLwc,
                     view,
                     proj,
                     x,
                     y + 2.5f,
                     w / 4,
                     h / 4,
                     press_menu_gauge_current,
                     press_menu_gauge_total,
                     1.0f);
    }
}

static void render_waves(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, float ship_y) {
    if (last_wave_spawn + 1.1f < (float)pLwc->app_time) {
        for (int i = 0; i < ARRAY_SIZE(wave); i++) {
            if (wave[i].valid == 0) {
                last_wave_spawn = (float)pLwc->app_time;
                wave[i].age = 0;
                wave[i].y = -5.5f + ship_y;
                wave[i].valid = 1;
                break;
            }
        }
    }
    float delta_time = (float)deltatime_delta_time(pLwc->render_dt);
    for (int i = 0; i < ARRAY_SIZE(wave); i++) {
        if (wave[i].valid) {
            float alpha = LWMAX(0, sinf(wave[i].age * ((float)M_PI / 2.0f)));
            lw_load_tex(pLwc, LAE_WAVE);
            lw_load_tex(pLwc, LAE_WAVE_ALPHA);
            render_solid_vb_ui_alpha_uv_shader_view_proj(pLwc,
                                                         0,
                                                         wave[i].y,
                                                         3.0f + 0.5f * alpha,
                                                         2.5f,
                                                         LAE_WAVE,
                                                         LAE_WAVE_ALPHA,
                                                         LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                                         alpha,
                                                         1.0f,
                                                         1.0f,
                                                         1.0f,
                                                         1.0f,
                                                         default_uv_offset,
                                                         default_uv_scale,
                                                         LWST_ETC1,
                                                         view,
                                                         proj);
            wave[i].y -= delta_time * 1.5f;
            wave[i].age += delta_time;
            if (wave[i].age > 4.0f) {
                wave[i].valid = 0;
            }
        }
    }
}

static void render_morphed_earth(const LWCONTEXT* pLwc,
                                 const LWTTLFIELDVIEWPORT* vp) {
    const LW_MORPH_VBO_TYPE lmvt = LMVT_EARTH;
    const LW_ATLAS_ENUM lae = LAE_WATER_2048_1024_AA;
    const int tex_index = pLwc->tex_atlas[lae];
    // original planar earth mesh size
    const float earth_planar_mesh_height = 3.0f;
    const float earth_globe_scale = LNGLAT_RES_HEIGHT / earth_planar_mesh_height / lwttl_viewport_view_scale(vp);
    const float earth_globe_morph_weight = 0;// lwttl_earth_globe_morph_weight(earth_globe_scale);
    const float x = 0.0f;
    const LWTTLLNGLAT* view_center = lwttl_center(pLwc->ttl);
    const float y = lwttl_earth_globe_y(view_center, earth_globe_scale);
    lazy_glBindBuffer(pLwc, lmvt);
    lazy_tex_atlas_glBindTexture(pLwc, lae);
    mat4x4 model_translate;
    mat4x4 model;
    mat4x4 model_scale;
    mat4x4_identity(model_scale);
    mat4x4_scale_aniso(model_scale, model_scale, earth_globe_scale, earth_globe_scale, earth_globe_scale);
    mat4x4_translate(model_translate, x, y, 0);
    mat4x4_identity(model);
    mat4x4_mul(model, model_translate, model_scale);
    const float uv_offset[] = {
        +(view_center->lng) / 360.0f,
        0.0f,
    };
    render_morph(pLwc,
                 tex_index,
                 lmvt,
                 1.0f,
                 1.0f,
                 1.0f,
                 1.0f,
                 0.0f,
                 lwttl_viewport_proj(vp),
                 lwttl_viewport_view(vp),
                 model,
                 uv_offset,
                 default_flip_y_uv_scale,
                 earth_globe_morph_weight);
}

static void render_earth(const LWCONTEXT* pLwc, const LWTTLLNGLAT* center, int view_scale) {
    const LW_VBO_TYPE lvt = LVT_EARTH;
    const LW_ATLAS_ENUM lae = LAE_WATER_2048_1024_AA;
    const float scale = 2.5f;
    const float x = -(pLwc->viewport_rt_x - 0.5f);
    const float y = pLwc->viewport_rt_y - 0.5f;
    const float alpha_multiplier = 0.5f;
    lazy_glBindBuffer(pLwc, lvt);
    lazy_tex_atlas_glBindTexture(pLwc, lae);

    const int shader_index = LWST_DEFAULT;
    lazy_glUseProgram(pLwc, shader_index);
    glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, default_uv_offset);
    glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, default_uv_scale);
    glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, alpha_multiplier);
    glUniform1i(pLwc->shader[shader_index].diffuse_location, 0); // 0 means GL_TEXTURE0
    glUniform1i(pLwc->shader[shader_index].alpha_only_location, 1); // 1 means GL_TEXTURE1
    glUniform3f(pLwc->shader[shader_index].overlay_color_location, 0, 0, 0);
    glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0);
    glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)pLwc->proj);

    const int tex_index = pLwc->tex_atlas[lae];
    mat4x4 model_translate;
    mat4x4 model;
    mat4x4 view_model;
    mat4x4 proj_view_model;
    mat4x4 model_scale;
    mat4x4 model_scale_rotate_1;
    mat4x4 model_scale_rotate;
    mat4x4 view_identity;
    mat4x4_identity(view_identity);
    mat4x4_identity(model_scale);
    mat4x4_scale_aniso(model_scale, model_scale, scale, scale, 0.0f);
    mat4x4_rotate_X(model_scale_rotate_1, model_scale, (float)LWDEG2RAD(center->lat));
    mat4x4_rotate_Y(model_scale_rotate, model_scale_rotate_1, (float)LWDEG2RAD(center->lng));
    mat4x4_translate(model_translate, x, y, 0);
    mat4x4_identity(model);
    mat4x4_mul(model, model_translate, model_scale_rotate);
    mat4x4_mul(view_model, view_identity, model);
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, pLwc->proj, view_model);

    lazy_glBindBuffer(pLwc, lvt);
    bind_all_vertex_attrib(pLwc, lvt);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_index);
    //assert(tex_index);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
    //set_tex_filter(GL_NEAREST, GL_NEAREST);
    glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);

    const float half_lng_extent_in_deg = lwttl_half_lng_extent_in_degrees(view_scale);
    const float half_lat_extent_in_deg = lwttl_half_lat_extent_in_degrees(view_scale);
    const float lng_min = center->lng - half_lng_extent_in_deg;
    const float lng_max = center->lng + half_lng_extent_in_deg;
    const float lat_min = center->lat - half_lat_extent_in_deg;
    const float lat_max = center->lat + half_lat_extent_in_deg;
    const float lng_extent = lng_max - lng_min;
    const float lat_extent = lat_max - lat_min;
    // current view window indicator
    render_solid_vb_ui_flip_y_uv(pLwc,
                                 x,
                                 y,
                                 scale * 2 * sinf((float)LWDEG2RAD(lng_extent) / 2),
                                 scale * 2 * sinf((float)LWDEG2RAD(lat_extent) / 2),
                                 pLwc->tex_atlas[LAE_ZERO_FOR_BLACK],
                                 LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                 0.5f,
                                 0.0f,
                                 1.0f,
                                 0.2f,
                                 1.0f,
                                 0);
}

static float distance_xy(const int ax,
                         const int ay,
                         const int bx,
                         const int by) {
    return sqrtf((float)((ax - bx) * (ax - bx) + (ay - by) * (ay - by)));
}

float float_value_with_stride(const float* a, size_t stride, int i);

int std_lower_bound_float(const float* a,
                          int first,
                          const int last,
                          const float value) {
    int count = last - first;
    int it;
    int step;
    while (count > 0) {
        it = first;
        step = count / 2;
        it += step;
        if (a[it] < value) {
            first = ++it;
            count -= step + 1;
        } else {
            count = step;
        }
    }
    return first;
}

static void pos_from_waypoints(const LWPTTLWAYPOINTS* wp,
                               const float param,
                               const int reversed,
                               float* px,
                               float* py,
                               float* dx,
                               float* dy) {
    *px = 0;
    *py = 0;
    *dx = 0;
    *dy = 0;
    if (wp->count < 2) {
        LOGEP("wp count less than 2");
    } else {
        float* accum_distance = alloca(sizeof(float) * wp->count);
        size_t accum_distance_cursor = 0;
        float dist = 0;
        accum_distance[accum_distance_cursor++] = dist;
        for (size_t i = 0; i < (size_t)wp->count - 1; i++) {
            dist += distance_xy(wp->waypoints[i + 0].x,
                                wp->waypoints[i + 0].y,
                                wp->waypoints[i + 1].x,
                                wp->waypoints[i + 1].y);
            accum_distance[accum_distance_cursor++] = dist;
        }
        if (accum_distance_cursor == 0) {
            return;
        }
        if (reversed) {
            //param = accum_distance[accum_distance_cursor - 1] - param;
        }
        int it_idx = std_lower_bound_float(accum_distance, 0, accum_distance_cursor, param);
        if (it_idx == 0) {
            *px = (float)wp->waypoints[0].x;
            *py = (float)wp->waypoints[0].y;
            *dx = (float)wp->waypoints[1].x - *px;
            *dy = (float)wp->waypoints[1].y - *py;
        } else if (it_idx == accum_distance_cursor) {
            *px = (float)wp->waypoints[wp->count - 1].x;
            *py = (float)wp->waypoints[wp->count - 1].y;
            *dx = *px - (float)wp->waypoints[wp->count - 2].x;
            *dy = *py - (float)wp->waypoints[wp->count - 2].y;
        } else {
            const xy32* wp1 = &wp->waypoints[it_idx - 1];
            const xy32* wp2 = &wp->waypoints[it_idx];
            float d1 = accum_distance[it_idx - 1];
            float d2 = accum_distance[it_idx];
            float r = (param - d1) / (d2 - d1);
            if (r < 0) r = 0;
            if (r > 1) r = 1;
            *dx = (float)(wp2->x - wp1->x);
            *dy = (float)(wp2->y - wp1->y);
            *px = wp1->x + *dx * r;
            *py = wp1->y + *dy * r;
        }
    }
}

static void render_sea_objects_nameplate(const LWCONTEXT* pLwc, const LWTTLFIELDVIEWPORT* vp) {
    mat4x4 proj_view;
    mat4x4_identity(proj_view);
    mat4x4_mul(proj_view,
               lwttl_viewport_proj(vp),
               lwttl_viewport_view(vp));
    const LWPTTLROUTESTATE* ttl_dynamic_state = lwttl_full_state(pLwc->ttl);
    const int view_scale = lwttl_viewport_view_scale(vp);
    for (int i = 0; i < ttl_dynamic_state->count; i++) {
        const LWPTTLWAYPOINTS* wp = lwttl_get_waypoints_by_ship_id(pLwc->ttl,
                                                                   ttl_dynamic_state->obj[i].db_id);
        if (wp == 0) {
            continue;
        }
        float px, py, dx, dy;
        pos_from_waypoints(wp,
                           ttl_dynamic_state->obj[i].route_param,
                           ttl_dynamic_state->obj[i].route_flags.reversed,
                           &px,
                           &py,
                           &dx,
                           &dy);
        const float rx = cell_fx_to_render_coords_vp(px + 0.5f, vp);
        const float ry = cell_fy_to_render_coords_vp(py + 0.5f, vp);
        vec4 obj_pos_vec4 = {
            rx,
            ry,
            0,
            1,
        };
        vec2 ui_point;
        calculate_ui_point_from_world_point(lwttl_viewport_aspect_ratio(vp), proj_view, obj_pos_vec4, ui_point);
        LWTEXTBLOCK test_text_block;
        test_text_block.text_block_width = 999.0f;
        test_text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
        test_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_F;
        char obj_nameplate[256];
        obj_nameplate[0] = 0;
        const char* route_state = lwttl_route_state(&ttl_dynamic_state->obj[i]);
        if (view_scale <= 4) {
            if (ttl_dynamic_state->obj[i].route_flags.breakdown) {
                sprintf(obj_nameplate,
                        "%s%s",
                        u8"⊠",
                        route_state);
            } else if (ttl_dynamic_state->obj[i].route_flags.sailing) {
                sprintf(obj_nameplate,
                        "%s%.0f",
                        u8"⏅",
                        ttl_dynamic_state->obj[i].route_param);
            } else {
                sprintf(obj_nameplate,
                        "%s%s",
                        u8"⏅",
                        route_state);
            }
        } else if (view_scale <= 32) {
            if (ttl_dynamic_state->obj[i].route_flags.breakdown) {
                sprintf(obj_nameplate,
                        "%s",
                        u8"⊠");
            } else if (ttl_dynamic_state->obj[i].route_flags.sailing) {
                sprintf(obj_nameplate,
                        "%s",
                        u8"⏅");
            } else {
                sprintf(obj_nameplate,
                        "%s",
                        u8"⏅");
            }
        }

        test_text_block.text = obj_nameplate;
        test_text_block.text_bytelen = (int)strlen(test_text_block.text);
        test_text_block.begin_index = 0;
        test_text_block.end_index = test_text_block.text_bytelen;
        test_text_block.multiline = 1;
        test_text_block.text_block_x = ui_point[0];
        test_text_block.text_block_y = ui_point[1];
        test_text_block.align = LTBA_LEFT_TOP;
        render_text_block_two_pass(pLwc, &test_text_block);
    }
}

static void render_sea_objects(const LWCONTEXT* pLwc, const LWTTLFIELDVIEWPORT* vp) {
    const LWPTTLROUTESTATE* ttl_dynamic_state = lwttl_full_state(pLwc->ttl);
    for (int i = 0; i < ttl_dynamic_state->count; i++) {
        const LWPTTLROUTEOBJECT* obj = &ttl_dynamic_state->obj[i];
        const LWPTTLWAYPOINTS* wp = lwttl_get_waypoints_by_ship_id(pLwc->ttl, obj->db_id);
        if (wp == 0) {
            continue;
        }
        float px, py, dx, dy;
        pos_from_waypoints(wp,
                           obj->route_param,
                           obj->route_flags.reversed,
                           &px,
                           &py,
                           &dx,
                           &dy);
        const float rx = cell_fx_to_render_coords_vp(px + 0.5f, vp);
        const float ry = cell_fy_to_render_coords_vp(py + 0.5f, vp);
        const float rot_z = atan2f(-dy, dx) + (obj->route_flags.reversed ? (-1) : (+1)) * (-(float)(M_PI / 2));
        if (obj->route_flags.land == 0) {
            render_ship(pLwc,
                        vp,
                        rx,
                        ry,
                        0,
                        rot_z);
        } else {
            render_truck(pLwc,
                         vp,
                         rx,
                         ry,
                         0,
                         rot_z);
        }
    }
}

static void render_waypoint_line_segment(const LWTTL* ttl,
                                         const LWCONTEXT* pLwc,
                                         const LWTTLFIELDVIEWPORT* vp,
                                         const int x0,
                                         const int y0,
                                         const int x1,
                                         const int y1,
                                         const float over_r,
                                         const float over_g,
                                         const float over_b) {
    if (x0 == x1 && y0 == y1) {
        return;
    }
    const float lng0_not_clamped = cell_fx_to_lng(x0 + 0.5f);
    const float lat0_not_clamped = cell_fy_to_lat(y0 + 0.5f);
    const float lng1_not_clamped = cell_fx_to_lng(x1 + 0.5f);
    const float lat1_not_clamped = cell_fy_to_lat(y1 + 0.5f);

    const float cell_x0 = lng_to_render_coords(lng0_not_clamped, vp);
    const float cell_y0 = lat_to_render_coords(lat0_not_clamped, vp);
    const float cell_x1 = lng_to_render_coords(lng1_not_clamped, vp);
    const float cell_y1 = lat_to_render_coords(lat1_not_clamped, vp);

    const float dx = cell_x1 - cell_x0;
    const float dy = cell_y1 - cell_y0;
    const float dlen = sqrtf(dx * dx + dy * dy);

    const float thickness = lwttl_viewport_waypoint_line_segment_thickness(vp);
    const float rot_z = atan2f(dy, dx);

    render_solid_vb_ui_uv_shader_rot_view_proj(pLwc,
                                               cell_x0,
                                               cell_y0,
                                               dlen,
                                               thickness,
                                               pLwc->tex_atlas[LAE_ZERO_FOR_BLACK],
                                               LVT_LEFT_CENTER_ANCHORED_SQUARE,
                                               1.0f,
                                               over_r,
                                               over_g,
                                               over_b,
                                               1.0f,
                                               default_uv_offset,
                                               default_uv_scale,
                                               LWST_DEFAULT,
                                               rot_z,
                                               lwttl_viewport_view(vp),
                                               lwttl_viewport_proj(vp));
}

static void render_waypoints(const LWTTL* ttl, const LWCONTEXT* pLwc, const LWTTLFIELDVIEWPORT* vp) {
    const LWPTTLWAYPOINTS* waypoints = lwttl_get_waypoints(ttl);
    for (int i = 0; i < waypoints->count - 1; i++) {
        render_waypoint_line_segment(ttl,
                                     pLwc,
                                     vp,
                                     waypoints->waypoints[i + 0].x,
                                     waypoints->waypoints[i + 0].y,
                                     waypoints->waypoints[i + 1].x,
                                     waypoints->waypoints[i + 1].y,
                                     WAYPOINT_COLOR_R,
                                     WAYPOINT_COLOR_G,
                                     WAYPOINT_COLOR_B);
    }
}

static void render_waypoints_by_ship_id(const LWTTL* ttl,
                                        const LWCONTEXT* pLwc,
                                        const LWTTLFIELDVIEWPORT* vp,
                                        int ship_id) {
    const LWPTTLWAYPOINTS* waypoints = lwttl_get_waypoints_by_ship_id(ttl, ship_id);
    if (waypoints) {
        for (int i = 0; i < waypoints->count - 1; i++) {
            render_waypoint_line_segment(ttl,
                                         pLwc,
                                         vp,
                                         waypoints->waypoints[i + 0].x,
                                         waypoints->waypoints[i + 0].y,
                                         waypoints->waypoints[i + 1].x,
                                         waypoints->waypoints[i + 1].y,
                                         waypoints->flags.land ? LAND_WAYPOINT2_COLOR_R : WATER_WAYPOINT2_COLOR_R,
                                         waypoints->flags.land ? LAND_WAYPOINT2_COLOR_G : WATER_WAYPOINT2_COLOR_G,
                                         waypoints->flags.land ? LAND_WAYPOINT2_COLOR_B : WATER_WAYPOINT2_COLOR_B);
        }
    }
}

static void render_waypoints_cache(const LWTTL* ttl, const LWCONTEXT* pLwc, const LWTTLFIELDVIEWPORT* vp) {
    const LWPTTLROUTESTATE* ttl_dynamic_state = lwttl_full_state(pLwc->ttl);
    for (int i = 0; i < ttl_dynamic_state->count; i++) {
        const int ship_id = ttl_dynamic_state->obj[i].db_id;
        render_waypoints_by_ship_id(pLwc->ttl,
                                    pLwc,
                                    vp,
                                    ship_id);
    }
}

static void render_seaports(const LWCONTEXT* pLwc,
                            const LWTTLFIELDVIEWPORT* vp) {
    const float icon_width = lwttl_viewport_icon_width(vp);
    const float icon_height = lwttl_viewport_icon_height(vp);

    int chunk_index_array[LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG_WITH_MARGIN*LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT_WITH_MARGIN];
    int bound_xcc0, bound_ycc0, bound_xcc1, bound_ycc1;
    const int chunk_index_array_count = lwttl_query_chunk_range_seaport_vp(pLwc->ttl,
                                                                           vp,
                                                                           chunk_index_array,
                                                                           ARRAY_SIZE(chunk_index_array),
                                                                           &bound_xcc0,
                                                                           &bound_ycc0,
                                                                           &bound_xcc1,
                                                                           &bound_ycc1);
    if (chunk_index_array_count > ARRAY_SIZE(chunk_index_array)) {
        LOGEP("incorrect query result");
        assert(0);
    }
    if (bound_xcc1 - bound_xcc0 > LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG_WITH_MARGIN) {
        LOGEP("incorrect query result");
        assert(0);
    }
    if (bound_ycc1 - bound_ycc0 > LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT_WITH_MARGIN) {
        LOGEP("incorrect query result");
        assert(0);
    }
    const int chunk_index_max = LWMIN(chunk_index_array_count, ARRAY_SIZE(chunk_index_array));
    for (int ci = 0; ci < chunk_index_max; ci++) {
        int obj_count = 0;
        int xc0 = 0;
        int yc0 = 0;
        const LWPTTLSEAPORTOBJECT* obj_begin = lwttl_query_chunk_seaport(pLwc->ttl,
                                                                         chunk_index_array[ci],
                                                                         &xc0,
                                                                         &yc0,
                                                                         &obj_count);
        if (obj_begin && obj_count > 0) {
            for (int i = 0; i < obj_count; i++) {
                float cell_x0, cell_y0, cell_z0;
                if (lwttl_viewport_icon_render_info(pLwc->ttl,
                                                    vp,
                                                    xc0,
                                                    obj_begin[i].x_scaled_offset_0,
                                                    yc0,
                                                    obj_begin[i].y_scaled_offset_0,
                                                    &cell_x0,
                                                    &cell_y0,
                                                    &cell_z0) != 0) {
                    continue;
                }
                if (obj_begin[i].flags.land == 0) {
                    render_seaport_icon(pLwc,
                                        lwttl_viewport_view(vp),
                                        lwttl_viewport_proj(vp),
                                        cell_x0,
                                        cell_y0,
                                        cell_z0,
                                        icon_width,
                                        icon_height);
                } else {
                    render_truck_terminal_icon(pLwc,
                                               lwttl_viewport_view(vp),
                                               lwttl_viewport_proj(vp),
                                               cell_x0,
                                               cell_y0,
                                               cell_z0,
                                               icon_width,
                                               icon_height);
                }
            }
        }
    }
}

static void render_cities(const LWCONTEXT* pLwc, const LWTTLFIELDVIEWPORT* vp) {
    const float icon_width = lwttl_viewport_icon_width(vp);
    const float icon_height = lwttl_viewport_icon_height(vp);

    int chunk_index_array[LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG_WITH_MARGIN*LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT_WITH_MARGIN];
    int bound_xcc0, bound_ycc0, bound_xcc1, bound_ycc1;
    const int chunk_index_array_count = lwttl_query_chunk_range_city_vp(pLwc->ttl,
                                                                        vp,
                                                                        chunk_index_array,
                                                                        ARRAY_SIZE(chunk_index_array),
                                                                        &bound_xcc0,
                                                                        &bound_ycc0,
                                                                        &bound_xcc1,
                                                                        &bound_ycc1);
    if (chunk_index_array_count > ARRAY_SIZE(chunk_index_array)) {
        LOGEP("incorrect query result");
        assert(0);
    }
    if (bound_xcc1 - bound_xcc0 > LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG_WITH_MARGIN) {
        LOGEP("incorrect query result");
        assert(0);
    }
    if (bound_ycc1 - bound_ycc0 > LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT_WITH_MARGIN) {
        LOGEP("incorrect query result");
        assert(0);
    }
    const int chunk_index_max = LWMIN(chunk_index_array_count, ARRAY_SIZE(chunk_index_array));
    for (int ci = 0; ci < chunk_index_max; ci++) {
        int obj_count = 0;
        int xc0 = 0;
        int yc0 = 0;
        const LWPTTLCITYOBJECT* obj_begin = lwttl_query_chunk_city(pLwc->ttl,
                                                                   chunk_index_array[ci],
                                                                   &xc0,
                                                                   &yc0,
                                                                   &obj_count);
        if (obj_begin && obj_count > 0) {
            for (int i = 0; i < obj_count; i++) {
                float cell_x0, cell_y0, cell_z0;
                if (lwttl_viewport_icon_render_info(pLwc->ttl,
                                                    vp,
                                                    xc0,
                                                    obj_begin[i].x_scaled_offset_0,
                                                    yc0,
                                                    obj_begin[i].y_scaled_offset_0,
                                                    &cell_x0,
                                                    &cell_y0,
                                                    &cell_z0) != 0) {
                    continue;
                }
                render_city_icon(pLwc,
                                 lwttl_viewport_view(vp),
                                 lwttl_viewport_proj(vp),
                                 cell_x0,
                                 cell_y0,
                                 cell_z0,
                                 icon_width,
                                 icon_height,
                                 obj_begin[i].population_level);
            }
        }
    }
}

static void render_salvages(const LWCONTEXT* pLwc, const LWTTLFIELDVIEWPORT* vp) {
    const float icon_width = lwttl_viewport_icon_width(vp);
    const float icon_height = lwttl_viewport_icon_height(vp);

    int chunk_index_array[LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG_WITH_MARGIN*LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT_WITH_MARGIN];
    int bound_xcc0, bound_ycc0, bound_xcc1, bound_ycc1;
    const int chunk_index_array_count = lwttl_query_chunk_range_salvage_vp(pLwc->ttl,
                                                                           vp,
                                                                           chunk_index_array,
                                                                           ARRAY_SIZE(chunk_index_array),
                                                                           &bound_xcc0,
                                                                           &bound_ycc0,
                                                                           &bound_xcc1,
                                                                           &bound_ycc1);
    if (chunk_index_array_count > ARRAY_SIZE(chunk_index_array)) {
        LOGEP("incorrect query result");
        assert(0);
    }
    if (bound_xcc1 - bound_xcc0 > LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG_WITH_MARGIN) {
        LOGEP("incorrect query result");
        assert(0);
    }
    if (bound_ycc1 - bound_ycc0 > LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT_WITH_MARGIN) {
        LOGEP("incorrect query result");
        assert(0);
    }
    const int chunk_index_max = LWMIN(chunk_index_array_count, ARRAY_SIZE(chunk_index_array));
    for (int ci = 0; ci < chunk_index_max; ci++) {
        int obj_count = 0;
        int xc0 = 0;
        int yc0 = 0;
        const LWPTTLSALVAGEOBJECT* obj_begin = lwttl_query_chunk_salvage(pLwc->ttl,
                                                                         chunk_index_array[ci],
                                                                         &xc0,
                                                                         &yc0,
                                                                         &obj_count);
        if (obj_begin && obj_count > 0) {
            for (int i = 0; i < obj_count; i++) {
                float cell_x0, cell_y0, cell_z0;
                if (lwttl_viewport_icon_render_info(pLwc->ttl,
                                                    vp,
                                                    xc0,
                                                    obj_begin[i].x_scaled_offset_0,
                                                    yc0,
                                                    obj_begin[i].y_scaled_offset_0,
                                                    &cell_x0,
                                                    &cell_y0,
                                                    &cell_z0) != 0) {
                    continue;
                }
                render_salvage_icon(pLwc,
                                    lwttl_viewport_view(vp),
                                    lwttl_viewport_proj(vp),
                                    cell_x0,
                                    cell_y0,
                                    cell_z0,
                                    icon_width,
                                    icon_height);
            }
        }
    }
}

static void render_sea_static_objects(const LWCONTEXT* pLwc,
                                      const LWTTLFIELDVIEWPORT* vp) {
    int chunk_index_array[LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG_WITH_MARGIN*LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT_WITH_MARGIN];
    int bound_xcc0, bound_ycc0, bound_xcc1, bound_ycc1;
    const int chunk_index_array_count = lwttl_query_chunk_range_land_vp(pLwc->ttl,
                                                                        vp,
                                                                        chunk_index_array,
                                                                        ARRAY_SIZE(chunk_index_array),
                                                                        &bound_xcc0,
                                                                        &bound_ycc0,
                                                                        &bound_xcc1,
                                                                        &bound_ycc1);
    if (chunk_index_array_count > ARRAY_SIZE(chunk_index_array)) {
        LOGEP("incorrect query result");
        assert(0);
    }
    if (bound_xcc1 - bound_xcc0 > LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG_WITH_MARGIN) {
        LOGEP("incorrect query result");
        assert(0);
    }
    if (bound_ycc1 - bound_ycc0 > LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT_WITH_MARGIN) {
        LOGEP("incorrect query result");
        assert(0);
    }
    unsigned char bitmap[LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT_WITH_MARGIN*LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS][LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG_WITH_MARGIN*LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS];
    memset(bitmap, 0, sizeof(bitmap));
    const int chunk_index_max = LWMIN(chunk_index_array_count, ARRAY_SIZE(chunk_index_array));
    const int clamped_view_scale = lwttl_viewport_clamped_view_scale(vp);
    for (int ci = 0; ci < chunk_index_max; ci++) {
        int obj_count = 0;
        int xc0 = 0;
        int yc0 = 0;
        const LWPTTLSTATICOBJECT2* obj_begin = lwttl_query_chunk_land(pLwc->ttl,
                                                                      chunk_index_array[ci],
                                                                      &xc0,
                                                                      &yc0,
                                                                      &obj_count);
        if (obj_begin && obj_count > 0) {
            //unsigned char chunk_bitmap[LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS][LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS];
            //memset(chunk_bitmap, 0, sizeof(chunk_bitmap));
            const int xcc0 = xc0 >> msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * clamped_view_scale);
            const int ycc0 = yc0 >> msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * clamped_view_scale);
            for (int i = 0; i < obj_count; i++) {
                // mapping scaled offset ranging [-LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS/2,+LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS/2]
                // in range of [0,LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS] so that in can be used as array index
                const int bitmap_x0 = LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS / 2 + obj_begin[i].x_scaled_offset_0;
                const int bitmap_y0 = LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS / 2 + obj_begin[i].y_scaled_offset_0;
                const int bitmap_x1 = LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS / 2 + obj_begin[i].x_scaled_offset_1;
                const int bitmap_y1 = LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS / 2 + obj_begin[i].y_scaled_offset_1;
                for (int by = bitmap_y0; by < bitmap_y1; by++) {
                    for (int bx = bitmap_x0; bx < bitmap_x1; bx++) {
                        bitmap[(ycc0 - bound_ycc0)*LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS + by][(xcc0 - bound_xcc0)*LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS + bx] = 1;
                    }
                }
            }
        }
    }
    // render water-land tilemap
    const int bound_xc0 = bound_xcc0 << msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * clamped_view_scale);
    const int bound_yc0 = bound_ycc0 << msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * clamped_view_scale);
    render_land_cell_bitmap(pLwc->ttl,
                            pLwc,
                            vp,
                            bound_xc0,
                            bound_yc0,
                            bitmap);
}

static void render_single_cell_info(const LWCONTEXT* pLwc,
                                    const LWTTLFIELDVIEWPORT* vp,
                                    const float x,
                                    const float y) {
    const LWPTTLSINGLECELL* p = lwttl_single_cell(pLwc->ttl);
    if (lwttl_is_selected_cell(pLwc->ttl, p->xc0, p->yc0) == 0) {
        return;
    }
    mat4x4 proj_view;
    mat4x4_identity(proj_view);
    mat4x4_mul(proj_view,
               lwttl_viewport_proj(vp),
               lwttl_viewport_view(vp));
    vec4 obj_pos_vec4 = {
        x,
        y,
        0,
        1,
    };
    vec2 ui_point;
    calculate_ui_point_from_world_point(pLwc->viewport_aspect_ratio, proj_view, obj_pos_vec4, ui_point);
    LWTEXTBLOCK tb;
    tb.text_block_width = 999.0f;
    tb.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    tb.size = DEFAULT_TEXT_BLOCK_SIZE_E;
    char info[512];
    const char* cell_type = 0;
    // fill cell type
    if ((p->attr >> 0) & 1) {
        cell_type = "Land";
    } else if ((p->attr >> 2) & 1) {
        cell_type = "Seawater";
    } else {
        cell_type = "Water";
    }
    // short description
    // "%s" is not interpolated if we use u8"★%s" as a format string... (why?)
    if (p->city_id >= 0 && p->city_name[0]) {
        sprintf(info,
                "%s%s%s%d",
                u8"★",
                p->city_name,
                u8"❖",
                1);
    } else if (p->port_id >= 0 && p->port_name[0]) {
        sprintf(info,
                "%s%s\n%s%d%s%d%s%d",
                u8"♦",
                p->port_name,
                u8"☗",
                p->cargo,
                u8"⬆",
                p->cargo_loaded,
                u8"⬇",
                p->cargo_unloaded);
    } else {
        /*sprintf(info,
                "%s",
                cell_type);*/
        info[0] = 0;
    }
    tb.text = info;
    tb.text_bytelen = (int)strlen(tb.text);
    tb.begin_index = 0;
    tb.end_index = tb.text_bytelen;
    tb.multiline = 1;
    tb.text_block_x = ui_point[0];
    tb.text_block_y = ui_point[1] + 0.05f / lwttl_viewport_view_scale(vp);
    tb.align = LTBA_LEFT_BOTTOM;
    render_text_block_two_pass(pLwc, &tb);
}

static void render_cell_pixel_selector_lng_lat(const LWTTL* ttl,
                                               const LWCONTEXT* pLwc,
                                               const LWTTLFIELDVIEWPORT* vp,
                                               const int xc0,
                                               const int yc0) {
    const float selector_rx = cell_x_to_render_coords(xc0, vp);
    const float selector_ry = cell_y_to_render_coords(yc0, vp);
    render_cell_pixel_selector(ttl,
                               pLwc,
                               lwttl_viewport_view(vp),
                               lwttl_viewport_proj(vp),
                               selector_rx,
                               selector_ry,
                               0,
                               lwttl_viewport_cell_render_width(vp),
                               lwttl_viewport_cell_render_height(vp),
                               lwttl_selected_cell_popup_height(pLwc->ttl, vp) / lwttl_viewport_view_scale(vp));
}

static void render_single_cell_info_lng_lat(const LWTTL* ttl,
                                            const LWCONTEXT* pLwc,
                                            const LWTTLFIELDVIEWPORT* vp,
                                            const int xc0,
                                            const int yc0) {
    const float selector_rx = cell_x_to_render_coords(xc0, vp);
    const float selector_ry = cell_y_to_render_coords(yc0, vp);
    render_single_cell_info(pLwc,
                            vp,
                            selector_rx,
                            selector_ry);
}

static void render_world_text(const LWCONTEXT* pLwc, const LWTTLFIELDVIEWPORT* vp) {
    mat4x4 proj_view;
    mat4x4_identity(proj_view);
    mat4x4_mul(proj_view,
               lwttl_viewport_proj(vp),
               lwttl_viewport_view(vp));
    const void* wt_it = lwttl_world_text_begin(pLwc->ttl);
    while (wt_it) {
        float ui_point_x, ui_point_y, scale;
        const char* text = lwttl_world_text_vp(pLwc->ttl,
                                               wt_it,
                                               vp,
                                               proj_view,
                                               &ui_point_x,
                                               &ui_point_y,
                                               &scale);
        LWTEXTBLOCK test_text_block;
        test_text_block.text_block_width = 999.0f;
        test_text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
        test_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_F * scale;
        test_text_block.text = text;
        test_text_block.text_bytelen = (int)strlen(test_text_block.text);
        test_text_block.begin_index = 0;
        test_text_block.end_index = test_text_block.text_bytelen;
        test_text_block.multiline = 1;
        test_text_block.text_block_x = ui_point_x;
        test_text_block.text_block_y = ui_point_y;
        test_text_block.align = LTBA_CENTER_CENTER;
        render_text_block_two_pass(pLwc, &test_text_block);
        wt_it = lwttl_world_text_next(pLwc->ttl, wt_it);
    }
}

static void render_world(const LWCONTEXT* pLwc, const LWTTLFIELDVIEWPORT* vp) {
    //render_ship(pLwc, view, proj, 0, ship_y, 0);
    render_sea_objects(pLwc, vp);
    //render_port(pLwc, view, proj, 0);
    //render_port(pLwc, view, proj, 160);
    //render_sea_city(pLwc, view, proj);
    //render_waves(pLwc, view, proj, ship_y);
}

static void degrees_to_dms(int* d, int* m, float* s, const float degrees) {
    *d = (int)degrees;
    const float minutes = (degrees - *d) * 60;
    *m = (int)minutes;
    *s = (minutes - *m) * 60;
}

static void render_coords(const LWCONTEXT* pLwc, const LWTTLLNGLAT* lng_lat_center) {
    LWTEXTBLOCK test_text_block;
    test_text_block.text_block_width = 999.0f;
    test_text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    test_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_E;
    SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_outline, 0, 0, 0, 1);
    char coords[256];
    snprintf(coords,
             ARRAY_SIZE(coords),
             "LNG %.3f LAT %.3f",
             lng_lat_center->lng,
             lng_lat_center->lat);
    coords[ARRAY_SIZE(coords) - 1] = 0;
    test_text_block.text = coords;
    test_text_block.text_bytelen = (int)strlen(test_text_block.text);
    test_text_block.begin_index = 0;
    test_text_block.end_index = test_text_block.text_bytelen;
    test_text_block.multiline = 1;
    test_text_block.text_block_x = -pLwc->viewport_rt_x;
    test_text_block.text_block_y = pLwc->viewport_rt_y;
    test_text_block.align = LTBA_LEFT_TOP;
    render_text_block(pLwc, &test_text_block);
}

static void render_coords_dms(const LWCONTEXT* pLwc, const LWTTLLNGLAT* lng_lat_center) {
    LWTEXTBLOCK test_text_block;
    test_text_block.text_block_width = 999.0f;
    test_text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    test_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_E;
    SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_outline, 0, 0, 0, 1);
    char coords[256];
    int lng_d, lng_m;
    float lng_s;
    int lat_d, lat_m;
    float lat_s;
    degrees_to_dms(&lng_d, &lng_m, &lng_s, lng_lat_center->lng);
    degrees_to_dms(&lat_d, &lat_m, &lat_s, lng_lat_center->lat);
    snprintf(coords,
             ARRAY_SIZE(coords),
             "LNG\n%dD\n%dM\n%.3fS\nLAT\n%dD\n%dM\n%.3fS",
             lng_d,
             lng_m,
             lng_s,
             lat_d,
             lat_m,
             lat_s);
    coords[ARRAY_SIZE(coords) - 1] = 0;
    test_text_block.text = coords;
    test_text_block.text_bytelen = (int)strlen(test_text_block.text);
    test_text_block.begin_index = 0;
    test_text_block.end_index = test_text_block.text_bytelen;
    test_text_block.multiline = 1;
    test_text_block.text_block_x = -pLwc->viewport_rt_x + 0.3f;
    test_text_block.text_block_y = 0;
    test_text_block.align = LTBA_LEFT_TOP;
    render_text_block(pLwc, &test_text_block);
}

static void render_region_name(const LWCONTEXT* pLwc, const LWTTLFIELDVIEWPORT* vp) {
    LWTEXTBLOCK test_text_block;
    test_text_block.text_block_width = 999.0f;
    test_text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    test_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_E;
    test_text_block.text = lwttl_seaarea(pLwc->ttl);
    test_text_block.text_bytelen = (int)strlen(test_text_block.text);
    test_text_block.begin_index = 0;
    test_text_block.end_index = test_text_block.text_bytelen;
    test_text_block.multiline = 1;
    test_text_block.text_block_x = -lwttl_viewport_rt_x(vp) + UI_SCREEN_EDGE_MARGIN;
    test_text_block.text_block_y = -lwttl_viewport_rt_y(vp) + UI_SCREEN_EDGE_MARGIN;
    test_text_block.align = LTBA_LEFT_BOTTOM;
    render_text_block_two_pass(pLwc, &test_text_block);
}

static void render_ttl_stat(const LWTTL* ttl, const LWCONTEXT* pLwc) {
    LWTEXTBLOCK test_text_block;
    test_text_block.text_block_width = 999.0f;
    test_text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_D;
    test_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_C;
    char gold_text[64];
    snprintf(gold_text,
             ARRAY_SIZE(gold_text) - 1,
             "%s%d\n%s%d\n%s%d",
             u8"◊",
             lwttl_gold(pLwc->ttl),
             u8"♦",
             lwttl_ports(pLwc->ttl),
             u8"⏅",
             lwttl_ships(pLwc->ttl));
    gold_text[ARRAY_SIZE(gold_text) - 1] = 0;
    test_text_block.text = gold_text;
    test_text_block.text_bytelen = (int)strlen(test_text_block.text);
    test_text_block.begin_index = 0;
    test_text_block.end_index = test_text_block.text_bytelen;
    test_text_block.multiline = 1;
    test_text_block.text_block_x = -pLwc->viewport_rt_x + UI_SCREEN_EDGE_MARGIN;
    test_text_block.text_block_y = +pLwc->viewport_rt_y - UI_SCREEN_EDGE_MARGIN;
    test_text_block.align = LTBA_LEFT_TOP;
    render_text_block_two_pass(pLwc, &test_text_block);
}

static void lwc_render_ttl_field_viewport(const LWCONTEXT* pLwc, const LWTTLFIELDVIEWPORT* vp) {
    glDisable(GL_DEPTH_TEST);
    render_morphed_earth(pLwc, vp);
    if (lwc_render_ttl_render("landcell")) {
        render_sea_static_objects(pLwc, vp);
    }
    render_waypoints(pLwc->ttl, pLwc, vp);
    render_waypoints_cache(pLwc->ttl, pLwc, vp);
    glEnable(GL_DEPTH_TEST);
    // render sea objects(ships)
    if (lwc_render_ttl_render("world")) {
        render_world(pLwc, vp);
    }
    glDisable(GL_DEPTH_TEST);
    int selected_xc0;
    int selected_yc0;
    if (lwttl_selected_int(pLwc->ttl, &selected_xc0, &selected_yc0)) {
        render_cell_pixel_selector_lng_lat(pLwc->ttl,
                                           pLwc,
                                           vp,
                                           selected_xc0,
                                           selected_yc0);
    }
    int dragging_xc0;
    int dragging_yc0;
    int dragging_xc1;
    int dragging_yc1;
    if (lwttl_dragging_info(pLwc->ttl,
                            &dragging_xc0,
                            &dragging_yc0,
                            &dragging_xc1,
                            &dragging_yc1)) {
        render_waypoint_line_segment(pLwc->ttl,
                                     pLwc,
                                     vp,
                                     dragging_xc0,
                                     dragging_yc0,
                                     dragging_xc1,
                                     dragging_yc1,
                                     CELL_DRAGGING_LINE_COLOR_R,
                                     CELL_DRAGGING_LINE_COLOR_G,
                                     CELL_DRAGGING_LINE_COLOR_B);
    }
    // UI (icons)
    render_seaports(pLwc, vp);
    render_cities(pLwc, vp);
    render_salvages(pLwc, vp);
    //render_coords(pLwc, &view_center);
    // UI (hud)
    if (lwc_render_ttl_render("world")) {
        render_sea_objects_nameplate(pLwc, vp);
    }
    if (lwc_render_ttl_render("landcell_nameplate")) {
    }
    if (lwttl_selected_int(pLwc->ttl, &selected_xc0, &selected_yc0)) {
        render_single_cell_info_lng_lat(pLwc->ttl,
                                        pLwc,
                                        vp,
                                        selected_xc0,
                                        selected_yc0);
    }
    render_world_text(pLwc, vp);
    render_region_name(pLwc, vp);
}

void lwc_render_ttl(const LWCONTEXT* pLwc) {
    LW_GL_VIEWPORT();
    lw_clear_color();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // render field viewports
    const int viewport_max_count = lwttl_viewport_max_count(pLwc->ttl);
    for (int i = 0; i < viewport_max_count; i++) {
        LWTTLFIELDVIEWPORT* vp_copy = alloca(lwttl_sizeof_viewport());
        if (lwttl_copy_viewport_data(pLwc->ttl, i, vp_copy)) {
            int viewport_x, viewport_y, viewport_width, viewport_height;
            lwttl_viewport_range(vp_copy,
                                 &viewport_x,
                                 &viewport_y,
                                 &viewport_width,
                                 &viewport_height);
            // render viewport frame
            if (i != 0) {
                glViewport(0,
                           0,
                           pLwc->window_width,
                           pLwc->window_height);
                const float viewport_frame_half_thickness = 0.015f;
                render_solid_vb_ui(pLwc,
                                   (-1.0f + 2.0f * (float)viewport_x / pLwc->window_width) * pLwc->viewport_rt_x - viewport_frame_half_thickness,
                                   (-1.0f + 2.0f * (float)viewport_y / pLwc->window_height) * pLwc->viewport_rt_y - viewport_frame_half_thickness,
                                   (float)viewport_width / pLwc->window_width * 2.0f * pLwc->viewport_rt_x + 2.0f * viewport_frame_half_thickness,
                                   (float)viewport_height / pLwc->window_height * 2.0f * pLwc->viewport_rt_y + 2.0f * viewport_frame_half_thickness,
                                   pLwc->tex_atlas[LAE_ZERO_FOR_BLACK],
                                   LVT_LEFT_BOTTOM_ANCHORED_SQUARE,
                                   1.0f,
                                   0.1f,
                                   0.3f,
                                   0.6f,
                                   1.0f);
            }
            glViewport(viewport_x,
                       viewport_y,
                       viewport_width,
                       viewport_height);
            lw_set_viewport_size((LWCONTEXT*)pLwc,
                                 viewport_width,
                                 viewport_height);
            lwc_render_ttl_field_viewport(pLwc, vp_copy);
        }
    }
    // revert to original viewport
    glViewport(0,
               0,
               pLwc->window_width,
               pLwc->window_height);
    lw_set_viewport_size((LWCONTEXT*)pLwc,
                         pLwc->window_width,
                         pLwc->window_height);
    render_ttl_stat(pLwc->ttl, pLwc);
    //render_coords_dms(pLwc, &view_center);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    // render FBO (HTML UI)
    render_solid_box_ui_lvt_flip_y_uv(pLwc,
                                      0,
                                      0,
                                      2 * pLwc->viewport_rt_x,
                                      2 * pLwc->viewport_rt_y,
                                      pLwc->shared_fbo.color_tex,
                                      LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                      1);
    // render joystick
    if (0) {
        render_dir_pad_with_start_joystick(pLwc, &pLwc->left_dir_pad, 1.0f);
    }
    glEnable(GL_DEPTH_TEST);
}

int lwc_render_ttl_render(const char* name) {
    size_t i;
    for (i = 0; i < MAX_VISIBILITY_ENTRY_COUNT; i++) {
        if (strcmp(visibility[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

void lwc_render_ttl_enable_render(const char* name, int v) {
    size_t i;
    for (i = 0; i < MAX_VISIBILITY_ENTRY_COUNT; i++) {
        if (strcmp(visibility[i], name) == 0) {
            if (v) {
                // already visible...
                return;
            } else {
                // clear
                visibility[i][0] = 0;
                return;
            }
        }
    }
    if (v) {
        for (i = 0; i < MAX_VISIBILITY_ENTRY_COUNT; i++) {
            if (visibility[i][0] == 0) {
                strcpy(visibility[i], name);
                return;
            }
        }
        LOGE("visibility_hash capacity exceeded.");
    }
}
