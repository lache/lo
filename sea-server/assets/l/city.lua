local inspect = require('assets/l/inspect')

print('loading city.lua...')

cities = {}

-- item 10000 x 1 + item 20000 x 2 --> item 30000 1
local test_converter = { source = {{item_id = 10000, amount = 1}, {item_id = 20000, amount = 2}},
                         target = {{item_id = 30000, amount = 1}} }
local test_item_generator = { item_id = 40000, update_interval = 1, limit = 10 }
local test_item_generator2 = { item_id = 50000, update_interval = 1, limit = 10 }

function new_city_instance(city_id)
    local city = {
        city_id = city_id,
        converters = {},
        generators = {},
        produced_items = {},
        wanted_items = {},
    }
    if city_id == 64 then -- Busan city
        table.insert(city.converters, test_converter)
        table.insert(city.generators, test_item_generator)
    elseif city_id == 4411 then -- test city
        table.insert(city.generators, test_item_generator2)
    end
    return city
end

function city_update(city_id)
    local city = cities[city_id]
    if city == nil then
        city = new_city_instance(city_id)
        cities[city_id] = city
    end
    city_update_produce(city_id)
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
    print(inspect(cities[city_id]))
end

return 1985
