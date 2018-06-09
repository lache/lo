print('post_init.lua visible')
local inspect = require('inspect')
local T = require('strings_en')
-- Utility functions begin

function lo.new_vec3(x, y, z)
	local v = lo.new_float(3)
	lo.float_setitem(v, 0, x)
	lo.float_setitem(v, 1, y)
	lo.float_setitem(v, 2, z)
	return v
end

function lo.print_vec3(v)
	print('x', lo.float_getitem(v, 0), 'y', lo.float_getitem(v, 1), 'z', lo.float_getitem(v, 2))
end

function lo.get_vec3(v)
	return {
		['x'] = lo.float_getitem(v, 0),
		['y'] = lo.float_getitem(v, 1),
		['z'] = lo.float_getitem(v, 2)
	}
end

function lo.delete_vec3(v)
	lo.delete_float(v)
end

function reload_require(modname)
	package.loaded[modname] = nil
	return require(modname)
end

-- Utility functions end

local c = lo.script_context()

local Data = reload_require('data')
print('Data loaded!')
local data = Data:new()

local Field = reload_require('field')
print('Field loaded!')
local field = Field:new('test field')
field:test()
field:start_updating()
-- Lua handler for anim marker events emitted from C.
-- This function is called from C, thus should not have 'local' specifier.
-- Also note that this function implicitly captures 'field'.
function on_anim_marker(key, name)
	--print('on_anim_marker key:',key,', name:', name)
	field:on_anim_marker(key, name)
	return 0
end
-- Lua handler for collision events (near events) emitted from C.
function on_near(key1, key2)
	--print('on_near key1', key1, 'key2', key2)
	field.objs[key2].obj:collide_with(key1)
	return 0
end
-- Lua handler for logc frame finish events emitted from C
function on_logic_frame_finish(dt)
	field:update(dt)
	return 0
end

local last_construct_gtid = ''

function copy(obj, seen)
  if type(obj) ~= 'table' then return obj end
  if seen and seen[obj] then return seen[obj] end
  local s = seen or {}
  local res = setmetatable({}, getmetatable(obj))
  s[obj] = res
  for k, v in pairs(obj) do res[copy(k, s)] = copy(v, s) end
  return res
end

function get_string(id)
	return T[id]
end

