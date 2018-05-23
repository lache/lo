print('room.lua visible')
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
	return 'room'
end

return M
