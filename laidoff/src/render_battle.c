#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "laidoff.h"
#include "lwcontext.h"
#include "render_battle.h"
#include "render_text_block.h"
#include "lwmacro.h"
#include "lwsimpleanim.h"
#include "lwenemy.h"
#include "render_solid.h"
#include "lwlog.h"

void render_enemy_scope(const LWCONTEXT* pLwc, float ux, float uy, float width, float height, LWATTRIBVALUE enemy_attrib) {

	// Minimum size constraints
	if (width < 0.4f || height < 0.4f) {
		return;
	}

	int shader_index = LWST_ETC1;

	float scale = 0.2f;
	//float sprite_aspect_ratio = 1.0f;

	mat4x4 model_translate;
	mat4x4 model_scale;
	mat4x4 model;
	mat4x4 identity_view;
	mat4x4 view_model;
	mat4x4 proj_view_model;

	mat4x4_identity(identity_view);

	//const float T = (float)M_PI;

	mat4x4_identity(model_scale);
	mat4x4_scale_aniso(model_scale, model_scale, scale, scale, scale);

	mat4x4_translate(model_translate, ux, uy, 0);

	mat4x4_identity(model);
	mat4x4_mul(model, model, model_translate);
	//mat4x4_rotate_Y(model, model, (float)LWDEG2RAD(-30));
	//mat4x4_rotate_X(model, model, (float)LWDEG2RAD(90));
	mat4x4_mul(model, model, model_scale);

	mat4x4 view_identity;
	mat4x4_identity(view_identity);

	mat4x4_mul(view_model, view_identity, model);

	mat4x4_identity(proj_view_model);
	mat4x4_mul(proj_view_model, pLwc->proj, view_model);

	LW_VBO_TYPE lvt = LVT_ENEMY_SCOPE;

	const float s9_offset[2] = { width / scale - 2, -(height / scale - 2) };

	int weak = 0;
	int resi = 0;
	int immu = 0;

	if (pLwc->player_turn_creature_index >= 0) {
		const LWBATTLECREATURE* pc = &pLwc->player[pLwc->player_turn_creature_index];
		if (pLwc->selected_command_slot >= 0 && pc->skill[pLwc->selected_command_slot]) {
			const LWATTRIBVALUE skill_attrib = pc->skill[pLwc->selected_command_slot]->attrib;

			weak = (skill_attrib.bits.air == LW_ATTRIB_DEFENCE_IMMU && enemy_attrib.bits.air == LW_ATTRIB_DEFENCE_WEAK)
				|| (skill_attrib.bits.wat == LW_ATTRIB_DEFENCE_IMMU && enemy_attrib.bits.wat == LW_ATTRIB_DEFENCE_WEAK)
				|| (skill_attrib.bits.fir == LW_ATTRIB_DEFENCE_IMMU && enemy_attrib.bits.fir == LW_ATTRIB_DEFENCE_WEAK)
				|| (skill_attrib.bits.ear == LW_ATTRIB_DEFENCE_IMMU && enemy_attrib.bits.ear == LW_ATTRIB_DEFENCE_WEAK)
				|| (skill_attrib.bits.god == LW_ATTRIB_DEFENCE_IMMU && enemy_attrib.bits.god == LW_ATTRIB_DEFENCE_WEAK)
				|| (skill_attrib.bits.evl == LW_ATTRIB_DEFENCE_IMMU && enemy_attrib.bits.evl == LW_ATTRIB_DEFENCE_WEAK);

			resi = (skill_attrib.bits.air == LW_ATTRIB_DEFENCE_IMMU && enemy_attrib.bits.air == LW_ATTRIB_DEFENCE_RESI)
				|| (skill_attrib.bits.wat == LW_ATTRIB_DEFENCE_IMMU && enemy_attrib.bits.wat == LW_ATTRIB_DEFENCE_RESI)
				|| (skill_attrib.bits.fir == LW_ATTRIB_DEFENCE_IMMU && enemy_attrib.bits.fir == LW_ATTRIB_DEFENCE_RESI)
				|| (skill_attrib.bits.ear == LW_ATTRIB_DEFENCE_IMMU && enemy_attrib.bits.ear == LW_ATTRIB_DEFENCE_RESI)
				|| (skill_attrib.bits.god == LW_ATTRIB_DEFENCE_IMMU && enemy_attrib.bits.god == LW_ATTRIB_DEFENCE_RESI)
				|| (skill_attrib.bits.evl == LW_ATTRIB_DEFENCE_IMMU && enemy_attrib.bits.evl == LW_ATTRIB_DEFENCE_RESI);

			immu = (skill_attrib.bits.air == LW_ATTRIB_DEFENCE_IMMU && enemy_attrib.bits.air == LW_ATTRIB_DEFENCE_IMMU)
				|| (skill_attrib.bits.wat == LW_ATTRIB_DEFENCE_IMMU && enemy_attrib.bits.wat == LW_ATTRIB_DEFENCE_IMMU)
				|| (skill_attrib.bits.fir == LW_ATTRIB_DEFENCE_IMMU && enemy_attrib.bits.fir == LW_ATTRIB_DEFENCE_IMMU)
				|| (skill_attrib.bits.ear == LW_ATTRIB_DEFENCE_IMMU && enemy_attrib.bits.ear == LW_ATTRIB_DEFENCE_IMMU)
				|| (skill_attrib.bits.god == LW_ATTRIB_DEFENCE_IMMU && enemy_attrib.bits.god == LW_ATTRIB_DEFENCE_IMMU)
				|| (skill_attrib.bits.evl == LW_ATTRIB_DEFENCE_IMMU && enemy_attrib.bits.evl == LW_ATTRIB_DEFENCE_IMMU);
		}
	}

	// overlay color
	const float color_preset[][5] = {
		//OR     OG     OB     ORat   alpha_mult
		{ 0.00f, 0.00f, 0.00f, 0.00f, 0.40f }, // Normal
		{ 0.38f, 0.97f, 1.00f, 1.00f, 0.70f }, // Weak
		{ 1.00f, 0.73f, 0.00f, 1.00f, 0.60f }, // Resi
		{ 1.00f, 0.40f, 0.40f, 1.00f, 0.60f }, // Immu
	};

	int color_preset_index = 0;

	if (!weak && !resi && !immu) {
		color_preset_index = 0;
	} else if (weak) {
		color_preset_index = 1;
	} else if (resi) {
		color_preset_index = 2;
	} else if (immu) {
		color_preset_index = 3;
	}

	lazy_glUseProgram(pLwc, shader_index);
	glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, default_uv_offset);
	glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, default_uv_scale);
	glUniform2fv(pLwc->shader[shader_index].vs9offset_location, 1, s9_offset);
	glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, color_preset[color_preset_index][4]);
	glUniform1i(pLwc->shader[shader_index].diffuse_location, 0); // 0 means GL_TEXTURE0
	glUniform1i(pLwc->shader[shader_index].alpha_only_location, 1); // 1 means GL_TEXTURE1

	lazy_glBindBuffer(pLwc, lvt);
	bind_all_vertex_attrib_etc1_with_alpha(pLwc, lvt);
	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE,
		(const GLfloat *)proj_view_model);
	glActiveTexture(GL_TEXTURE0);
    lazy_tex_atlas_glBindTexture(pLwc, LAE_U_ENEMY_SCOPE_KTX);
	//glBindTexture(GL_TEXTURE_2D, pLwc->tex_programmed[LPT_SOLID_RED]);
	set_tex_filter(GL_LINEAR, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glActiveTexture(GL_TEXTURE1);
    lazy_tex_atlas_glBindTexture(pLwc, LAE_U_ENEMY_SCOPE_ALPHA_KTX);
	//glBindTexture(GL_TEXTURE_2D, pLwc->tex_programmed[LPT_SOLID_WHITE_WITH_ALPHA]);
	set_tex_filter(GL_LINEAR, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glUniform3f(pLwc->shader[shader_index].overlay_color_location, color_preset[color_preset_index][0], color_preset[color_preset_index][1], color_preset[color_preset_index][2]);
	glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, color_preset[color_preset_index][3]);
	glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);

	//glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	set_tex_filter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0);

	// Revert S9 Offset
	glUniform2fv(pLwc->shader[shader_index].vs9offset_location, 1, default_uv_offset);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);

    lazy_glUseProgram(pLwc, 0);
}

