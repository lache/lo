local T = require('strings_en')
local c = lo.script_context()
return function ()
	print('[script]tutorial_button')
	lo.puck_game_set_static_default_values(c.puck_game)
	lo.puck_game_set_static_default_values_client(c.puck_game)
	lo.puck_game_set_tower_invincible(c.puck_game, 0, 0)
	lo.puck_game_set_tower_invincible(c.puck_game, 1, 0)
	lo.puck_game_set_dash_disabled(c.puck_game, 1, 1)
	lo.puck_game_set_bogus_disabled(c.puck_game, 1)
	lo.puck_game_reset_tutorial_state(c.puck_game)
	lo.puck_game_clear_match_data(c, c.puck_game)
	lo.puck_game_reset_view_proj(c, c.puck_game)
	lo.puck_game_roll_to_tutorial(c.puck_game)
	lo.lwcontext_set_custom_puck_game_stage(c, lo.LVT_DONTCARE, lo.LAE_DONTCARE)
	lo.puck_game_create_walls(c.puck_game)
	start_coro(function()
		lo.puck_game_set_tutorial_guide_str(c.puck_game, '')
		-- restore to full HP
		lo.puck_game_player(c.puck_game, 0).current_hp = c.puck_game.hp;
		lo.puck_game_target(c.puck_game, 0).current_hp = c.puck_game.hp;
		-- hide pull and dash buttons
		-- (left dir pad not already hidden since UI alpha is 0)
		c.puck_game.control_flags = c.puck_game.control_flags | lo.LPGCF_HIDE_PULL_BUTTON | lo.LPGCF_HIDE_DASH_BUTTON
		lo.puck_game_set_tutorial_guide_str(c.puck_game, T['환영합니다!'])
		yield_wait_ms(1500)
		lo.puck_game_create_go(c.puck_game, lo.LPGO_PLAYER, 0, 0, 10, c.puck_game.player_sphere_radius);
		lo.puck_game_set_tutorial_guide_str(c.puck_game, T['왼쪽 <스틱>으로 플레이어를 움직여보세요.'])
		lo.puck_game_create_control_joint(c.puck_game, lo.LPGO_PLAYER)
		c.puck_game.battle_control_ui_alpha = 1
		yield_wait_ms(4000)
		lo.puck_game_create_go(c.puck_game, lo.LPGO_PUCK, 1, 0, 10, c.puck_game.puck_sphere_radius);
		--------------------------------------
		local near_count = 2
		local near_msg = T['흰색 공과 부딪쳐보세요. (%d/%d)']
		for i=1,near_count do
			lo.puck_game_set_tutorial_guide_str(c.puck_game, string.format(near_msg, i-1, near_count))
			if i ~= 1 then
				yield_wait_ms(1000)
			end
			yield_wait_near_puck_player(1)
		end
		lo.puck_game_set_tutorial_guide_str(c.puck_game, string.format(near_msg, near_count, near_count))
		yield_wait_ms(1000)
		lo.puck_game_set_tutorial_guide_str(c.puck_game, T['잘했습니다!'])
		yield_wait_ms(2000)
		--------------------------------------
		c.puck_game.control_flags = c.puck_game.control_flags & ~lo.LPGCF_HIDE_DASH_BUTTON
		local dash_near_count = 2
		local dash_near_msg = T['<대시>를 사용해 공과 부딪쳐보세요. (%d/%d)']
		for i=1,dash_near_count do
			lo.puck_game_set_tutorial_guide_str(c.puck_game, string.format(dash_near_msg, i-1, dash_near_count))
			if i ~= 1 then
				yield_wait_ms(1000)
			end
			yield_wait_near_puck_player_with_dash(1)
		end
		lo.puck_game_set_tutorial_guide_str(c.puck_game, string.format(dash_near_msg, dash_near_count, dash_near_count))
		yield_wait_ms(1000)
		lo.puck_game_set_tutorial_guide_str(c.puck_game, T['아주 잘했습니다!'])
		yield_wait_ms(2000)
		--------------------------------------
		lo.puck_game_set_tutorial_guide_str(c.puck_game, T['그럼 <타워>를 소환하겠습니다.'])
		-- make enemy tower invincible for now
		lo.puck_game_set_tower_invincible(c.puck_game, 1, 1)
		yield_wait_ms(2500)
		lo.puck_game_set_tutorial_guide_str(c.puck_game, T['왼쪽 하단에 생긴 것은 <아군 타워>입니다.'])
		lo.puck_game_create_tower_geom(c.puck_game, 0);
		yield_wait_ms(3500)
		lo.puck_game_set_tutorial_guide_str(c.puck_game, T['오른쪽 상단에 생긴 것은 <적군 타워>입니다.'])
		lo.puck_game_create_tower_geom(c.puck_game, 1);
		yield_wait_ms(3500)
		--------------------------------------
		lo.puck_game_set_tower_invincible(c.puck_game, 1, 0)
		local damage_count = 2
		local damage_msg = T['공으로 <적군 타워>에 데미지를 입히세요. (%d/%d)']
		for i=1,damage_count do
			lo.puck_game_set_tutorial_guide_str(c.puck_game, string.format(damage_msg, i-1, damage_count))
			yield_wait_player_attack(1)
		end
		-- make enemy tower invincible for now
		lo.puck_game_set_tower_invincible(c.puck_game, 1, 1)
		lo.puck_game_set_tutorial_guide_str(c.puck_game, string.format(damage_msg, damage_count, damage_count))
		yield_wait_ms(1000)
		lo.puck_game_set_tutorial_guide_str(c.puck_game, T['대단히 잘했습니다!'])
		yield_wait_ms(2000)
		--------------------------------------
		lo.puck_game_set_tutorial_guide_str(c.puck_game, T['마지막으로 <적 플레이어>을 소환하겠습니다.'])
		yield_wait_ms(2000)
		lo.puck_game_create_go(c.puck_game, lo.LPGO_TARGET, 1, 0, 10, c.puck_game.target_sphere_radius);
		-- at first, bogus does not use dash
		lo.puck_game_set_dash_disabled(c.puck_game, 1, 1)
		
		lo.puck_game_create_control_joint(c.puck_game, lo.LPGO_TARGET)
		yield_wait_ms(2000)
		lo.puck_game_set_tutorial_guide_str(c.puck_game, T['적의 방해를 피해 <적군 타워>를 파괴하십시오!'])
		lo.puck_game_set_bogus_disabled(c.puck_game, 0)
		-- make enemy tower can be damaged
		lo.puck_game_set_tower_invincible(c.puck_game, 1, 0)
		start_coro(function()
			yield_wait_ms(5000)
			lo.puck_game_set_dash_disabled(c.puck_game, 1, 0)
		end)
		local victory = 0
		-- remove guide text after some time passed
		start_coro(function()
			yield_wait_ms(4000)
			if victory == 0 then
				lo.puck_game_set_tutorial_guide_str(c.puck_game, '')
			end
		end)
		start_coro(function()
			yield_wait_target_attack(4)
			-- make player tower invincible at emergency
			lo.puck_game_set_tower_invincible(c.puck_game, 0, 1)
		end)
		yield_wait_player_attack(3)
		victory = 1
		lo.puck_game_set_tutorial_guide_str(c.puck_game, T['축하합니다! 실전에서도 건투를 빕니다.'])
		lo.touch_file(c.user_data_path, 'tutorial-finished')
	end)
end
