print('data.lua visible')
local M = {
	objs = {}
}
M.__index = M
local c = lo.script_context()
local Guntower = reload_require('guntower')
function M:new()
	local o = {}
	
	o.guntower = {}
	
	local catapult = Guntower:new('catapult-template', -14, 3, 0)
	catapult.atlas = lo.LAE_CATAPULT_KTX
	catapult.skin_vbo = lo.LSVT_CATAPULT
	catapult.armature = lo.LWAR_CATAPULT_ARMATURE
	catapult.anim_action_id = lo.LWAC_CATAPULT_FIRE
	catapult.bullet_spawn_offset_x = -2.28591 / 2
	catapult.bullet_spawn_offset_y = 0
	catapult.bullet_spawn_offset_z = 4.57434 / 2
	catapult.bullet_vbo = lo.LVT_CATAPULT_BALL
	catapult.bullet_atlas = lo.LAE_CATAPULT_KTX
	catapult.bullet_sx = 0.5
	catapult.bullet_sy = 0.5
	catapult.bullet_sz = 0.5
	catapult.fire_anim_marker = 'fire'
	catapult.parabola = true
	o.guntower['catapult'] = catapult
	
	local crossbow = Guntower:new('crossbow-template', 0, 0, 0)
	crossbow.atlas = lo.LAE_CROSSBOW_KTX
	crossbow.skin_vbo = lo.LSVT_CROSSBOW
	crossbow.armature = lo.LWAR_CROSSBOW_ARMATURE
	crossbow.anim_action_id = lo.LWAC_CROSSBOW_FIRE
	crossbow.bullet_spawn_offset_z = 2.23069 / 2
	crossbow.bullet_vbo = lo.LVT_CROSSBOW_ARROW
	crossbow.bullet_atlas = lo.LAE_CROSSBOW_KTX
	crossbow.bullet_sx = 0.5
	crossbow.bullet_sy = 0.5
	crossbow.bullet_sz = 0.5
	crossbow.fire_anim_marker = 'fire'
	o.guntower['crossbow'] = crossbow
	
	local guntower = Guntower:new('guntower-template', 0, 0, 0)
	o.guntower['guntower'] = guntower
	
	local pyro = Guntower:new('pyro-template', -12, 6, 0)
	pyro.atlas = lo.LAE_PYRO_KTX
	pyro.skin_vbo = lo.LSVT_PYRO
	pyro.armature = lo.LWAR_PYRO_ARMATURE
	pyro.anim_action_id = lo.LWAC_PYRO_IDLE
	pyro.bullet_spawn_offset_x = -2.28591 / 2
	pyro.bullet_spawn_offset_y = 0
	pyro.bullet_spawn_offset_z = 4.57434 / 2
	pyro.bullet_vbo = lo.LVT_CATAPULT_BALL
	pyro.bullet_atlas = lo.LAE_CATAPULT_KTX
	pyro.bullet_sx = 0.5
	pyro.bullet_sy = 0.5
	pyro.bullet_sz = 0.5
	pyro.fire_anim_marker = 'fire'
	pyro.parabola = true
	o.guntower['pyro'] = pyro
	
	local turret = Guntower:new('turret-template', 0, 0, 0)
	turret.atlas = lo.LAE_TURRET_KTX
	turret.skin_vbo = lo.LSVT_TURRET
	turret.armature = lo.LWAR_TURRET_ARMATURE
	turret.anim_action_id = lo.LWAC_TURRET_RECOIL
	turret.bullet_spawn_offset_z = 3.15099 / 2
	o.guntower['turret'] = turret
	
	o.orig_string = tostring(o)
	setmetatable(o, self)
	return o
end

return M