void render_enemy_shadow_3d(
	const LWCONTEXT* pLwc,
	float x,
	float y,
	float z,
	float shadow_scale,
	const mat4x4 view,
	const mat4x4 proj,
	float alpha_multiplier,
	float time_offset) {

	const float T = (float)M_PI;

	const float osc = (float)sin((2 * M_PI) * (pLwc->app_time + time_offset) / T);

	mat4x4 mt, ms, m, pvm;
	mat4x4_identity(ms);
	mat4x4_identity(pvm);

	shadow_scale += 0.005f * osc;

	mat4x4_translate(mt, x, y, z);
	mat4x4_scale_aniso(ms, ms, shadow_scale, shadow_scale, shadow_scale);
	mat4x4_mul(m, mt, ms);
	mat4x4_mul(pvm, m, pvm);
	mat4x4_mul(pvm, view, pvm);
	mat4x4_mul(pvm, proj, pvm);

	int shader_index = LWST_DEFAULT;

	lazy_glUseProgram(pLwc, shader_index);
	glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, default_uv_offset);
	glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, default_uv_scale);
	glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, alpha_multiplier);
	glUniform3f(pLwc->shader[shader_index].overlay_color_location, 0, 0, 0);
	glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0.0f);

	LW_VBO_TYPE lvt = LVT_CENTER_CENTER_ANCHORED_SQUARE;

	lazy_glBindBuffer(pLwc, lvt);
	bind_all_vertex_attrib(pLwc, lvt);
	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE,
		(const GLfloat *)pvm);
	glActiveTexture(GL_TEXTURE0);
    lazy_tex_atlas_glBindTexture(pLwc, LAE_C2_PNG);
	set_tex_filter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	set_texture_parameter(pLwc, LAE_C2_PNG, LAS_SHADOW);
	glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

