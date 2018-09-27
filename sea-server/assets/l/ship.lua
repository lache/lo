local Ship = {}
local ships = {}

function Ship:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    o.ship_id = #ships + 1
    ships[o.ship_id] = o
    return o
end

function Ship:id()
    return self.ship_id
end

function Ship.Get(ship_id)
    return ships[ship_id] or error('ship cannot be found')
end

return Ship