-- Lua handler for logc frame finish events emitted from C
function on_ui_event(id, w_ratio, h_ratio)
	--print(string.format('ui event emitted from C:%s (w_ratio:%.2f, h_ratio:%.2f)', id, w_ratio, h_ratio))
	local gtid = 'catapult'
	if id == 'seltower0' then gtid = 'catapult'
	elseif id == 'seltower1' then gtid = 'crossbow'
	elseif id == 'seltower2' then gtid = 'guntower'
	elseif id == 'seltower3' then gtid = 'pyro'
	elseif id == 'pull_button' then
		--print('[script]button_pull')
		lo.puck_game_pull_puck_toggle(c, c.puck_game)
		return 0
	elseif id == 'dash_button' then
		--print('[script]button_dash')
		lo.puck_game_dash_and_send(c, c.puck_game)
		return 0
	elseif id == 'jump_button' then
		--print('[script]button_jump')
		lo.puck_game_jump(c, c.puck_game)
		return 0
	elseif id == 'practice_button' then
		reload_require('practice_button')()
	elseif id == 'back_button' then
		reload_require('back_button')()
	elseif id == 'tutorial_button' then
		reload_require('tutorial_button')()
	elseif id == 'online_button' then
		reload_require('online_button')(lo.LW_PUCK_GAME_QUEUE_TYPE_NEAREST_SCORE_WITH_OCTAGON_SUPPORT)
	elseif id == 'leaderboard_button' then
		lo.show_leaderboard(c)
	elseif id == 'leaderboard_page_button' then
		--print(c.last_leaderboard.Current_page)
		if c.last_leaderboard.Current_page ~= 0 then
			local items_in_page = lo.puck_game_leaderboard_items_in_page(c.viewport_aspect_ratio)
			if w_ratio < 1.0/3 then
				-- go to the previous page if possible
				if c.last_leaderboard.Current_page > 1 then
					lo.request_leaderboard(c.tcp, items_in_page, c.last_leaderboard.Current_page - 1)
				end
			elseif w_ratio < 2.0/3 then
				-- go to the page which reveals the player
				lo.request_player_reveal_leaderboard(c.tcp, items_in_page)
			elseif c.last_leaderboard.Current_page < c.last_leaderboard.Total_page then
				-- go to the next page if possible
				lo.request_leaderboard(c.tcp, items_in_page, c.last_leaderboard.Current_page + 1)
			end
		end
	elseif id == 'change_nickname_button' then
		lo.start_nickname_text_input_activity(c)
	elseif id == 'more_button' then
		reload_require('more_button')()
	elseif id == 'football_mod' then
		lo.puck_game_reset_view_proj_ortho(c, c.puck_game, 1.9, 0.1, 100, 0, -8, 14, 0, 0.3, 0)
		lo.lwcontext_set_custom_puck_game_stage(c, lo.LVT_FOOTBALL_GROUND, lo.LAE_FOOTBALL_GROUND)
		c.puck_game.dash_by_direction = 1
		c.puck_game.follow_cam = 1
		c.puck_game.player_sphere_radius = 0.2
		c.puck_game.target_sphere_radius = 0.2
		c.puck_game.puck_sphere_radius = 0.3
		c.puck_game.puck_lvt = lo.LVT_PUCK_PLAYER
		c.puck_game.puck_lae = lo.LAE_PUCK_GRAY_KTX
		c.puck_game.player_lvt = lo.LVT_PLAYER_CAPSULE
		c.puck_game.player_lae = lo.LAE_PLAYER_CAPSULE
		c.puck_game.world_width = 12
		c.puck_game.tower_pos = 1
		c.puck_game.hide_hp_star = 1
		c.puck_game.player_max_move_speed = 1 -- double it
		c.puck_game.player_dash_speed = 6 -- 
		--c.puck_game.dash_duration = 0.2 -- double it
		c.puck_game.sphere_mass = 1
		c.puck_game.control_joint_max_force = 100
		c.puck_game.bounce = 0.5
		c.puck_game.player_capsule = 1
		c.puck_game.player_capsule_length = 0.3
		lo.puck_game_set_tower_pos_multiplier(c.puck_game, 0, -6, 0)
		lo.puck_game_set_tower_pos_multiplier(c.puck_game, 1, 6, 0)
		lo.puck_game_set_secondary_static_default_values(c.puck_game)
		lo.puck_game_reset_tutorial_state(c.puck_game)
		lo.puck_game_reset_battle_state(c.puck_game)
		lo.puck_game_clear_match_data(c, c.puck_game)
		lo.puck_game_roll_to_practice(c.puck_game)
	else
		lo.construct_set_preview_enable(c.construct, 0)
		return 0
	end
	--[[
	local gt = data.guntower[gtid]
	if last_construct_gtid ~= gtid then
		lo.construct_set_preview_enable(c.construct, 1)
		lo.construct_set_preview(c.construct, gt.atlas, gt.skin_vbo, gt.armature, gt.anim_action_id)
	else
		local Faction1 = 1
		local gtinst = copy(gt)
		print(inspect(gtinst))
		local pt = lo.new_vec3(0, 0, 0)
		lo.field_get_player_position(c.field, pt)
		local ptvec = lo.get_vec3(pt)
		gtinst.x = ptvec.x
		gtinst.y = ptvec.y
		gtinst.z = ptvec.z
		field:spawn(gtinst, Faction1)
		gtinst:start_thinking()
	end
	last_construct_gtid = gtid
	]]--
	return 0
end

local function split_path(p)
	return string.match(p, "(.-)([^\\/]-%.?([^%.\\/]*))$")
end
local field_filename = lo.field_filename(c.field)
if field_filename then
	print('Field filename:' .. field_filename)
	local field_filename_path, field_filename_name, field_filename_ext = split_path(field_filename)
	local field_module_name = string.sub(field_filename_name, 1, string.len(field_filename_name) - string.len(field_filename_ext) - 1)
	print('Field filename path:' .. field_filename_path)
	print('Field filename name:' .. field_filename_name)
	print('Field filename name only:' .. field_module_name)
	print('Field filename ext:' .. field_filename_ext)

	local FieldModule = reload_require(field_module_name)
	local field_module = FieldModule:new(field_filename_name, field)
	print('field_module:test()', field_module:test())
end

if lo.puck_game_is_tutorial_completed(c.puck_game) == 1 then
else
	on_ui_event('tutorial_button', 0, 0)
end

return 1