void render_enemy_3d(
	const LWCONTEXT* pLwc,
	const LWENEMY* enemy,
	int selected,
	float overlay[5],
	const mat4x4 view,
	const mat4x4 proj) {

	int shader_index = LWST_DEFAULT;

	float sprite_aspect_ratio = 1.0f;

	mat4x4 model_translate;
	mat4x4 model_scale;
	mat4x4 model;
	mat4x4 identity_view;
	mat4x4 proj_view_model;

	const float shadow_alpha_multiplier = overlay[4];

	render_enemy_shadow_3d(pLwc, enemy->render_pos[0], enemy->render_pos[1], 0,
		enemy->shadow_scale, view, proj, 1 - shadow_alpha_multiplier, enemy->time_offset);

	mat4x4_identity(identity_view);

	// enemy
	sprite_aspect_ratio = (float)SPRITE_DATA[0][enemy->las].w / SPRITE_DATA[0][enemy->las].h;
	mat4x4_identity(model_scale);
	mat4x4_scale_aniso(model_scale, model_scale, enemy->scale * sprite_aspect_ratio, enemy->scale, enemy->scale);

	mat4x4_translate(model_translate, enemy->render_pos[0], enemy->render_pos[1], enemy->render_pos[2]);

	//printf("%.2f %.2f %.2f\n", enemy->render_pos[0], enemy->render_pos[1], enemy->render_pos[2]);

	mat4x4_identity(model);
	mat4x4_mul(model, model_translate, model_scale);
	mat4x4_rotate_X(model, model, (float)LWDEG2RAD(90));

	mat4x4 proj_view;
	mat4x4_identity(proj_view);
	mat4x4_mul(proj_view, proj, view);

	mat4x4_identity(proj_view_model);
	mat4x4_mul(proj_view_model, proj_view, model);

	LW_VBO_TYPE lvt = LVT_CENTER_BOTTOM_ANCHORED_SQUARE;

	glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, 1.0f - overlay[4]);

	{
		lazy_glBindBuffer(pLwc, lvt);
		bind_all_vertex_attrib(pLwc, lvt);
		glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE,
			(const GLfloat *)proj_view_model);
		glUniform3f(pLwc->shader[shader_index].overlay_color_location, 1, 0, 0);
		glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0.0f);
        lazy_tex_atlas_glBindTexture(pLwc, LAE_C2_PNG);
		//glBindTexture(GL_TEXTURE_2D, pLwc->tex_programmed[LPT_SOLID_BLUE]);
		set_tex_filter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

		if (selected) {
			glUniform3f(pLwc->shader[shader_index].overlay_color_location, 1, 0, 0);
			glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 1.0f);
			set_texture_parameter(pLwc, LAE_C2_PNG, enemy->las - 1); // fat version
			glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
		}

		glUniform3fv(pLwc->shader[shader_index].overlay_color_location, 1, overlay);
		glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, overlay[3]);
		set_texture_parameter(pLwc, LAE_C2_PNG, enemy->las);
		glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
	}

	glUniform1f(pLwc->shader[0].overlay_color_ratio_location, 0);

	const int enemy_scope_at_player_turn = (pLwc->battle_state == LBS_SELECT_COMMAND || pLwc->battle_state == LBS_SELECT_TARGET)
		&& enemy->c.hp > 0;

	const int enemy_scope_at_enemy_turn = (pLwc->battle_state == LBS_ENEMY_COMMAND_IN_PROGRESS)
		&& enemy->c.selected;

	float enemy_scope_ui_width = enemy->right_bottom_ui_point[0] - enemy->left_top_ui_point[0];
	float enemy_scope_ui_height = enemy->left_top_ui_point[1] - enemy->right_bottom_ui_point[1];

	// Enemy Scope
	if (enemy_scope_at_player_turn || enemy_scope_at_enemy_turn) {
		render_enemy_scope(
			pLwc,
			enemy->left_top_ui_point[0],
			enemy->left_top_ui_point[1],
			enemy_scope_ui_width,
			enemy_scope_ui_height,
			enemy->c.stat.attrib
		);
	}
}

