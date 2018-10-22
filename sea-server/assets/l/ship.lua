local inspect = require('assets/l/inspect')
local City = require('assets/l/city')
local Item = require('assets/l/item')
local Relation = require('assets/l/relation')

local ship_id_nonce = 0
local ships = {}

local Ship = {} Ship.__index = Ship -- {__gc = function(u) print(string.format('ship id %d gced', u.ship_id)) end}

function Ship.New(specified_id)
    -- prevent duplicated ID
    if specified_id ~= nil and ships[specified_id] ~= nil then
        error('Duplicated ship ID. Cannot create a new ship instance')
    end
    -- jump ship_id_nonce if it is less than specified_id
    if specified_id ~= nil and ship_id_nonce < specified_id then
        ship_id_nonce = specified_id - 1
    end
    ship_id_nonce = ship_id_nonce + 1
    local ship_id = ship_id_nonce
    local ship = {
        ship_id = ship_id,
        route_pos = 0,
        route_dir = 1, -- +1 or -1
        loaded_items = {},
        owner_entity = nil,
    }
    setmetatable(ship, Ship)
    ships[ship_id] = ship
    return ship
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

function ship_new(specified_id)
    print('ship_new() called')
    return Ship.New(specified_id)
end

function ship_id(ship)
    print('ship_id() called')
    return ship:id()
end

return Ship
