local inspect = require('assets/l/inspect')
local City = require('assets/l/city')
local Item = require('assets/l/item')
local Relation = require('assets/l/relation')

local Ship = {} -- {__gc = function(u) print(string.format('ship id %d gced', u.ship_id)) end}
local ships = {}

function Ship:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    o.ship_id = #ships + 1
    o.route_pos = 0
    o.route_dir = 1 -- +1 or -1
    o.loaded_items = {}
    o.owner_entity = nil
    ships[o.ship_id] = o
    return o
end

function Ship:id()
    return self.ship_id
end

function Ship.Get(ship_id)
    return ships[ship_id] or error('ship cannot be found')
end

function Ship.Sell()

end

function Ship.Dispose(ship_id)
    local ship = Ship.Get(ship_id)
    local owner_entity = ship.owner_entity
    ship.owner_entity = nil
    owner_entity.ships[ship_id] = nil
    ships[ship_id] = nil
end

function Ship:proceed_route()
    if not self.route then error('no route assigned') end
    if #self.route.waypoints < self.route_pos then error('already finished route') end
    if self.route_pos > 0 then
        self:on_exit_waypoint()
    end
    self.route_pos = self.route_pos + self.route_dir
    if self.route_pos == 1 then
        -- reverse direction
        self.route_pos = 1
        self.route_dir = 1
    elseif #self.route.waypoints >= self.route_pos then
        self:on_enter_waypoint()
        if #self.route.waypoints == self.route_pos then
            -- reverse direction
            self.route_pos = #self.route.waypoints
            self.route_dir = -1
        end
    end
end

function Ship:on_exit_waypoint()
    local waypoint = self.route.waypoints[self.route_pos]
end

function Ship:on_enter_waypoint()
    local waypoint = self.route.waypoints[self.route_pos]
    waypoint:on_enter_ship(self:id())
end

return Ship