void render_attack_trail_3d(
	const LWCONTEXT* pLwc,
	const LWTRAIL* trail,
	const mat4x4 view,
	const mat4x4 proj) {

	int shader_index = LWST_ETC1;

	float scale = 1.0f;
	//float sprite_aspect_ratio = 1.0f;

	mat4x4 model_translate;
	mat4x4 model_scale;
	mat4x4 model;
	mat4x4 identity_view;
	mat4x4 view_model;
	mat4x4 proj_view_model;

	mat4x4_identity(identity_view);

	//const float T = (float)M_PI;

	mat4x4_identity(model_scale);
	mat4x4_scale_aniso(model_scale, model_scale, scale, scale / 1.5f, scale);

	mat4x4_translate(model_translate, trail->x, trail->y, trail->z);

	mat4x4_identity(model);
	mat4x4_mul(model, model, model_translate);
	mat4x4_rotate_Y(model, model, (float)LWDEG2RAD(-30));
	mat4x4_rotate_X(model, model, (float)LWDEG2RAD(90));
	mat4x4_mul(model, model, model_scale);

	mat4x4_mul(view_model, view, model);

	mat4x4_identity(proj_view_model);
	mat4x4_mul(proj_view_model, proj, view_model);

	LW_VBO_TYPE lvt = LVT_TRAIL;

	const float uv_offset[2] = { trail->tex_coord, 0 };

	lazy_glUseProgram(pLwc, shader_index);
	glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, uv_offset);
	glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, default_uv_scale);
	glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, 1);
	glUniform1i(pLwc->shader[shader_index].diffuse_location, 0); // 0 means GL_TEXTURE0
	glUniform1i(pLwc->shader[shader_index].alpha_only_location, 1); // 1 means GL_TEXTURE1

	lazy_glBindBuffer(pLwc, lvt);
	bind_all_vertex_attrib(pLwc, lvt);
	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE,
		(const GLfloat *)proj_view_model);
	glActiveTexture(GL_TEXTURE0);
	lazy_tex_atlas_glBindTexture(pLwc, LAE_FX_TRAIL);
	set_tex_filter(GL_LINEAR, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glActiveTexture(GL_TEXTURE1);
	lazy_tex_atlas_glBindTexture(pLwc, LAE_FX_TRAIL_ALPHA);
	set_tex_filter(GL_LINEAR, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glUniform3f(pLwc->shader[shader_index].overlay_color_location, 1, 0, 0);
	glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0.0f);
	glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);

	//glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	set_tex_filter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0);

    lazy_glUseProgram(pLwc, 0);
}

static void render_battle_twirl(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj) {
	int shader_index = LWST_DEFAULT;

	lazy_glUseProgram(pLwc, shader_index);
	glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0);

	// render vbo #2 (Wall)
	{
		mat4x4 model;
		mat4x4_identity(model);
		//mat4x4_rotate_Z(model, model, (float)(-pLwc->app_time / 4));

		mat4x4 view_model;
		mat4x4_mul(view_model, view, model);

		mat4x4 proj_view_model;
		mat4x4_identity(proj_view_model);
		mat4x4_mul(proj_view_model, proj, view_model);

		// uv manipulation
		const float uv_offset[2] = { 0, pLwc->battle_wall_tex_v };
		glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, uv_offset);
		const float uv_scale[2] = { 1, 1 };
		glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, uv_scale);

        lazy_glBindBuffer(pLwc, LVT_BATTLE_BOWL_INNER);
		bind_all_vertex_attrib(pLwc, LVT_BATTLE_BOWL_INNER);
		glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
        lazy_tex_atlas_glBindTexture(pLwc, LAE_TWIRL_PNG);
		set_tex_filter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
		glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[LVT_BATTLE_BOWL_INNER].vertex_count);
	}

	// render vbo #1 (Ground)
	{
		mat4x4 model;
		mat4x4_identity(model);
		mat4x4_rotate_Z(model, model, -(float)(pLwc->app_time / 3));

		mat4x4 view_model;
		mat4x4_mul(view_model, view, model);

		mat4x4 proj_view_model;
		mat4x4_identity(proj_view_model);
		mat4x4_mul(proj_view_model, proj, view_model);

		// uv manipulation
		const float uv_offset[2] = { 0, 0 };
		glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, uv_offset);
		const float uv_scale[2] = { 1, 1 };
		glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, uv_scale);

        lazy_glBindBuffer(pLwc, LVT_BATTLE_BOWL_OUTER);
		bind_all_vertex_attrib(pLwc, LVT_BATTLE_BOWL_OUTER);
		glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
        lazy_tex_atlas_glBindTexture(pLwc, LAE_TWIRL_PNG);
		set_tex_filter(GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR);
		glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[LVT_BATTLE_BOWL_OUTER].vertex_count);
	}
}

void get_player_creature_ui_box(int pos, float screen_aspect_ratio, float* left_top_x, float* left_top_y, float* area_width, float* area_height) {
	*left_top_x = (-1 + (2.0f / MAX_PLAYER_SLOT * pos)) * screen_aspect_ratio;
	*left_top_y = 1;

	*area_width = 2.0f / MAX_PLAYER_SLOT * screen_aspect_ratio;
	*area_height = 0.5f;
}

