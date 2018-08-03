print('bullet.lua visible')
local M = {
	objtype = 2,
	space_group = lo.LSG_SCRIPT_BULLET,
	collider_radius = 0.5,
	collider_offset_x = 0,
	collider_offset_y = 0,
	collider_offset_z = 0,
}
M.__index = M
local c = lo.script_context()

function M:new(name, x, y, z, angle, speed)
	local o = {}
	o.orig_string = tostring(o)
	o.name = name
	o.x = x
	o.y = y
	o.z = z
	o.angle = angle
	o.speed = speed
	o.age = 0
	o.max_age = 5
	o.range = 2
	o.damage = 50
	o.sx = 3
	o.sy = 4
	o.sz = 4
	setmetatable(o, self)
	--print(self, 'bullet spawned')
	return o
end

function M:__tostring()
	return self.name..'<Bullet['..tostring(self.orig_string:sub(8))..']>' --..debug.getinfo(1).source
end

function M:update(dt)
	--print(self, 'update')
	if self.parabola3d then
		local pt = lo.new_vec3(0, 0, 0)
		local pt_diff = self.parabola3d.p2t - self.parabola3d.p0t
		local pt_count = 10
		local pt_step = pt_diff / pt_count
		lo.lwparabola_p_3d_from_param_t(self.parabola3d, self.parabola3d.p0t + pt_diff * self.age / 2, pt)
		local ptvec = lo.get_vec3(pt)
		self.x = ptvec.x
		self.y = ptvec.y
		self.z = ptvec.z
		lo.delete_vec3(pt)
	else
		self.x = self.x + dt * self.speed * math.cos(self.angle)
		self.y = self.y + dt * self.speed * math.sin(self.angle)
	end
	lo.rmsg_pos(c, self.key, self.x, self.y, self.z)
	lo.field_geom_set_position(c.field, self.geom_idx, self.x, self.y, self.z)
	
	self.age = self.age + dt
	--print(self, 'x', self.x, 'y', self.y)
	if self.age > self.max_age or self.z < 0 then
		self.dead_flag = true
	end
end

function M:collide_with(obj_key)
	if self.dead_flag then return end
	local target = self.field.objs[obj_key]
	if target then
		if target.faction ~= self.faction then
			target.obj.hp = target.obj.hp - self.damage
			--print(self, 'target HP reduced to ', target.hp, 'damage', self.damage)
			self.dead_flag = true
			--self:play_explosion()
		end
	else
		self.dead_flag = true
		self:play_explosion()
	end
end

function M:play_explosion()
	local pt = lo.new_vec3(self.x, self.y, self.z)
	local field = lo.lwcontext_field(c)
	local ps = lo.field_ps(field)
	lo.ps_play_new_pos(c.ps_context, ps, pt)
	lo.delete_vec3(pt)
end

return M
