#include "lwskill.h"

const LWSKILL SKILL_DATA_LIST[] = {
    { 1, LWU("공격"), LWU("Brown fox qq l 적 하나를 물리 속성으로 공격합니다."), 1,{ LW_NON_WEAK }, 1, 1, 1, LST_ENEMY_1, LSE_DAMAGE, 0, 0 },
    { 1, LWU("광풍"), LWU("적 하나를 공기 속성으로 공격합니다."), 1,{ LW_AIR_IMMU }, 1, 100, 100, LST_ENEMY_1, LSE_DAMAGE, 0, 2 },
    { 1, LWU("파도"), LWU("적 하나를 물 속성으로 공격합니다."), 1,{ LW_WAT_IMMU }, 1, 1, 1, LST_ENEMY_1, LSE_DAMAGE, 0, 2 },
    { 1, LWU("방화"), LWU("적 하나를 불 속성으로 공격합니다."), 1,{ LW_FIR_IMMU }, 1, 1, 1, LST_ENEMY_1, LSE_DAMAGE, 0, 2 },
    { 1, LWU("지진"), LWU("적 하나를 땅 속성으로 공격합니다."), 1,{ LW_EAR_IMMU }, 1, 1, 1, LST_ENEMY_1, LSE_DAMAGE, 0, 2 },
    { 1, LWU("광명"), LWU("적 하나를 선 속성으로 공격합니다."), 1,{ LW_GOD_IMMU }, 1, 1, 1, LST_ENEMY_1, LSE_DAMAGE, 0, 8 },
    { 1, LWU("저주"), LWU("적 하나를 악 속성으로 공격합니다."), 1,{ LW_EVL_IMMU }, 1, 1, 1, LST_ENEMY_1, LSE_DAMAGE, 0, 8 },
    { 1, LWU("치료"), LWU("우리편 하나의 HP를 회복합니다."), 1,{ LW_NON_WEAK }, 1, 1, 1, LST_ALLY_1, LSE_HEAL, 0, 10 },
    { 1, LWU("구속"), LWU("적 하나의 전투 행동을 방해합니다."), 1,{ LW_NON_WEAK }, 1, 1, 1, LST_ENEMY_1, LSE_DEBUFF_CONFINE, 0, 5 },
    { 1, LWU("수면가스"), LWU("적 하나를 잠들게 합니다."), 1,{ LW_NON_WEAK }, 1, 1, 1, LST_ENEMY_1, LSE_DEBUFF_SLEEP, 0, 5 },
    { 1, LWU("응원"), LWU("우리편 전체의 공격력을 강화시킵니다."), 1,{ LW_NON_WEAK }, 1, 1, 1, LST_ALLY_ALL, LSE_BUFF_ATTACK, 0, 17 },
    { 1, LWU("신중"), LWU("우리편 전체의 방어력을 강화시킵니다."), 1,{ LW_NON_WEAK }, 1, 1, 1, LST_ALLY_ALL, LSE_BUFF_DEFENCE, 0, 15 },
    { 1, LWU("공격교란"), LWU("적 전체의 공격력을 약화시킵니다."), 1,{ LW_NON_WEAK }, 1, 1, 1, LST_ENEMY_ALL, LSE_DEBUFF_ATTACK, 0, 19 },
    { 1, LWU("방어교란"), LWU("적 전체의 방어력을 약화시킵니다."), 1,{ LW_NON_WEAK }, 1, 1, 1, LST_ENEMY_ALL, LSE_DEBUFF_DEFENCE, 0, 17 },
};