void render_player_creature_ui(const LWCONTEXT* pLwc, const LWBATTLECREATURE* c, int pos) {
	LWTEXTBLOCK text_block;
	text_block.align = LTBA_LEFT_TOP;
	text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
	text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
	text_block.size = DEFAULT_TEXT_BLOCK_SIZE_E;
	text_block.multiline = 1;
	SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);


	

	float left_top_x = 0;
	float left_top_y = 0;

	float area_width = 0;
	float area_height = 0;

	get_player_creature_ui_box(pos, pLwc->viewport_aspect_ratio, &left_top_x, &left_top_y, &area_width, &area_height);

	const float block_x_margin = 0.075f * pLwc->viewport_aspect_ratio;
	const float block_y_margin = 0.025f;

	const float bar_width = area_width - block_x_margin * 2;
	const float bar_height = 0.03f;

	if (c->selected) {
		render_solid_vb_ui(
			pLwc,
			left_top_x,
			left_top_y,
			area_width,
			area_height,
			pLwc->tex_programmed[LPT_BOTH_END_GRADIENT_HORIZONTAL],
			LVT_LEFT_TOP_ANCHORED_SQUARE,
			c->selected_a,
			c->selected_r,
			c->selected_g,
			c->selected_b,
			0.5f);
	}

	// Lv.
	{
		char str[32];
		sprintf(str, "Lv.%d", c->lv);
		text_block.text = str;
		text_block.text_bytelen = (int)strlen(text_block.text);
		text_block.begin_index = 0;
		text_block.end_index = text_block.text_bytelen;
		text_block.text_block_x = left_top_x + block_x_margin;
		text_block.text_block_y = left_top_y - block_y_margin;

		render_text_block(pLwc, &text_block);
	}

	// Name
	{
		text_block.text = c->name;
		text_block.text_bytelen = (int)strlen(text_block.text);
		text_block.begin_index = 0;
		text_block.end_index = text_block.text_bytelen;
		text_block.text_block_y -= text_block.text_block_line_height;

		render_text_block(pLwc, &text_block);
	}

	// Turn Token
	if (c->turn_token > 0) {
		const float turn_token_size = area_width / 10;
		const float turn_token_x = left_top_x + area_width - block_x_margin - turn_token_size / 2;
		const float turn_token_y = left_top_y - block_y_margin - turn_token_size / 2;

		if (!c->turn_consumed && c->selected) {
            lw_load_tex(pLwc, LAE_U_GLOW);
            lw_load_tex(pLwc, LAE_U_GLOW_ALPHA);
			render_solid_vb_ui_alpha(
				pLwc,
				turn_token_x,
				turn_token_y,
				turn_token_size * 3.0f,
				turn_token_size * 3.0f,
				pLwc->tex_atlas[LAE_U_GLOW],
				pLwc->tex_atlas[LAE_U_GLOW_ALPHA],
				LVT_CENTER_CENTER_ANCHORED_SQUARE,
				0.2f + (float)fabs(0.5f * sinf((float)pLwc->app_time * 3)),
				0,
				1,
				0,
				1
			);
		}

		render_solid_vb_ui(
			pLwc,
			turn_token_x,
			turn_token_y,
			turn_token_size,
			turn_token_size,
			pLwc->tex_programmed[c->turn_consumed ? LPT_SOLID_GRAY : LPT_SOLID_GREEN],
			LVT_CENTER_CENTER_ANCHORED_SQUARE,
			0.8f,
			0,
			0,
			0,
			0
		);


		LWTEXTBLOCK turn_token_text_block = text_block;
		char str[32];
		sprintf(str, "%d", c->turn_token);
		turn_token_text_block.text = str;
		turn_token_text_block.text_bytelen = (int)strlen(str);
		turn_token_text_block.begin_index = 0;
		turn_token_text_block.end_index = turn_token_text_block.text_bytelen;
		turn_token_text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
		turn_token_text_block.text_block_x = left_top_x + area_width - block_x_margin - turn_token_size + turn_token_size / 2;
		turn_token_text_block.text_block_y = left_top_y - block_y_margin;
		turn_token_text_block.multiline = 1;
		SET_COLOR_RGBA_FLOAT(turn_token_text_block.color_normal_glyph, 0, 0, 0, 1);
		SET_COLOR_RGBA_FLOAT(turn_token_text_block.color_normal_outline, 0, 0, 0, 0);
		SET_COLOR_RGBA_FLOAT(turn_token_text_block.color_emp_glyph, 1, 1, 0, 1);
		SET_COLOR_RGBA_FLOAT(turn_token_text_block.color_emp_outline, 0, 0, 0, 1);
		render_text_block(pLwc, &turn_token_text_block);
	}

	text_block.text_block_y -= text_block.text_block_line_height;

	vec2 shake_diff_pos = { 0, };
	const float shake_magnitude = c->shake_duration > 0 ? c->shake_magitude : 0;

	// offset by shake anim
	if (shake_magnitude) {
		shake_diff_pos[0] += (float)rand() / RAND_MAX * shake_magnitude;
		shake_diff_pos[1] += (float)rand() / RAND_MAX * shake_magnitude;
	}

	// HP
	{
		LWTEXTBLOCK hp_text_block;
		memcpy(&hp_text_block, &text_block, sizeof(LWTEXTBLOCK));

		char str[32];
		sprintf(str, "HP %d", c->hp);
		hp_text_block.text = str;
		hp_text_block.text_bytelen = (int)strlen(hp_text_block.text);
		hp_text_block.begin_index = 0;
		hp_text_block.end_index = hp_text_block.text_bytelen;
		hp_text_block.text_block_x += shake_diff_pos[0];
		hp_text_block.text_block_y += shake_diff_pos[1];
		hp_text_block.multiline = 1;
		render_text_block(pLwc, &hp_text_block);
	}

	render_solid_box_ui(
		pLwc,
		text_block.text_block_x + shake_diff_pos[0],
		text_block.text_block_y + shake_diff_pos[1] - text_block.text_block_line_height,
		bar_width,
		bar_height,
		pLwc->tex_programmed[LPT_SOLID_GRAY]
	);

	render_solid_box_ui(
		pLwc,
		text_block.text_block_x + shake_diff_pos[0],
		text_block.text_block_y + shake_diff_pos[1] - text_block.text_block_line_height,// + 0.01f,
		bar_width * c->hp / c->max_hp,
		bar_height,
		pLwc->tex_programmed[LPT_SOLID_RED]
	);

	// MP
	{
		char str[32];
		sprintf(str, "MP %d", c->mp);
		text_block.text = str;
		text_block.text_bytelen = (int)strlen(text_block.text);
		text_block.begin_index = 0;
		text_block.end_index = text_block.text_bytelen;
		text_block.text_block_y -= text_block.text_block_line_height;

		render_text_block(pLwc, &text_block);
	}

	render_solid_box_ui(
		pLwc,
		text_block.text_block_x,
		text_block.text_block_y - text_block.text_block_line_height,
		bar_width,
		bar_height,
		pLwc->tex_programmed[LPT_SOLID_GRAY]
	);

	int display_consume_mp = 0;
	if (c->selected
		&& c->skill[pLwc->selected_command_slot]
		&& c->skill[pLwc->selected_command_slot]->consume_mp > 0
		&& pLwc->battle_state != LBS_COMMAND_IN_PROGRESS) {
		display_consume_mp = c->skill[pLwc->selected_command_slot]->consume_mp;

		render_solid_box_ui_alpha(
			pLwc,
			text_block.text_block_x + bar_width * LWMAX(0, c->mp - display_consume_mp) / c->max_mp,
			text_block.text_block_y - text_block.text_block_line_height,// + 0.01f,
			bar_width * display_consume_mp / c->max_mp,
			bar_height,
			pLwc->tex_programmed[LPT_SOLID_YELLOW],
			(float)(0.3 + 0.7 * fabs(sin(pLwc->app_time * 5)))
		);
	}

	render_solid_box_ui(
		pLwc,
		text_block.text_block_x,
		text_block.text_block_y - text_block.text_block_line_height,// + 0.01f,
		bar_width * LWMAX(0, c->mp - display_consume_mp) / c->max_mp,
		bar_height,
		pLwc->tex_programmed[LPT_SOLID_BLUE]
	);
}

