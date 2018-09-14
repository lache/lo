local inspect = require('assets/l/inspect')

print('loading city.lua...')

cities = {}

-- item 10000 x 1 + item 20000 x 2 --> item 30000 1
local test_converter = { source = {{item = 10000, amount = 1}, {item = 20000, amount = 2}},
                         target = {{item = 30000, amount = 1}} }
local test_item_generator = { item = 40000, update_interval = 1, limit = 10 }
local test_item_generator2 = { item = 50000, update_interval = 1, limit = 10 }

function new_city_instance(city_id)
    local city = {
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
    if cities[city_id] == nil then
        city = new_city_instance(city_id)
        cities[city_id] = city
    end
    for k, v in pairs(city.generators) do
        local current = city.produced_items[v.item] or 0
        if current < v.limit then
            city.produced_items[v.item] = current + 1
        end
    end
end

function city_debug_query(city_id)
    print(inspect(cities[city_id]))
end

return 1985
