local inspect = require('assets/l/inspect')
local info = debug.getinfo(1,'S')
print('loading '..info.source)

contracts = {}

function contract_new(item_id, amount, dep_city_id, dep_seaport_id, dest_seaport_id, dest_city_id)
    -- variable preconditions check
    if item_id <= 0 then error('item id not positive') end
    if amount <= 0 then error('amount not positive') end
    local dep_city = cities[dep_city_id]
    if dep_city == nil then error('departure city nil') end
    local dep_seaport = seaports[dep_seaport_id]
    if dep_seaport == nil then error('departure seaport nil') end
    local dest_seaport = seaports[dest_seaport_id]
    if dest_seaport == nil then error('destination seaport nil') end
    local dest_city = cities[dest_city_id]
    if dest_city == nil then error('destination city nil') end
    -- departure city contract rule check
    local dep_city_item = dep_city.produced_items[item_id]
    if dep_city_item == nil then error('departure city produced item nil') end
    if dep_city_item.amount < amount then error('departure city produced item not sufficient') end
    -- destination city contract rule check
    local dest_city_item = dest_city.wanted_items[item_id]
    if dest_city_item == nil then error('destination city wanted item nil') end
    if dest_city_item.amount > amount then error('destination city wanted item not sufficient') end
    -- ok to create a new contract
    local contract_id = #contracts + 1
    local contract = {
        contract_id = contract_id,
        item_id = item_id,
        amount = amount,
        dep_city = dep_city,
        dep_seaport = dep_seaport,
        dest_seaport = dest_seaport,
        dest_city = dest_city,
    }
    contracts[contract_id] = contract
    return contract
end

function contract_update()
    for contract_id, contract in pairs(contracts) do
        contract_tick(contract_id)
    end
end

function contract_tick(contract_id)
    local contract = contracts[contract_id]
    if contract == nil then error('contract nil') end
    pcall(collection_collect, contract.dep_city.city_id, contract.dep_seaport.seaport_id, contract.item_id, contract.amount)
    for docked_sea_object_id, docked_ship_object in pairs(contract.dep_seaport.docked_sea_objects) do
        local ret, err = pcall(loadunload_load, contract.dep_seaport.seaport_id, docked_sea_object_id, contract.item_id, contract.amount)
        if err ~= nil then print(err) end
    end
end