static void render_command_banner(const LWCONTEXT* pLwc) {

	const char* skill_name = 0;

	// Player turn
	if (pLwc->player_turn_creature_index >= 0
		&& pLwc->selected_command_slot >= 0
		&& pLwc->battle_state == LBS_COMMAND_IN_PROGRESS) {

		skill_name = pLwc->player[pLwc->player_turn_creature_index].skill[pLwc->selected_command_slot]->name;
	}

	// Enemy turn
	if (pLwc->enemy_turn_creature_index >= 0
		&& pLwc->selected_command_slot >= 0
		&& pLwc->battle_state == LBS_ENEMY_COMMAND_IN_PROGRESS) {

		skill_name = pLwc->enemy[pLwc->enemy_turn_creature_index].c.skill[pLwc->selected_command_slot]->name;
	}

	if (skill_name) {
		const float x = 0;
		const float y = -0.5f;
		const float w = 1.0f * pLwc->viewport_aspect_ratio;
		const float h = 0.2f;

		const float anim_v = LWCLAMP(5 * lwanim_get_1d(&pLwc->command_banner_anim), 0, 1);
		const float bg_alpha = 0.5f;

		render_solid_vb_ui(
			pLwc,
			x,
			y,
			w,
			h,
			pLwc->tex_programmed[LPT_BOTH_END_GRADIENT_HORIZONTAL],
			LVT_CENTER_CENTER_ANCHORED_SQUARE,
			bg_alpha * anim_v,
			0,
			0,
			0,
			0
		);

		LWTEXTBLOCK text_block = { 0, };
		text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
		text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_B;
		text_block.size = anim_v * DEFAULT_TEXT_BLOCK_SIZE_B;
		SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 0, 0, 0, 1);
		SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 1, 1, 1, 1);
		SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
		SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
		text_block.text = skill_name;
		text_block.text_bytelen = (int)strlen(text_block.text);
		text_block.begin_index = 0;
		text_block.end_index = text_block.text_bytelen;
		text_block.text_block_x = x;
		text_block.text_block_y = y;
		text_block.align = LTBA_CENTER_CENTER;
		text_block.multiline = 1;
		render_text_block(pLwc, &text_block);
	}
}

