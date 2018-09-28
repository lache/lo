local Ship = require('assets/l/ship')
local Shipyard = {} Shipyard.__index = Shipyard
local inspect = require('assets/l/inspect')
local Ownership = require('assets/l/ownership')
local Entity = require('assets/l/entity')
local shipyard_id_nonce = 0
local shipyards = {}

function Shipyard.New(o)
    o = o or {}
    setmetatable(o, Shipyard)
    shipyard_id_nonce = shipyard_id_nonce + 1
    o.shipyard_id = shipyard_id_nonce
    shipyards[o.shipyard_id] = o
    return o
end

function Shipyard:id() return self.shipyard_id end

function Shipyard.Get(shipyard_id) return shipyards[shipyard_id] end

function Shipyard.Purchase_Ship(shipyard_entity_id, buyer_entity_id, ship_template_id)
    local shipyard_entity = Entity.Get(shipyard_entity_id)
    if not shipyard_entity:has_shipyard_ability() then error('only entity with shipyard ability can build a ship') end
    local buyer_entity = Entity.Get(buyer_entity_id)
    --print(inspect(ship))
    local ship_price = 210
    buyer_entity:transfer_fund(shipyard_entity_id, ship_price)
    local ship = Ship:new()
    Ownership.First_Acquire_Ship(buyer_entity:id(), ship:id())
    shipyard_entity:add_credit_rating(ship_price // 2)
    return ship
end

function Shipyard:on_enter_ship(ship_id)

end

return Shipyard
