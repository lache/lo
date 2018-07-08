#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "battlelogic.h"
#include "lwbattlecommand.h"
#include "lwbattlecreature.h"
#include "lwbattlecommandresult.h"
#include "lwmacro.h"

int is_immune(const LWSKILL* s, const LWBATTLECREATURE* c) {
    return (s->attrib.bits.air && (c->stat.attrib.bits.air == LW_ATTRIB_DEFENCE_IMMU))
        || (s->attrib.bits.wat && (c->stat.attrib.bits.wat == LW_ATTRIB_DEFENCE_IMMU))
        || (s->attrib.bits.fir && (c->stat.attrib.bits.fir == LW_ATTRIB_DEFENCE_IMMU))
        || (s->attrib.bits.ear && (c->stat.attrib.bits.ear == LW_ATTRIB_DEFENCE_IMMU))
        || (s->attrib.bits.god && (c->stat.attrib.bits.god == LW_ATTRIB_DEFENCE_IMMU))
        || (s->attrib.bits.evl && (c->stat.attrib.bits.evl == LW_ATTRIB_DEFENCE_IMMU));
}

// 스킬과 크리처 간 친숙도: <매우 친숙하지 않음> [-6, 12] ==> [0, 18] <0: 매우 친숙하지 않음, 6: 중립, 18: 매우 친숙>
int affinity_value(const LWSKILL* s, const LWBATTLECREATURE* c) {
    int value = 0;

    value += 2 * (s->attrib.bits.air && (c->stat.attrib.bits.air == LW_ATTRIB_DEFENCE_IMMU));
    value += 2 * (s->attrib.bits.wat && (c->stat.attrib.bits.wat == LW_ATTRIB_DEFENCE_IMMU));
    value += 2 * (s->attrib.bits.fir && (c->stat.attrib.bits.fir == LW_ATTRIB_DEFENCE_IMMU));
    value += 2 * (s->attrib.bits.ear && (c->stat.attrib.bits.ear == LW_ATTRIB_DEFENCE_IMMU));
    value += 2 * (s->attrib.bits.god && (c->stat.attrib.bits.god == LW_ATTRIB_DEFENCE_IMMU));
    value += 2 * (s->attrib.bits.evl && (c->stat.attrib.bits.evl == LW_ATTRIB_DEFENCE_IMMU));

    value += s->attrib.bits.air && (c->stat.attrib.bits.air == LW_ATTRIB_DEFENCE_RESI);
    value += s->attrib.bits.wat && (c->stat.attrib.bits.wat == LW_ATTRIB_DEFENCE_RESI);
    value += s->attrib.bits.fir && (c->stat.attrib.bits.fir == LW_ATTRIB_DEFENCE_RESI);
    value += s->attrib.bits.ear && (c->stat.attrib.bits.ear == LW_ATTRIB_DEFENCE_RESI);
    value += s->attrib.bits.god && (c->stat.attrib.bits.god == LW_ATTRIB_DEFENCE_RESI);
    value += s->attrib.bits.evl && (c->stat.attrib.bits.evl == LW_ATTRIB_DEFENCE_RESI);

    value -= s->attrib.bits.air && (c->stat.attrib.bits.air == LW_ATTRIB_DEFENCE_WEAK);
    value -= s->attrib.bits.wat && (c->stat.attrib.bits.wat == LW_ATTRIB_DEFENCE_WEAK);
    value -= s->attrib.bits.fir && (c->stat.attrib.bits.fir == LW_ATTRIB_DEFENCE_WEAK);
    value -= s->attrib.bits.ear && (c->stat.attrib.bits.ear == LW_ATTRIB_DEFENCE_WEAK);
    value -= s->attrib.bits.god && (c->stat.attrib.bits.god == LW_ATTRIB_DEFENCE_WEAK);
    value -= s->attrib.bits.evl && (c->stat.attrib.bits.evl == LW_ATTRIB_DEFENCE_WEAK);

    return 6 + value;
}

// 레벨 차이에 의한 보정치 상수
const static float c_lvdiff = 0.1f;

// 친밀도 상수
const static float c_maxafi = 16.0f;

// ca가 cb에게 cmd 커맨드를 적용시킬 때의 스킬 친숙도에 대한 보정치
float affinity(
    const struct _LWBATTLECREATURE* ca,
    const struct _LWBATTLECREATURE* cb,
    const struct _LWBATTLECOMMAND* cmd) {

    return (affinity_value(cmd->skill, ca) - affinity_value(cmd->skill, cb)) / c_maxafi;
}

// 운 함수에 의한 보정치 [-1, 1]
float luck(float x) {
    return LWCLAMP(powf(x - 1.0f, 5.0f), -1.0f, 1.0f);
}

float phy_success_value(int a_agl, int b_agl, int s_sop, float luck_factor) {
    return LWCLAMP((float)a_agl / (s_sop + b_agl) + luck_factor, 0.0f, 1.0f);
}

