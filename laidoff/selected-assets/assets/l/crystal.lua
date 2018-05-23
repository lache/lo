print('crystal.lua visible')
local M = {
	objtype = 2,
	space_group = lo.LSG_TOWER,
	collider_radius = 0.5,
	collider_offset_x = 0,
	collider_offset_y = 0,
	collider_offset_z = 0,
}
M.__index = M
local c = lo.script_context()

function M:new(name, x, y, z, angle)
	local o = {}
	o.orig_string = tostring(o)
	o.name = name
	o.x = x
	o.y = y
	o.z = z
	o.angle = angle
	o.speed = 0
	o.age = 0
	o.max_age = 5
	o.range = 2
	o.damage = 50
	o.sx = 1
	o.sy = 1
	o.sz = 1
	o.vbo = lo.LVT_CRYSTAL
	o.atlas = lo.LAE_CRYSTAL_KTX
	setmetatable(o, self)
	--print(self, 'crystal spawned')
	return o
end

function M:__tostring()
	return self.name..'<Crystal['..tostring(self.orig_string:sub(8))..']>' --..debug.getinfo(1).source
end

function M:update(dt)
end

function M:collide_with(obj_key)
end

function M:play_explosion()
end

return M
