#include "render_ps.h"
#include "lwcontext.h"
#include "lwgl.h"
#include "ps.h"
#include "laidoff.h"
#include "platform_detection.h"
#include "lwmacro.h"

static void s_render_rose(const LWCONTEXT* pLwc, const LWPSCONTEXT* c) {
	int shader_index = LWST_EMITTER;
	lazy_glUseProgram(pLwc, shader_index);
	mat4x4 identity;
	mat4x4_identity(identity);
	glUniformMatrix4fv(pLwc->shader[shader_index].projection_matrix_location, 1, 0, (const GLfloat*)identity);
	glUniform1f(pLwc->shader[shader_index].k_location, c->emitter.k);
	glUniform3f(pLwc->shader[shader_index].color_location, c->emitter.color[0], c->emitter.color[1], c->emitter.color[2]);
	glUniform1f(pLwc->shader[shader_index].time_location, c->time_current / c->time_max);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pLwc->tex_programmed[LPT_DIR_PAD]);
	glUniform1i(pLwc->shader[shader_index].texture_location, 0);
	glBindBuffer(GL_ARRAY_BUFFER, pLwc->particle_buffer);
	bind_all_ps0_vertex_attrib(pLwc, LP0VT_DEFAULT);
	glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
}

void ps_render_explosion(const LWCONTEXT* pLwc, const LWPSCONTEXT* c, const LWEMITTER2OBJECT* emit_object, const mat4x4 proj_view, const mat4x4 model) {
	int shader_index = LWST_EMITTER2;
	lazy_glUseProgram(pLwc, shader_index);
	// Uniforms
	glUniformMatrix4fv(pLwc->shader[shader_index].u_ProjectionViewMatrix, 1, 0, (const GLfloat*)proj_view);
	glUniformMatrix4fv(pLwc->shader[shader_index].u_ModelMatrix, 1, 0, (const GLfloat*)model);
	glUniform2f(pLwc->shader[shader_index].u_Gravity, emit_object->gravity[0], emit_object->gravity[1]);
	glUniform1f(pLwc->shader[shader_index].u_Time, emit_object->time);
	glUniform1f(pLwc->shader[shader_index].u_eRadius, c->emitter2.eRadius);
	glUniform1f(pLwc->shader[shader_index].u_eVelocity, c->emitter2.eVelocity);
	glUniform1f(pLwc->shader[shader_index].u_eDecay, c->emitter2.eDecay);
	glUniform1f(pLwc->shader[shader_index].u_eSizeStart, c->emitter2.eSizeStart);
	glUniform1f(pLwc->shader[shader_index].u_eSizeEnd, c->emitter2.eSizeEnd);
	glUniform1f(pLwc->shader[shader_index].u_eScreenWidth, (float)pLwc->width);
	glUniform3fv(pLwc->shader[shader_index].u_eColorStart, 1, c->emitter2.eColorStart);
	glUniform3fv(pLwc->shader[shader_index].u_eColorEnd, 1, c->emitter2.eColorEnd);

	glActiveTexture(GL_TEXTURE0);
	lazy_tex_atlas_glBindTexture(pLwc, LAE_U_GLOW);
	glUniform1i(pLwc->shader[shader_index].u_Texture, 0);
	set_tex_filter(GL_LINEAR, GL_LINEAR);

	glActiveTexture(GL_TEXTURE1);
	lazy_tex_atlas_glBindTexture(pLwc, LAE_U_GLOW_ALPHA);
	glUniform1i(pLwc->shader[shader_index].u_TextureAlpha, 1);
	set_tex_filter(GL_LINEAR, GL_LINEAR);

	glBindBuffer(GL_ARRAY_BUFFER, pLwc->particle_buffer2);

	bind_all_ps_vertex_attrib(pLwc, LPVT_DEFAULT);

	// Draw particles
	glDrawArrays(GL_POINTS, 0, NUM_PARTICLES2);
}

void lwc_render_ps(const LWCONTEXT* pLwc, const LWPSCONTEXT* c) {
	LW_GL_VIEWPORT();
	lw_clear_color();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if LW_PLATFORM_WIN32
	//glEnable(GL_POINT_SPRITE);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
	// Test render rose
	s_render_rose(pLwc, pLwc->ps_context);
	// Test render explosion
	mat4x4 identity;
	mat4x4_identity(identity);
	ps_render_explosion(pLwc, c, &c->emitter2_object, identity, identity);
}

void ps_render_with_projection(const LWCONTEXT* pLwc, const LWEMITTER2OBJECT* emit, const float* proj) {
	
}
