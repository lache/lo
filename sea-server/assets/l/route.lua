local City = require('assets/l/city')
local Ship = require('assets/l/ship')
local Shipyard = require('assets/l/shipyard')

local Route = {} Route.__index = Route
local routes = {}

function Route.New()
    local o = {}
    setmetatable(o, Route)
    o.route_id = #routes + 1
    o.waypoints = {}
    routes[o.route_id] = o
    return o
end

function Route:id() return self.route_id end

function Route:add_city(city_id)
    table.insert(self.waypoints, City.Get(city_id))
    return self
end

function Route:add_shipyard(shipyard_id)
    table.insert(self.waypoints, Shipyard.Get(shipyard_id))
    return self
end

function Route:get_waypoint_count() return #self.waypoints end

function Route.Get(route_id) return routes[route_id] or error('route nil') end

function Route.Place(route_id, ship_id)
    local route = Route.Get(route_id)
    local ship = Ship.Get(ship_id)
    ship.route = route
end

return Route