float mag_success_value(int a_spd, int b_spd, int s_sop, float luck_factor) {
    return LWCLAMP((float)a_spd / (s_sop + b_spd) + luck_factor, 0.0f, 1.0f);
}

float eff_success_value(int a_spd, int b_spd, int s_dif, float affinity_factor, float luck_factor) {
    return LWCLAMP((float)a_spd / (s_dif + b_spd) * affinity_factor + luck_factor, 0.0f, 1.0f);
}

int calculate_physical_damage(
    const struct _LWBATTLECREATURE* ca,
    const struct _LWBATTLECREATURE* cb,
    const struct _LWBATTLECOMMAND* cmd) {

    const float damage = (1.0f + c_lvdiff * LWMAX(0, ca->lv - cb->lv) + luck((float)ca->stat.lck / ca->stat.lck))
        * cmd->skill->pow
        * (ca->stat.str / ca->stat.str);

    return (int)damage;
}

int calculate_magical_damage(
    const struct _LWBATTLECREATURE* ca,
    const struct _LWBATTLECREATURE* cb,
    const struct _LWBATTLECOMMAND* cmd) {

    const float affinity_factor = affinity(ca, cb, cmd);

    const float damage = (1.0f + c_lvdiff * LWMAX(0, ca->lv - cb->lv) + affinity_factor + luck((float)ca->stat.lck / ca->stat.lck))
        * cmd->skill->pow
        * (ca->stat.mag / ca->stat.mag);

    return (int)damage;
}

void calculate_battle_command_result(
    const struct _LWBATTLECREATURE* ca,
    const struct _LWBATTLECREATURE* cb,
    const struct _LWBATTLECOMMAND* cmd,
    struct _LWBATTLECOMMANDRESULT* cmd_result_a,
    struct _LWBATTLECOMMANDRESULT* cmd_result_b) {

    memset(cmd_result_a, 0, sizeof(LWBATTLECOMMANDRESULT));
    memset(cmd_result_b, 0, sizeof(LWBATTLECOMMANDRESULT));

    if (cmd->skill->consume_hp > ca->hp) {
        cmd_result_a->type = LBCR_NOT_ENOUGH_HP;
        cmd_result_b->type = LBCR_NULL_RESULT;
        return;
    }

    if (cmd->skill->consume_mp > ca->mp) {
        cmd_result_a->type = LBCR_NOT_ENOUGH_MP;
        cmd_result_b->type = LBCR_NULL_RESULT;
        return;
    }

    cmd_result_a->delta_hp -= cmd->skill->consume_hp;
    cmd_result_a->delta_mp -= cmd->skill->consume_mp;

    const int immune = is_immune(cmd->skill, cb);

    if (immune) {
        cmd_result_b->delta_hp = 0;
        cmd_result_b->delta_hp = 0;

        cmd_result_a->type = LBCR_IMMUNE_RESPONSE;
        cmd_result_b->type = LBCR_NULL_RESULT;
    }
    else
    {
        int physical_damage = 0;
        int magical_damage = 0;
        if (cmd->skill->attrib.value == 0) {
            physical_damage = calculate_physical_damage(ca, cb, cmd);
        }
        else {
            magical_damage = calculate_magical_damage(ca, cb, cmd);
        }


        const float lck_factor = luck((float)ca->stat.lck / cb->stat.lck);
        //const float aff_factor = affinity(ca, cb, cmd);

        const float r = (float)rand() / RAND_MAX;

        const float e_phy = phy_success_value(ca->stat.agl, cb->stat.agl, cmd->skill->sop, lck_factor);
        const float e_mag = mag_success_value(ca->stat.spd, cb->stat.spd, cmd->skill->dif, lck_factor);
        //const float e_eff = eff_success_value(ca->stat.spd, cb->stat.spd, cmd->skill->dif, aff_factor, lck_factor);
        const float c_cri = 0.1f;

        const int phy_critical = r < e_phy * c_cri;
        const int phy_normal = r < e_phy;
        const int mag_critical = r < e_mag * c_cri;
        const int mag_normal = r < e_mag;

        cmd_result_b->delta_hp -= phy_critical ? (2 * physical_damage) : phy_normal ? physical_damage : 0;
        cmd_result_b->delta_hp -= mag_critical ? (2 * magical_damage) : mag_normal ? magical_damage : 0;

        cmd_result_a->type = (phy_critical || mag_critical) ? LBCR_LUCKY : (phy_normal || mag_normal) ? LBCR_NORMAL : LBCR_MISSED;
        cmd_result_b->type = LBCR_NORMAL;
    }
}

void apply_battle_command_result(
    struct _LWBATTLECREATURE* c,
    struct _LWBATTLECOMMANDRESULT* cmd_result) {

    c->hp += cmd_result->delta_hp;
    c->mp += cmd_result->delta_mp;
}
