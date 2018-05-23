#pragma once

#include "vertices.h"
#include "linmath.h"
#include "lwbattlecreature.h"
#include "lwsimpleanim.h"
#include "lwatlassprite.h"
#include "lwmacro.h"

typedef struct _LWENEMY
{
	int valid;
	enum _LW_ATLAS_SPRITE las;
	float scale;
	float shadow_scale;
	float time_offset;
	LWANIM1D evasion_anim;
	LWANIM5D death_anim;
	LWBATTLECREATURE c;

	vec2 left_top_ui_point;
	vec2 right_bottom_ui_point;

	vec3 render_pos;
} LWENEMY;

#define ENEMY_ICECREAM { 1, LAS_ICECREAM, 0.15f, 0.1f, 0.3f, 0.f, 0.f, {0}, {{0}}, BATTLECREATURE_ICECREAM }
#define ENEMY_HANNIBAL { 1, LAS_HANNIBAL, 0.3f, 0.15f, 0.2f, 0.f, 0.f, {0}, {{0}}, BATTLECREATURE_HANNIBAL }
#define ENEMY_KEYBOARD { 1, LAS_ICECREAM, 0.3f, 0.1f, 0.3f, 0.f, 0.f, {0}, {{0}}, BATTLECREATURE_KEYBOARD }
#define ENEMY_FISH { 1, LAS_HANNIBAL, 0.3f, 0.15f, 0.2f, 0.f, 0.f, {0}, {{0}}, BATTLECREATURE_FISH }
#define ENEMY_ANIMAL { 1, LAS_ICECREAM, 0.3f, 0.1f, 0.3f, 0.f, 0.f, {0}, {{0}}, BATTLECREATURE_ANIMAL }
#define ENEMY_TEST_PLAYER_1 { 1, LAS_ICECREAM, 0.3f, 0.1f, 0.3f, 0.f, 0.f, {0}, {{0}}, BATTLECREATURE_BIKER }
#define ENEMY_TEST_PLAYER_2 { 1, LAS_ICECREAM, 0.3f, 0.1f, 0.3f, 0.f, 0.f, {0}, {{0}}, BATTLECREATURE_MADAM }
#define ENEMY_TEST_PLAYER_3 { 1, LAS_ICECREAM, 0.3f, 0.1f, 0.3f, 0.f, 0.f, {0}, {{0}}, BATTLECREATURE_TREE }
#define ENEMY_TEST_PLAYER_4 { 1, LAS_ICECREAM, 0.3f, 0.1f, 0.3f, 0.f, 0.f, {0}, {{0}}, BATTLECREATURE_TOFU }

typedef enum LW_ENEMY_TYPE {
	LET_ICECREAM,
	LET_HANNIBAL,
	LET_KEYBOARD,
	LET_FISH,
	LET_ANIMAL,
	LET_TEST_PLAYER_1,
	LET_TEST_PLAYER_2,
	LET_TEST_PLAYER_3,
	LET_TEST_PLAYER_4,

	LET_COUNT,
} LW_ENEMY_TYPE;

extern const LWENEMY ENEMY_DATA_LIST[LET_COUNT];

LwStaticAssert(ARRAY_SIZE(ENEMY_DATA_LIST) == LET_COUNT, "LET_COUNT error");

struct _LWCONTEXT;

void update_enemy(const LWCONTEXT* pLwc, int enemy_slot_index, LWENEMY* enemy);
float get_battle_enemy_x_center(int enemy_slot_index);
void update_render_enemy_position(const LWCONTEXT* pLwc, int enemy_slot_index, const LWENEMY* enemy, vec3 pos);
