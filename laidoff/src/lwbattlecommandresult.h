#pragma once

#include "lwskilleffect.h"

// 전투 커맨드 결과 (실행자 입장으로 용어 통일)
typedef enum _LW_BATTLE_COMMAND_RESULT {
	// 아무 효과 없음
	LBCR_NULL_RESULT,
	// 일반 성공
	LBCR_NORMAL,
	// 대박 성공
	LBCR_LUCKY,
	// 빗나감
	LBCR_MISSED,
	// 면역 반응
	LBCR_IMMUNE_RESPONSE,
	// 약점 반응
	LBCR_WEAK_RESPONSE,
	// HP 부족으로 커맨드 실행 실패
	LBCR_NOT_ENOUGH_HP,
	// MP 부족으로 커맨드 실행 실패
	LBCR_NOT_ENOUGH_MP,
} LW_BATTLE_COMMAND_RESULT;

typedef struct _LWBATTLECOMMANDRESULT {
	LW_BATTLE_COMMAND_RESULT type;
	int delta_hp;
	int delta_mp;
	LW_SKILL_EFFECT skill_effect;
} LWBATTLECOMMANDRESULT;