static void render_command_palette(const LWCONTEXT* pLwc) {
	if (pLwc->player_turn_creature_index < 0) {
		return;
	}

	

	const float screen_width = 2 * pLwc->viewport_aspect_ratio;
	const float screen_height = 2;
	const int command_slot_margin_count = 2;
	const int command_slot_count = 6;
	const float command_slot_width = screen_width / (command_slot_count + 2 * command_slot_margin_count);
	const float command_slot_height = screen_height / 4;
	const float command_desc_height = command_slot_height / 4;

	const int max_command_in_palette = 6;

	const LWSKILL *const *const skill = pLwc->player[pLwc->player_turn_creature_index].skill;

	const int selected_command_slot = pLwc->selected_command_slot;

	// command description
	if (skill[selected_command_slot] && skill[selected_command_slot]->valid) {
		// description bg
		render_solid_vb_ui(
			pLwc,
			0,
			-command_slot_height + command_desc_height / 2,
			command_slot_width * (max_command_in_palette + 2),
			command_desc_height,
			pLwc->tex_programmed[LPT_BOTH_END_GRADIENT_HORIZONTAL],
			LVT_CENTER_CENTER_ANCHORED_SQUARE,
			0.5f,
			26 / 255.0f,
			50 / 255.0f,
			74 / 255.0f,
			1.0f
		);

		LWTEXTBLOCK desc_text_block;
		desc_text_block.align = LTBA_LEFT_TOP;
		desc_text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
		desc_text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_D;
		desc_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_D;
		SET_COLOR_RGBA_FLOAT(desc_text_block.color_normal_glyph, 1, 1, 1, 1);
		SET_COLOR_RGBA_FLOAT(desc_text_block.color_normal_outline, 0, 0, 0, 1);
		SET_COLOR_RGBA_FLOAT(desc_text_block.color_emp_glyph, 1, 1, 0, 1);
		SET_COLOR_RGBA_FLOAT(desc_text_block.color_emp_outline, 0, 0, 0, 1);
		desc_text_block.text_block_x = -screen_width / 2 + command_slot_width * (0 + command_slot_margin_count);
		desc_text_block.text_block_y = -command_slot_height + command_desc_height;
		desc_text_block.text = skill[selected_command_slot]->desc;
		desc_text_block.text_bytelen = (int)strlen(desc_text_block.text);
		desc_text_block.begin_index = 0;
		desc_text_block.end_index = desc_text_block.text_bytelen;
		desc_text_block.multiline = 1;
		render_text_block(pLwc, &desc_text_block);
	}

	LWTEXTBLOCK cmd_text_block;
	cmd_text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
	cmd_text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_C;
	cmd_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_C;
	SET_COLOR_RGBA_FLOAT(cmd_text_block.color_normal_glyph, 1, 1, 1, 1);
	SET_COLOR_RGBA_FLOAT(cmd_text_block.color_normal_outline, 0, 0, 0, 1);
	SET_COLOR_RGBA_FLOAT(cmd_text_block.color_emp_glyph, 1, 1, 0, 1);
	SET_COLOR_RGBA_FLOAT(cmd_text_block.color_emp_outline, 0, 0, 0, 1);
	cmd_text_block.text_block_y = -command_slot_height;
	cmd_text_block.align = LTBA_LEFT_TOP;
	cmd_text_block.multiline = 1;
	// command list
	for (int i = 0; i < max_command_in_palette; i++) {

		cmd_text_block.text_block_x = -screen_width / 2 + command_slot_width * (i + command_slot_margin_count);

		if (skill[i] && skill[i]->valid) {

			if (selected_command_slot == i) {
				// Selected bg
				render_solid_vb_ui(
					pLwc,
					cmd_text_block.text_block_x + command_slot_width / 2,
					-command_slot_height - command_slot_height / 2,
					command_slot_width * 1.1f + (command_slot_width * 0.1f * (float)fabs(sinf((float)pLwc->app_time * 5))),
					(screen_height / 2 - command_slot_height),
					pLwc->tex_programmed[LPT_BOTH_END_GRADIENT_HORIZONTAL],
					LVT_CENTER_CENTER_ANCHORED_SQUARE,
					0.5f,
					48 / 255.0f,
					93 / 255.0f,
					138 / 255.0f,
					1.0f
				);
			}

			// Skill name
			cmd_text_block.text = skill[i]->name;
			cmd_text_block.text_bytelen = (int)strlen(cmd_text_block.text);
			cmd_text_block.begin_index = 0;
			cmd_text_block.end_index = cmd_text_block.text_bytelen;
			cmd_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_C;
			cmd_text_block.text_block_y = -command_slot_height;
			render_text_block(pLwc, &cmd_text_block);

			// MP consumption
			if (skill[i]->consume_mp > 0) {
				char str[64];
				sprintf(str, "MP %d", skill[i]->consume_mp);
				cmd_text_block.text = str;
				cmd_text_block.text_bytelen = (int)strlen(cmd_text_block.text);
				cmd_text_block.begin_index = 0;
				cmd_text_block.end_index = cmd_text_block.text_bytelen;
				cmd_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_E;
				cmd_text_block.text_block_y -= cmd_text_block.text_block_line_height;

				render_text_block(pLwc, &cmd_text_block);
			}
		}
	}
}

