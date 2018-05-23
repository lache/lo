#pragma once

#include "unicode.h"
#include "lwattrib.h"
#include "lwskilleffect.h"

typedef enum _LW_SKILL_TARGET {
	LST_ENEMY_1,
	LST_ENEMY_ALL,
	LST_ALLY_1,
	LST_ALLY_ALL,
} LW_SKILL_TARGET;

typedef struct _LWSKILL {
	int valid;
	const char* name;
	const char* desc;
	int lv;
	LWATTRIBVALUE attrib;
	int pow;
	int sop;
	int dif;
	LW_SKILL_TARGET skill_target;
	LW_SKILL_EFFECT skill_effect;
	int consume_hp;
	int consume_mp;
} LWSKILL;

typedef enum _LW_SKILL_DATA {
	LSD_01,
	LSD_02,
	LSD_03,
	LSD_04,
	LSD_05,
	LSD_06,
	LSD_07,
	LSD_08,
	LSD_09,
	LSD_10,
	LSD_11,
	LSD_12,
	LSD_13,
	LSD_14,

	LSD_COUNT,
} LW_SKILL_DATA;

extern const LWSKILL SKILL_DATA_LIST[LSD_COUNT];
