#pragma once

typedef enum _LW_SKILL_EFFECT {
	// ���ݿ� ���� HP ����
	LSE_DAMAGE,
	// ȸ���� ���� HP ����
	LSE_HEAL,
	// ���� �̻� ���� �ο�
	LSE_DEBUFF_CONFINE,
	// ���� �̻� ���� �ο�
	LSE_DEBUFF_SLEEP,
	// ���ݷ� �ӽ� ��ȭ
	LSE_BUFF_ATTACK,
	// ���� �ӽ� ��ȭ
	LSE_BUFF_DEFENCE,
	// ���ݷ� �ӽ� ��ȭ
	LSE_DEBUFF_ATTACK,
	// ���� �ӽ� ��ȭ
	LSE_DEBUFF_DEFENCE,
	// ��ų ��뿡 ���� HP ����
	LSE_SKILL_CONSUME_HP,
	// ��ų ��뿡 ���� MP ����
	LSE_SKILL_CONSUME_MP,
} LW_SKILL_EFFECT;
