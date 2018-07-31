#include "render_tilemap.h"
#include "lwgl.h"
#include <math.h>
#include "lwcontext.h"
#include "vertices.h"
#include "laidoff.h"
#include "render_solid.h"
#include "lwmacro.h"

static GLuint dynamic_vbo = 0;
static GLuint dynamic_vao = 0;
static LWVERTEX square[] = {
	//x     y     z  r  g  b  u  v  s  s
	{ -1.f, -1.f, 0, 0, 0, 0, 0, 1, 0, 0 },
	{ +1.f, -1.f, 0, 0, 0, 0, 1, 1, 0, 0 },
	{ +1.f, +1.f, 0, 0, 0, 0, 1, 0, 0, 0 },
	{ +1.f, +1.f, 0, 0, 0, 0, 1, 0, 0, 0 },
	{ -1.f, +1.f, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ -1.f, -1.f, 0, 0, 0, 0, 0, 1, 0, 0 },
};
const static int tile_count_x = 9*3;
const static int tile_count_y = 9*3;

void lwc_init_tilemap(LWCONTEXT* pLwc) {
	if (dynamic_vao == 0) {
		glGenBuffers(1, &dynamic_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, dynamic_vbo);
        LWVERTEX* tilemap = malloc(sizeof(square)*tile_count_x*tile_count_y);
        LWVERTEX* tilemap_cursor = tilemap;
        for (int i = -tile_count_y/2; i <= tile_count_y/2; i++) {
            for (int j = -tile_count_x/2; j <= tile_count_x/2; j++) {
                memcpy(tilemap_cursor, square, sizeof(square));
                for (int k = 0; k < ARRAY_SIZE(square); k++) {
                    tilemap_cursor[k].x += 2.0f * j;
                    tilemap_cursor[k].y += 2.0f * i;
                    tilemap_cursor[k].u /= 4.0f;
                    tilemap_cursor[k].v /= 4.0f;
                    tilemap_cursor[k].u += 1.0f / 4 * j;
                    tilemap_cursor[k].v += 1.0f / 4 * i;
                }
                tilemap_cursor += ARRAY_SIZE(square);
            }
        }
		glBufferData(GL_ARRAY_BUFFER, sizeof(square)*tile_count_x*tile_count_y, tilemap, GL_DYNAMIC_DRAW);
        free(tilemap), tilemap = tilemap_cursor = 0;
#if LW_SUPPORT_VAO
		glGenVertexArrays(1, &dynamic_vao);
		glBindVertexArray(dynamic_vao);
		set_vertex_attrib_pointer(pLwc, LWST_TILEMAP);
#endif
	}
}

static void update_vbo(const LWCONTEXT* pLwc) {
}

void lwc_render_tilemap(const LWCONTEXT* pLwc) {
    LW_GL_VIEWPORT();
    lw_clear_color();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	int shader_index = LWST_TILEMAP;
	float alpha_multiplier = 1.0f;
	float over_r = 1.0f;
	float over_g = 1.0f;
	float over_b = 1.0f;
	float oratio = 0.0f;
	const vec4* proj = pLwc->proj;
	float sx = 0.1f;
	float sy = 0.1f;
	float sz = 0.1f;
	float rot_z = 0;
	mat4x4 view;
	mat4x4_identity(view);
	float x = 0;
	float y = 0;
	float z = 0;
	GLsizei vertex_count = ARRAY_SIZE(square) * tile_count_x * tile_count_y;

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

	glBindBuffer(GL_ARRAY_BUFFER, dynamic_vbo); //lazy_glBindBuffer(pLwc, lvt);
#if LW_SUPPORT_VAO
	glBindVertexArray(dynamic_vao);
#else
	set_vertex_attrib_pointer(pLwc, shader_index);
#endif
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pLwc->tex_atlas[LAE_WATER_SAND_TILE_GRID_1X]);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
	glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
	glDrawArrays(GL_TRIANGLES, 0, vertex_count);

    update_vbo(pLwc);
}
