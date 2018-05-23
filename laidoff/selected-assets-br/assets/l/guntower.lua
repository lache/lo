print('guntower.lua visible ^__^;')
local Bullet = require('bullet')

local M = {
	objtype = 1,
	fired = 0, -- last fired field time
	turnspeed = 5, -- rad/sec
	fireinterval = 1, -- seconds
	angle = 0, -- current angle
	targetangle = 0, -- target angle
	firerange = 100, -- fire range
	firearcangle = 0.05, -- rad
	bulletspeed = 30,
	x = 0, -- x position
	y = 0, -- y position
	z = 0, -- z position
	target = nil, -- current target
	hp = 100,
	anim_action_id = lo.LWAC_RECOIL,
	atlas = lo.LAE_GUNTOWER_KTX,
	skin_vbo = lo.LSVT_GUNTOWER,
	armature = lo.LWAR_GUNTOWER_ARMATURE,
	bullet_vbo = lo.LVT_BEAM,
	bullet_atlas = lo.LAE_BEAM_KTX,
	bullet_sx = 3,
	bullet_sy = 4,
	bullet_sz = 4,
	bullet_spawn_offset_x = 0,
	bullet_spawn_offset_y = 0,
	bullet_spawn_offset_z = 1.548 / 2,
	collider_offset_x = 0,
	collider_offset_y = 0,
	collider_offset_z = 0.5,
	space_group = lo.LSG_TOWER,
	collider_radius = 1,
}
M.__index = M
local c = lo.script_context()

function M:new(name, x, y, z)
	local o = {}
	o.orig_string = tostring(o)
	o.name = name
	o.x = x
	o.y = y
	o.z = z
	setmetatable(o, self)
	return o
end

function M:__tostring()
	return self.name..'<Guntower['..tostring(self.orig_string:sub(8))..']>' --..debug.getinfo(1).source
end

function M:update(dt)
	if self.hp <= 0 then
		self.dead_flag = true
		--self:play_explosion()
	end
end

function M:play_explosion()
	local pt = lo.new_vec3(self.x, self.y, self.z)
	local field = lo.lwcontext_field(c)
	local ps = lo.field_ps(field)
	lo.ps_play_new_pos(ps, pt)
	lo.delete_vec3(pt)
end

function M:test()
	print(self, 'guntower test successful', self.x, self.y, self.turnspeed, self.target, self.xxx)
end

function M:start_thinking()
	start_coro(function() self:think_coro() end)
end

function M:nearest_target_in_range_coro(range)
	while true do
		local target = self.field:query_nearest_target_in_range(self, range)
		if target == nil then
			yield_wait_ms(100)
		else
			return target
		end
	end
end

function M:play_fire_anim()
	lo.rmsg_anim(c, self.key, self.anim_action_id)
end

function M:bullet_spawn_pos()
	local ca = math.cos(self.angle)
	local sa = math.sin(self.angle)
	return {
		x = self.x + (ca * self.bullet_spawn_offset_x - sa * self.bullet_spawn_offset_y),
		y = self.y + (sa * self.bullet_spawn_offset_x + ca * self.bullet_spawn_offset_y),
		z = self.z + self.bullet_spawn_offset_z,
	}
end

function M:spawn_bullet()
	local bsp = self:bullet_spawn_pos()
	local bullet = Bullet:new('', bsp.x, bsp.y, bsp.z, self.angle, self.bulletspeed)
	bullet.vbo = self.bullet_vbo
	bullet.atlas = self.bullet_atlas
	bullet.sx = self.bullet_sx
	bullet.sy = self.bullet_sy
	bullet.sz = self.bullet_sz
	if self.parabola then
		self:setup_parabola(bullet)
	end
	self.field:spawn(bullet, self.faction)
end

function M:setup_parabola(bullet)
	bullet.parabola3d = lo.LWPARABOLA3D()
	local p0 = lo.new_vec3(bullet.x, bullet.y, bullet.z)
	local a = 0.85
	local p1 = lo.new_vec3(
		a * bullet.x + (1 - a) * self.last_target_x,
		a * bullet.y + (1 - a) * self.last_target_y,
		bullet.z * 2
	)
	local p2 = lo.new_vec3(self.last_target_x, self.last_target_y, 0)
	lo.lwparabola_three_points_on_plane_perpendicular_to_xy_plane(p0, p1, p2, bullet.parabola3d)
	lo.delete_vec3(p0)
	lo.delete_vec3(p1)
	lo.delete_vec3(p2)
end

function M:try_fire(target)
	local ft = self.field.time()
	local last_fire_elapsed = ft - self.fired
	--print('ft',ft, 'self.fired', self.fired, 'last_fire_elapsed', last_fire_elapsed, 'self.fireinterval', self.fireinterval)
	if self.fired ~= 0 and last_fire_elapsed < self.fireinterval then
		-- No fire.
		-- Just end this think tick and try again at next tick.
	else
		self.fired = ft
		self:play_fire_anim()
		if self.fire_anim_marker == nil then
			self:spawn_bullet()
		end
	end
end

function M:on_anim_marker(name)
	--print('guntower: anim marker triggered', name)
	if self.fire_anim_marker and name == self.fire_anim_marker then
		self:spawn_bullet()
	end
end

function M:turn_barrel(target)

	self.targetangle = math.atan(target.y - self.y, target.x - self.x)
	local angle_diff = self.targetangle - self.angle
	local move_angle = angle_diff * self.turnspeed * self.field.dt
	self.angle = self.angle + move_angle;
	
	lo.rmsg_turn(c, self.key, self.angle)
	
	self.last_target_x = target.x
	self.last_target_y = target.y
	
	if math.abs(self.angle - self.targetangle) < self.firearcangle then
		return true
	else
		return false
	end
end

function M:think_coro()
	while true do
		--print(self, 'Finding target...')
		local target = self:nearest_target_in_range_coro(self.firerange)
		--print(self, 'Target found!', target, 'Turning the barrel...')
		local barrel_aligned = self:turn_barrel(target)
		if barrel_aligned then
			--print(self, 'Barrel aligned to target!', target, 'Try fire...')
			self:try_fire(target)
			--print(self, 'Wait for next think tick.. (long)')
			yield_wait_ms(100)
		else
			--print(self, 'Barrel NOT aligned to target. current', self.angle, 'target', self.targetangle, 'No fire.')
			--print(self, 'Wait for next think tick.. (short)')
			yield_wait_ms(32)
		end
    end
end

return M