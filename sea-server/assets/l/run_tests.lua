local info = debug.getinfo(1,'S')
print('loading '..info.source)
local inspect = require('assets/l/inspect')
local Item = require('assets/l/item')
require('assets/l/dock')
local Generator = require('assets/l/generator')
require('assets/l/funding')
require('assets/l/entity')
require('assets/l/city')
require('assets/l/seaport')
require('assets/l/contract')
require('assets/l/sea')
local Shipyard = require('assets/l/shipyard')

local dep_city_id = 1999
local dest_city_id = 2999
local item_id = 123
local amount = 5
local dep_seaport_id = 19
local dest_seaport_id = 29

local bank_entity = Entity:new():add_fund(100000000):add_credit_rating(10000)
local dep_city_entity = Entity:new():add_fund(500000):add_credit_rating(100)
local dest_city_entity = Entity:new():add_fund(800000):add_credit_rating(120)
local user_entity = Entity:new():add_fund(250)
local shipyard_entity = Entity:new():add_fund(1):add_shipyard_ability()

--print(Entity.inspect_all())

-- 1. User try to get funding
local funding = Funding.Try_Grant(bank_entity:id(), user_entity:id(), 1000, 10, 2000)

--print(Funding.Inspect_All())

Shipyard.Purchase_Ship(shipyard_entity:id(), user_entity:id(), 1)

--for i=1,10 do funding:repay(100) end
print(inspect(funding))
print(inspect(shipyard_entity))

cities[dep_city_id] = new_city_instance(dep_city_id)
Item.register_produced_at_city(dep_city_id, item_id, amount)
local generator = Generator:new(item_id, amount, 1)
Generator.register_generator_at_city(dep_city_id, generator.generator_id)
--print('departure city info:'..inspect(cities[dep_city_id]))
seaports[dep_seaport_id] = new_seaport_instance(dep_seaport_id)

--print('generators (before):'..inspect(generators))
Generator.tick_all()
--print('generators (after):'..inspect(generators))

seaports[dest_seaport_id] = new_seaport_instance(dest_seaport_id)
cities[dest_city_id] = new_city_instance(dest_city_id)
Item.register_wanted_at_city(dest_city_id, item_id, amount)
--print('destination city info:'..inspect(cities[dest_city_id]))

local contract = contract_new(item_id, amount, dep_city_id, dep_seaport_id, dest_seaport_id, dest_city_id)
--print(inspect(contract))

--print('--- before ---')
--print(inspect(seaports[dep_seaport_id]))

contract_tick(contract.contract_id)

--print('--- after 1 ---')
--print(inspect(seaports[dep_seaport_id]))

local sea_object_id = 99999
local sea_object = new_sea_object_instance(sea_object_id)
sea_objects[sea_object_id] = sea_object
dock_ship_object_at_seaport(sea_object_id, dep_seaport_id)

contract_tick(contract.contract_id)

--print('--- after 2 ---')
--print(inspect(seaports[dep_seaport_id]))

local item1 = Item:new()
--print('item1 id (before):'..item1.id)
item1.id = 100
--print('item1 id (after):'..item1.id)

local item2 = Item:new()
--print('item2 id (before):'..item2.id)
--print(inspect(item2))
item2.id = 200
--print('item2 id (after):'..item2.id)
--print(inspect(item2))

return 9999
