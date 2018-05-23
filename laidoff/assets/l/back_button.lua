local c = lo.script_context()
return function ()
	print('[script]back_button')
	c.eye_x_offset = 0
	c.puck_game.follow_cam = 0
	lo.puck_game_reset_view_proj(c, c.puck_game)
	lo.puck_game_set_tower_invincible(c.puck_game, 0, 0)
	lo.puck_game_set_tower_invincible(c.puck_game, 1, 0)
	lo.puck_game_set_dash_disabled(c.puck_game, 1, 0)
	lo.puck_game_set_bogus_disabled(c.puck_game, 0)
	lo.lwcontext_set_custom_puck_game_stage(c, lo.LVT_DONTCARE, lo.LAE_DONTCARE)
	if c.puck_game.game_state == lo.LPGS_SEARCHING then
		lo.tcp_send_cancelqueue(c.tcp, c.tcp.user_id)
		c.puck_game.game_state = lo.LPGS_MAIN_MENU
	elseif c.game_scene == lo.LGS_LEADERBOARD then
		lo.request_player_reveal_leaderboard(c.tcp)
		lo.change_to_physics(c)
	elseif c.puck_game.game_state == lo.LPGS_TUTORIAL then
		lo.script_cleanup_all_coros(c)
		lo.lw_go_back(c, c.android_native_activity)
	elseif c.puck_game.game_state == lo.LPGS_BATTLE then
		-- refresh leaderboard
		lo.request_player_reveal_leaderboard(c.tcp)
		lo.lw_go_back(c, c.android_native_activity)
	else
		lo.lw_go_back(c, c.android_native_activity)
	end
end
