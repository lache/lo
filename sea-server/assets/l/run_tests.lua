local inspect = require('assets/l/inspect')
local info = debug.getinfo(1,'S')
print('loading '..info.source)

local dep_city_id = 1999
local dest_city_id = 2999
local item_id = 1
local amount = 5
local dep_seaport_id = 19
local dest_seaport_id = 29

cities[dep_city_id] = new_city_instance(dep_city_id)
cities[dep_city_id].produced_items[item_id] = {item_id=item_id,amount=amount}
seaports[dep_seaport_id] = new_seaport_instance(dep_seaport_id)

seaports[dest_seaport_id] = new_seaport_instance(dest_seaport_id)
cities[dest_city_id] = new_city_instance(dest_city_id)
cities[dest_city_id].wanted_items[item_id] = {item_id=item_id,amount=amount}

local contract = contract_new(item_id, amount, dep_city_id, dep_seaport_id, dest_seaport_id, dest_city_id)
print(inspect(contract))

print('--- before ---')
print(inspect(seaports[dep_seaport_id]))

contract_tick(contract.contract_id)

print('--- after 1 ---')
print(inspect(seaports[dep_seaport_id]))

local sea_object_id = 99999
local sea_object = new_sea_object_instance(sea_object_id)
sea_objects[sea_object_id] = sea_object

seaports[dep_seaport_id].docked_sea_objects[sea_object_id] = sea_object

contract_tick(contract.contract_id)

print('--- after 2 ---')
print(inspect(seaports[dep_seaport_id]))

return 9999
