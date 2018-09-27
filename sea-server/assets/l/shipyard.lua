local Ship = require('assets/l/ship')
local Shipyard = {}
local inspect = require('assets/l/inspect')
function Shipyard:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function Shipyard.Purchase_Ship(shipyard_entity_id, buyer_entity_id, ship_template_id)
    local shipyard_entity = Entity.Get(shipyard_entity_id)
    if not shipyard_entity:has_shipyard_ability() then error('only entity with shipyard ability can build a ship') end
    local buyer_entity = Entity.Get(buyer_entity_id)
    --print(inspect(ship))
    local ship_price = 210
    buyer_entity:transfer_fund(shipyard_entity_id, ship_price)
    local ship = Ship:new()
    buyer_entity:acquire_ship(ship:id())
    shipyard_entity:add_credit_rating(ship_price // 2)
end

return Shipyard
