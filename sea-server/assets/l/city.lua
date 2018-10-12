local inspect = require('assets/l/inspect')
local info = debug.getinfo(1,'S')
print('loading '..info.source)
local Entity = require('assets/l/entity')
local city_id_nonce = 0
local cities = {}

local City = {} City.__index = City

-- item 10000 x 1 + item 20000 x 2 --> item 30000 1
local test_converter = { source = {{item_id = 10000, amount = 1}, {item_id = 20000, amount = 2}},
                         target = {{item_id = 30000, amount = 1}} }
local test_item_generator = { item_id = 40000, update_interval = 1, limit = 10 }
local test_item_generator2 = { item_id = 50000, update_interval = 1, limit = 10 }

function City.New()
    city_id_nonce = city_id_nonce + 1
    local city_id = city_id_nonce
    local city = {
        city_id = city_id,
        converters = {},
        generators = {},
        produced_items = {},
        wanted_items = {},
        inventory_items = {},
        entity_relation_points = {}
    }
    if city_id == 64 then -- Busan city
        table.insert(city.converters, test_converter)
        table.insert(city.generators, test_item_generator)
    elseif city_id == 4411 then -- test city
        table.insert(city.generators, test_item_generator2)
    end
    setmetatable(city, City)
    cities[city_id] = city
    return city
end

function City.Get(city_id) return cities[city_id] or error('city nil') end
function City:id() return self.city_id end

function city_update(city_id)
    local city = cities[city_id]
    if city == nil then
        error('No city with ID '..city_id)
    else
        city_update_produce(city_id)
    end
end

function city_update_produce(city_id)
    local city = cities[city_id]
    if city == nil then error('city nil') end
    for _, generator in pairs(city.generators) do
        local current = city.produced_items[generator.item_id] or {item_id=generator.item_id,amount=0}
        city.produced_items[generator.item_id] = current
        if current.amount < generator.limit then
            current.amount = current.amount + 1
        end
    end
end

function city_debug_query(city_id)
    print('-------------------------')
    print(inspect(cities[city_id]))
end

function City:add_relation_points(entity_id, relation_point)
    -- preconditions
    Entity.Get(entity_id)
    -- transact
    self.entity_relation_points[entity_id] = (self.entity_relation_points[entity_id] or 0) + relation_point
end

function City:on_enter_ship(ship_id)
    local Ship = require 'assets/l/ship'
    local Item = require 'assets/l/item'
    local Relation = require 'assets/l/relation'
    local ship = Ship.Get(ship_id)
    -- load all produced items
    Item.Move_All(ship.loaded_items, self.produced_items)
    -- unload all wanted items
    local unloaded_items = Item.Move_All_Wanted(self.inventory_items, ship.loaded_items, self.wanted_items)
    local relation_points = Relation.Calculate_Points_From_Unloaded_Items(unloaded_items)
    if relation_points > 0 then
        self:add_relation_points(ship.owner_entity:id(), relation_points)
    end
end

function city_new()
    City.New()
end

return City