void render_center_image(const LWCONTEXT* pLwc) {
	if (pLwc->center_image_anim.t > 0) {
		const float s = LWCLAMP(5 * lwanim_get_1d(&pLwc->center_image_anim), 0, 1);
		if (s > 0) {
            lw_load_tex(pLwc, pLwc->center_image);
            lw_load_tex(pLwc, pLwc->center_image + 1);
			render_solid_vb_ui_alpha(pLwc,
                                     0,
                                     0,
                                     s,
                                     s,
                                     pLwc->tex_atlas[pLwc->center_image],
                                     pLwc->tex_atlas[pLwc->center_image + 1],
                                     LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                     1,
                                     0,
                                     0,
                                     0,
                                     0);
		}
	}
}

void lwc_render_battle(const LWCONTEXT* pLwc) {
	LW_GL_VIEWPORT();
	lw_clear_color();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Result curtain
	//mat4x4 identity;
	//mat4x4_identity(identity);


	int shader_index = LWST_DEFAULT;

	lazy_glUseProgram(pLwc, shader_index);

	glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, default_uv_offset);
	glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, default_uv_scale);
	glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, 1);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(pLwc->shader[shader_index].diffuse_location, 0); // 0 means GL_TEXTURE0

	// default texture param
	lazy_tex_atlas_glBindTexture(pLwc, pLwc->tex_atlas_index);


	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)pLwc->proj);

	

	int only_render_ui = 0;

	if (only_render_ui) {
		// Make alpha to zero across the entire screen (testing)
		render_solid_vb_ui(
			pLwc,
			0,
			0,
			2 * pLwc->viewport_aspect_ratio,
			2,
			pLwc->tex_programmed[LPT_SOLID_TRANSPARENT],
			LVT_CENTER_CENTER_ANCHORED_SQUARE,
			1.0f,
			0,
			0,
			0,
			0
		);
	}

	// render background & enemy
	if (!only_render_ui) {

		// background
		render_battle_twirl(pLwc, pLwc->battle_view, pLwc->battle_proj);

		ARRAY_ITERATE_VALID(const LWENEMY, pLwc->enemy) {
			float overlay[5];
			lwanim_get_5d(&e->death_anim, overlay);

			render_enemy_3d(pLwc,
				e,
				pLwc->battle_state == LBS_SELECT_TARGET && pLwc->selected_enemy_slot == i,
				overlay,
				pLwc->battle_view,
				pLwc->battle_proj
			);
		} ARRAY_ITERATE_VALID_END();
	}

	// Attack trail
	ARRAY_ITERATE_VALID(const LWTRAIL, pLwc->trail) {
		render_attack_trail_3d(pLwc, e, pLwc->battle_view, pLwc->battle_proj);
	} ARRAY_ITERATE_VALID_END();

	// Player battle creature UI
	ARRAY_ITERATE_VALID(const LWBATTLECREATURE, pLwc->player) {
		render_player_creature_ui(pLwc, e, i);
	} ARRAY_ITERATE_VALID_END();

	// Attack damage_text
	render_damage_text(pLwc, pLwc->battle_view, pLwc->battle_proj, pLwc->proj, 1.0f);

	// Command palette
	if (pLwc->battle_state != LBS_COMMAND_IN_PROGRESS) {
		render_command_palette(pLwc);
	}

	// Command banner
	render_command_banner(pLwc);

	render_center_image(pLwc);

	// give up const-ness
	((LWCONTEXT*)pLwc)->render_count++;
}
