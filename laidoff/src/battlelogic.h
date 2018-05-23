#pragma once

struct _LWBATTLECREATURE;
struct _LWBATTLECOMMAND;
struct _LWBATTLECOMMANDRESULT;

void calculate_battle_command_result(
	const struct _LWBATTLECREATURE* ca,
	const struct _LWBATTLECREATURE* cb,
	const struct _LWBATTLECOMMAND* cmd,
	struct _LWBATTLECOMMANDRESULT* cmd_result_a,
	struct _LWBATTLECOMMANDRESULT* cmd_result_b);

void apply_battle_command_result(
	struct _LWBATTLECREATURE* c,
	struct _LWBATTLECOMMANDRESULT* cmd_result);
