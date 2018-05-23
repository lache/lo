print('testfield2.lua visible')

local Guntower = reload_require('guntower')
print('Guntower loaded! ^__^')

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

function M:start_enemy_spawn_coro()
	start_coro(function()
		local idx = 1
		while true do
			local gt = Guntower:new('gt-enemy-'..idx, math.random(-10, 10), math.random(-13, -3), 0)
			idx = idx + 1
			self.field:spawn(gt, Faction2)
			yield_wait_ms(1.5 * 1000)
		end
	end)
end

function M:spawn_nav_devil()
	local nav = lo.field_nav(c.field)
	for i = 1, 10 do
	  -- Devil field object
	  local devil = spawn_devil(pLwc, -8, -8, 0)
	  -- Path query
	  local pq = lo.nav_new_path_query(nav)
	  -- Activate path query update
	  lo.nav_update_output_path_query(nav, pq, 1)
	  -- Bind path query to devil
	  lo.nav_bind_path_query_output_location(nav, pq, c.field, devil)
	end
end

function M:test()
	local c = lo.script_context()

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

	self:start_enemy_spawn_coro()
	self:spawn_nav_devil()
	
	return 'testfield2'
end

return M
