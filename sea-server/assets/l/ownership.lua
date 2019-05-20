local Entity = require('assets/l/entity')
local Ship = require('assets/l/ship')

local Ownership = {}

function Ownership.First_Acquire_Ship(owner_entity_id, ship_id)
    local ship = Ship.Get(ship_id)
    if ship.owner_entity then error('ship already owned by entity') end
    local owner_entity = Entity.Get(owner_entity_id)
    if owner_entity.ships[ship_id] then error('entity already owned this ship') end
    ship.owner_entity = owner_entity
    owner_entity.ships[ship_id] = ship
end

function Ownership.Transfer_Ship(from_entity_id, ship_id, to_entity_id)
    if from_entity_id == to_entity_id then error('\'from entity\' id and \'to entity\' id are the same') end
    local ship = Ship.Get(ship_id)
    local from_entity = Entity.Get(from_entity_id)
    if ship.owner_entity ~= from_entity then error('ship not owned by \'from entity\'') end
    local to_entity = Entity.Get(to_entity_id)
    ship.owner_entity = to_entity
    from_entity.ships[ship_id] = nil
    to_entity.ships[ship_id] = ship
end

return Ownership
