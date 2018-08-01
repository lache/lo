#include "render_tilemap.h"
#include "lwgl.h"
#include <math.h>
#include "lwcontext.h"
#include "vertices.h"
#include "laidoff.h"
#include "render_solid.h"
#include "lwmacro.h"
#include <stdlib.h>
#include <string.h>
#include "lwlog.h"
#include "tilemap.h"

static LWVERTEX single_tile[] = {
    //x     y     z  r  g  b  u  v  s  s
    { -1.f, -1.f, 0, 0, 0, 0, 0, 1, 0, 0 },
    { +1.f, -1.f, 0, 0, 0, 0, 1, 1, 0, 0 },
    { +1.f, +1.f, 0, 0, 0, 0, 1, 0, 0, 0 },
    { +1.f, +1.f, 0, 0, 0, 0, 1, 0, 0, 0 },
    { -1.f, +1.f, 0, 0, 0, 0, 0, 0, 0, 0 },
    { -1.f, -1.f, 0, 0, 0, 0, 0, 1, 0, 0 },
};
const static int tile_chunk_tile_count_x = 18 * 1;
const static int tile_chunk_tile_count_y = 18 * 1;

const static int tile_chunk_count_x = 1;
const static int tile_chunk_count_y = 1;

const static float global_scale = 0.04f;

inline int tile_chunk_render_tile_count() {
    return (tile_chunk_tile_count_x) * (tile_chunk_tile_count_y);
}

inline int tile_chunk_vertex_count() {
    return ARRAY_SIZE(single_tile) * tile_chunk_render_tile_count();
}

inline float tile_chunk_width() {
    return 2.0f * (tile_chunk_tile_count_x);
}

inline float tile_chunk_height() {
    return 2.0f * (tile_chunk_tile_count_y);
}

#define TILEMAP_BITMAP_TOTAL_WIDTH (18)
#define TILEMAP_BITMAP_TOTAL_HEIGHT (18)
unsigned char bitmap[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

typedef struct _LWVBOVAO {
    GLuint vbo;
    GLuint vao;
} LWVBOVAO;

static LWVBOVAO* dynamic_vbovao;

static LWVBOVAO create_tile_chunk(const LWCONTEXT* pLwc, int tcx, int tcy) {
    LWVBOVAO vbovao;
    memset(&vbovao, 0, sizeof(LWVBOVAO));
    glGenBuffers(1, &vbovao.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbovao.vbo);
    LWVERTEX* tilemap = malloc(sizeof(single_tile) * tile_chunk_render_tile_count());
    LWVERTEX* tilemap_cursor = tilemap;
    const float* uv_scale = tilemap_uv_scale();
    for (int i = 0; i < tile_chunk_tile_count_y; i++) {
        for (int j = 0; j < tile_chunk_tile_count_x; j++) {
            memcpy(tilemap_cursor, single_tile, sizeof(single_tile));
            const float* uv_offset = tilemap_uv_offset(TILEMAP_BITMAP_TOTAL_WIDTH,
                                                       TILEMAP_BITMAP_TOTAL_HEIGHT,
                                                       bitmap,
                                                       tcx * tile_chunk_tile_count_x + j,
                                                       tcy * tile_chunk_tile_count_y + i);
            for (int k = 0; k < ARRAY_SIZE(single_tile); k++) {
                // make left top corner anchor point
                tilemap_cursor[k].x += +2.0f * j + 1.0f;
                tilemap_cursor[k].y += -2.0f * i - 1.0f;
                tilemap_cursor[k].u = uv_offset[0] + single_tile[k].u * uv_scale[0];
                tilemap_cursor[k].v = uv_offset[1] + single_tile[k].v * uv_scale[1];
            }
            tilemap_cursor += ARRAY_SIZE(single_tile);
        }
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(single_tile) * tile_chunk_render_tile_count(), tilemap, GL_DYNAMIC_DRAW);
    (void)free(tilemap), tilemap = tilemap_cursor = 0;
#if LW_SUPPORT_VAO
    glGenVertexArrays(1, &vbovao.vao);
    glBindVertexArray(vbovao.vao);
    set_vertex_attrib_pointer(pLwc, LWST_TILEMAP);
#endif
    LOGI("Tile chunk (%d,%d) vertex count: %d", tcx, tcy, tile_chunk_vertex_count());
    return vbovao;
}

void lwc_init_tilemap(const LWCONTEXT* pLwc) {
    if (dynamic_vbovao == 0) {
        dynamic_vbovao = malloc(sizeof(LWVBOVAO) * tile_chunk_count_x * tile_chunk_count_y);
        for (int i = 0; i < tile_chunk_count_y; i++) {
            for (int j = 0; j < tile_chunk_count_x; j++) {
                dynamic_vbovao[i * tile_chunk_count_x + j] = create_tile_chunk(pLwc, j, i);
            }
        }
    }
}

void lwc_destroy_tilemap() {
    if (dynamic_vbovao) {
        for (int i = 0; i < tile_chunk_count_y; i++) {
            for (int j = 0; j < tile_chunk_count_x; j++) {
                glDeleteBuffers(1, &dynamic_vbovao[i * tile_chunk_count_x + j].vbo);
#if LW_SUPPORT_VAO
                glDeleteVertexArrays(1, &dynamic_vbovao[i * tile_chunk_count_x + j].vao);
#endif
            }
        }
        (void)free(dynamic_vbovao), dynamic_vbovao = 0;
    }
}

static void update_vbo(const LWCONTEXT* pLwc) {
}

static void render_tile_chunk_at(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, int tcx, int tcy) {
    const int shader_index = LWST_TILEMAP;
    const float alpha_multiplier = 1.0f;
    const float over_r = 1.0f;
    const float over_g = 1.0f;
    const float over_b = 1.0f;
    const float oratio = 0.0f;
    //const vec4* proj = pLwc->proj;
    const float sx = 1.0f;
    const float sy = 1.0f;
    const float sz = 1.0f;
    const float rot_z = 0;
    const float x = tile_chunk_width() * tcx;
    const float y = -tile_chunk_height() * tcy;
    const float z = 0;
    /*mat4x4 view;
    mat4x4_identity(view);

    view[0][0] = global_scale;
    view[1][1] = global_scale;
    view[2][2] = global_scale;*/

    const LWSHADER* shader = &pLwc->shader[shader_index];
    lazy_glUseProgram(pLwc, shader_index);
    glUniform2fv(shader->vuvoffset_location, 1, default_uv_offset);
    glUniform2fv(shader->vuvscale_location, 1, default_uv_scale);
    glUniform2fv(shader->vs9offset_location, 1, default_uv_offset);
    glUniform1f(shader->alpha_multiplier_location, alpha_multiplier);
    glUniform1i(shader->diffuse_location, 0); // 0 means GL_TEXTURE0
    glUniform1i(shader->alpha_only_location, 1); // 1 means GL_TEXTURE1
    glUniform3f(shader->overlay_color_location, over_r, over_g, over_b);
    glUniform1f(shader->overlay_color_ratio_location, oratio);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj);

    mat4x4 model_translate;
    mat4x4 model;
    mat4x4 view_model;
    mat4x4 proj_view_model;
    mat4x4 model_scale;
    mat4x4 model_rotate;
    mat4x4 model_scale_rotate;

    mat4x4_identity(model_scale);
    mat4x4_identity(model_rotate);
    mat4x4_scale_aniso(model_scale, model_scale, sx, sy, sz);
    mat4x4_rotate_Z(model_rotate, model_rotate, rot_z);
    mat4x4_mul(model_scale_rotate, model_rotate, model_scale);
    mat4x4_translate(model_translate, x, y, z);
    mat4x4_identity(model);
    mat4x4_mul(model, model_translate, model_scale_rotate);
    mat4x4_mul(view_model, view, model);
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);

    const LWVBOVAO* vbovao = &dynamic_vbovao[tcy * tile_chunk_count_x + tcx];

    glBindBuffer(GL_ARRAY_BUFFER, vbovao->vbo); //lazy_glBindBuffer(pLwc, lvt);
