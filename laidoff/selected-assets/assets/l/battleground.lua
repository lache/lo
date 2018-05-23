print('battleground.lua visible')
local M = {
	objs = {}
}
M.__index = M
local c = lo.script_context()

function M:new(name, field)
	local o = {}
	o.orig_string = tostring(o)
	o.name = name
	o.field = field
	setmetatable(o, self)
	return o
end

function M:test()

	print("LVT_ENEMY_SCOPE: " .. lo.LVT_ENEMY_SCOPE)

	local c = lo.script_context()
	--print(inspect(getmetatable(c)))

	lo.show_sys_msg(c.def_sys_msg, 'I AM SCRIPT 5 한글')

	local tm = reload_require('testmod')
	print('testmod.foo()', tm.foo())

	reload_require('bullet')

	local Guntower = reload_require('guntower')
	print('Guntower loaded! ^__^')
	local Faction1 = 1
	local Faction2 = 2

	local guntower1 = Guntower:new('gt1', 4, 0, 0)
	self.field:spawn(guntower1, Faction1)
	guntower1:start_thinking()

	local guntower2 = Guntower:new('gt2', 13, 3, 0)
	guntower2.atlas = lo.LAE_CROSSBOW_KTX
	guntower2.skin_vbo = lo.LSVT_CROSSBOW
	guntower2.armature = lo.LWAR_CROSSBOW_ARMATURE
	guntower2.anim_action_id = lo.LWAC_CROSSBOW_FIRE
	guntower2.bullet_spawn_offset_z = 2.23069 / 2
	guntower2.bullet_vbo = lo.LVT_CROSSBOW_ARROW
	guntower2.bullet_atlas = lo.LAE_CROSSBOW_KTX
	guntower2.bullet_sx = 0.5
	guntower2.bullet_sy = 0.5
	guntower2.bullet_sz = 0.5
	guntower2.fire_anim_marker = 'fire'
	self.field:spawn(guntower2, Faction1)
	guntower2:start_thinking()

	local guntower3 = Guntower:new('gt3', -10, 3, 0)
	guntower3.atlas = lo.LAE_TURRET_KTX
	guntower3.skin_vbo = lo.LSVT_TURRET
	guntower3.armature = lo.LWAR_TURRET_ARMATURE
	guntower3.anim_action_id = lo.LWAC_TURRET_RECOIL
	guntower3.bullet_spawn_offset_z = 3.15099 / 2
	self.field:spawn(guntower3, Faction1)
	guntower3:start_thinking()

	local guntower4 = Guntower:new('gt4', -14, 3, 0)
	guntower4.atlas = lo.LAE_CATAPULT_KTX
	guntower4.skin_vbo = lo.LSVT_CATAPULT
	guntower4.armature = lo.LWAR_CATAPULT_ARMATURE
	guntower4.anim_action_id = lo.LWAC_CATAPULT_FIRE
	guntower4.bullet_spawn_offset_x = -2.28591 / 2
	guntower4.bullet_spawn_offset_y = 0
	guntower4.bullet_spawn_offset_z = 4.57434 / 2
	guntower4.bullet_vbo = lo.LVT_CATAPULT_BALL
	guntower4.bullet_atlas = lo.LAE_CATAPULT_KTX
	guntower4.bullet_sx = 0.5
	guntower4.bullet_sy = 0.5
	guntower4.bullet_sz = 0.5
	guntower4.fire_anim_marker = 'fire'
	guntower4.parabola = true
	self.field:spawn(guntower4, Faction1)
	guntower4:start_thinking()

	local guntower5 = Guntower:new('gt5', -12, 6, 0)
	guntower5.atlas = lo.LAE_PYRO_KTX
	guntower5.skin_vbo = lo.LSVT_PYRO
	guntower5.armature = lo.LWAR_PYRO_ARMATURE
	guntower5.anim_action_id = lo.LWAC_PYRO_IDLE
	guntower5.bullet_spawn_offset_x = -2.28591 / 2
	guntower5.bullet_spawn_offset_y = 0
	guntower5.bullet_spawn_offset_z = 4.57434 / 2
	guntower5.bullet_vbo = lo.LVT_CATAPULT_BALL
	guntower5.bullet_atlas = lo.LAE_CATAPULT_KTX
	guntower5.bullet_sx = 0.5
	guntower5.bullet_sy = 0.5
	guntower5.bullet_sz = 0.5
	guntower5.fire_anim_marker = 'fire'
	guntower5.parabola = true
	self.field:spawn(guntower5, Faction1)
	guntower5:start_thinking()

	start_coro(function()
		local idx = 1
		while true do
			local guntower2 = Guntower:new('gt-enemy-'..idx, math.random(-10, 10), math.random(-13, -3), 0)
			idx = idx + 1
			--guntower2:test()
			--print(inspect(guntower2))
			self.field:spawn(guntower2, Faction2)
			yield_wait_ms(1.5 * 1000)
			--yield_wait_ms(0.1 * 1000)
		end
	end)

	--field:despawn(guntower1)
	--field:despawn(guntower2)



	function foo()
	  print("foo", 1)
	  coroutine.yield('yielded value')
	  print("foo", 2)
	end

	co = coroutine.create(foo)
	print(type(co))
	print(coroutine.status(co))
	print(coroutine.resume(co))
	print(coroutine.resume(co))
	print(coroutine.status(co))
	print(coroutine.resume(co))

	--lo.change_to_font_test(c)

	tm.testcoro()

	start_coro(function ()
		for i=1,4 do
		  lo.show_sys_msg(c.def_sys_msg, 'hello ' .. i)
		  yield_wait_ms(1000)
		end
	end)
	--print(coro1)

	start_coro(function ()
		yield_wait_ms(5000)
		lo.show_sys_msg(c.def_sys_msg, 'hello x 1')
		yield_wait_ms(1000)
		lo.show_sys_msg(c.def_sys_msg, 'hello x 2')
		yield_wait_ms(1000)
		lo.show_sys_msg(c.def_sys_msg, 'hello x 3')
		
		-- Collect garbage forcefully
		collectgarbage()
		print('Garbage collected.')
	end)

	start_coro(function ()
		--while true do
		  local cw1 = spawn_blue_cube_wall_2(10, 10, 1)
		  yield_wait_ms(1000)
		  lo.despawn_field_object(c, cw1)
		  local cw2 = spawn_blue_cube_wall_2(11, 11, 1)
		  yield_wait_ms(1000)
		  lo.despawn_field_object(c, cw2)
		  local cw3 = spawn_blue_cube_wall_2(12, 12, 1)
		  yield_wait_ms(1000)
		  lo.despawn_field_object(c, cw3)
		  yield_wait_ms(1000)
		--end
	end)

	local nav = lo.field_nav(c.field)

	start_coro(function ()
		for i=1, 10 do
		  -- Truck field object
		  --local truck = spawn_oil_truck(pLwc, -8, -8, 0)
		  local truck = spawn_devil(pLwc, -8, -8, 0)
		  -- Path query
		  local pq = lo.nav_new_path_query(nav)
		  -- Activate path query update
		  lo.nav_update_output_path_query(nav, pq, 1)
		  -- Bind path query to truck
		  lo.nav_bind_path_query_output_location(nav, pq, c.field, truck)
		end
	end)
	
	return 'battleground'
end

return M
