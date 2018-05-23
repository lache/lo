#pragma once

typedef enum _LW_SKILL_EFFECT {
	// 공격에 의한 HP 감소
	LSE_DAMAGE,
	// 회복에 의한 HP 증가
	LSE_HEAL,
	// 구속 이상 상태 부여
	LSE_DEBUFF_CONFINE,
	// 수면 이상 상태 부여
	LSE_DEBUFF_SLEEP,
	// 공격력 임시 강화
	LSE_BUFF_ATTACK,
	// 방어력 임시 강화
	LSE_BUFF_DEFENCE,
	// 공격력 임시 약화
	LSE_DEBUFF_ATTACK,
	// 방어력 임시 약화
	LSE_DEBUFF_DEFENCE,
	// 스킬 사용에 의한 HP 감소
	LSE_SKILL_CONSUME_HP,
	// 스킬 사용에 의한 MP 감소
	LSE_SKILL_CONSUME_MP,
} LW_SKILL_EFFECT;