#if LW_SUPPORT_VAO
    glBindVertexArray(vbovao->vao);
#else
    set_vertex_attrib_pointer(pLwc, shader_index);
#endif
    glActiveTexture(GL_TEXTURE0);
    lazy_tex_atlas_glBindTexture(pLwc, LAE_WATER_SAND_TILE_GRID_1X);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glDrawArrays(GL_TRIANGLES, 0, tile_chunk_vertex_count());
}

void lwc_render_tilemap(const LWCONTEXT* pLwc) {
    LW_GL_VIEWPORT();
    lw_clear_color();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4x4 view, proj;
    const float aspect_ratio = pLwc->viewport_aspect_ratio;
    // half_height := (how many cells in vertical axis) / 2
    // (relative to main viewport height)
    float half_height = 40.0f;
    float near_z = 0.1f;
    float far_z = 1000.0f;
    // eye(camera) position
    const float cx = +(float)(tile_chunk_tile_count_x * tile_chunk_count_x);
    const float cy = -(float)(tile_chunk_tile_count_y * tile_chunk_count_y);
    vec3 eye = {
        cx,
        cy,
        10.0f,
    };
    // look position
    vec3 center = {
        cx,
        cy,
        0,
    };
    vec3 center_to_eye;
    vec3_sub(center_to_eye, eye, center);
    float cam_a = atan2f(center_to_eye[1], center_to_eye[0]);
    // right := rotate (1, 0, 0) by cam_a in +Z axis
    vec3 right = { 0, 0, 0 };
    if (center_to_eye[0] == 0 && center_to_eye[1] == 0) {
        right[0] = 1;
    } else {
        right[0] = cosf((float)(M_PI / 2) + cam_a);
        right[1] = sinf((float)(M_PI / 2) + cam_a);
    }
    vec3 eye_right;
    vec3_mul_cross(eye_right, center_to_eye, right);
    vec3 up;
    vec3_norm(up, eye_right);
    mat4x4_ortho(proj,
                 -half_height * aspect_ratio,
                 +half_height * aspect_ratio,
                 -half_height,
                 +half_height,
                 near_z,
                 far_z);
    mat4x4_look_at(view, eye, center, up);

    for (int i = 0; i < tile_chunk_count_y; i++) {
        for (int j = 0; j < tile_chunk_count_x; j++) {
            render_tile_chunk_at(pLwc, view, proj, j, i);
        }
    }
    /*render_tile_chunk_at(pLwc, 0, 0);
    render_tile_chunk_at(pLwc, 1, 0);
    render_tile_chunk_at(pLwc, 2, 0);
    render_tile_chunk_at(pLwc, 0, 1);*/
    update_vbo(pLwc);
}
