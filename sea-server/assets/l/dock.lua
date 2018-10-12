local Seaport = require('assets/l/seaport')
local Ship = require('assets/l/ship')

function dock_ship_object_at_seaport(sea_object_id, seaport_id)
    local sea_object = sea_objects[sea_object_id]
    local seaport = seaports[seaport_id]
    seaport.docked_sea_objects[sea_object_id] = sea_object
    sea_object.docked_at = seaport
end

function dock_ship(seaport_id, ship_id)
    local seaport = Seaport.Get(seaport_id)
    local ship = Ship.Get(ship_id)
    if seaport.docked_sea_objects[ship_id] ~= nil then error('Ship ID '..ship_id..' already docked in this seaport') end
    if ship.docked_at ~= nil then error('Ship ID '..ship_id..' already docked in other seaport ID '..ship.docked_at:id()) end
    seaport.docked_sea_objects[ship_id] = ship
    ship.docked_at = seaport
    return 0
end

function undock_ship(ship_id)
    local ship = Ship.Get(ship_id)
    local seaport = ship.docked_at
    if seaport == nil then error('Ship ID '..ship_id..' not docked') end
    seaport.docked_sea_objects[ship_id] = nil
    ship.docked_at = nil
    return 0
end
