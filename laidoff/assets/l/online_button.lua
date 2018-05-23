local T = require('strings_en')
local c = lo.script_context()
return function (queue_type)
	print('[script]online_button')
	c.puck_game.game_state = lo.LPGS_SEARCHING
	lo.puck_game_set_searching_str(c.puck_game, T['상대방을 찾는 중...'])
	lo.puck_game_clear_match_data(c, c.puck_game)
	lo.puck_game_reset_battle_state(c.puck_game)
	lo.tcp_send_queue3(c.tcp, c.tcp.user_id, queue_type)
	lo.lwcontext_set_custom_puck_game_stage(c, lo.LVT_DONTCARE, lo.LAE_DONTCARE)
	lo.puck_game_set_static_default_values(c.puck_game)
	lo.puck_game_set_static_default_values_client(c.puck_game)
end
