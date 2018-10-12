local info = debug.getinfo(1,'S')
print('loading '..info.source)
local inspect = require('assets/l/inspect')
local Item = require('assets/l/item')
local Dock = require('assets/l/dock')
local Generator = require('assets/l/generator')
local Funding = require('assets/l/funding')
local Entity = require('assets/l/entity')
local City = require('assets/l/city')
local Seaport = require('assets/l/seaport')
local Contract = require('assets/l/contract')
local Sea = require('assets/l/sea')
local Shipyard = require('assets/l/shipyard')
local Ownership = require('assets/l/ownership')
local Ship = require('assets/l/ship')
local Route = require('assets/l/route')

local item_id = 123
local amount = 5

local bank_entity = Entity:new():add_fund(100000000):add_credit_rating(10000):add_bank_ability()
local dep_city_entity = Entity:new():add_fund(500000):add_credit_rating(100):add_city_ability()
local dest_city_entity = Entity:new():add_fund(800000):add_credit_rating(120):add_city_ability()
local user_entity = Entity:new():add_fund(250)
local shipyard_entity = Entity:new():add_fund(1):add_shipyard_ability()

local dep_city = City.New()
local dest_city = City.New()

--print(Entity.inspect_all())

-- 1. User try to get funding
local funding = Funding.Try_Grant(bank_entity:id(), user_entity:id(), 1000, 10, 2000)

--print(Funding.Inspect_All())

local ship = Shipyard.Purchase_Ship(shipyard_entity:id(), user_entity:id(), 1)

local user2_entity = Entity:new()

Ownership.Transfer_Ship(user_entity:id(), ship:id(), user2_entity:id())

--collectgarbage()

--for i=1,10 do funding:repay(100) end
--print(inspect(funding))
--print(inspect(shipyard_entity))
--print(inspect(user2_entity))
--print(inspect(ship))

--print('Trying to dispose ship...')
Ship.Dispose(ship:id())
ship = nil
collectgarbage()
--print('Disposed!')

local dep_city = City.New()
local dest_city = City.New()
local dep_city_id = dep_city:id()
local dest_city_id = dest_city:id()

Item.Register_Produced_At_City(dep_city_id, item_id, amount + 100)
Item.Register_Wanted_At_City(dest_city_id, item_id, amount + 1000)
Item.Register_Produced_At_City(dest_city_id, 999, amount + 1)
Item.Register_Wanted_At_City(dep_city_id, 999, 100)

local generator = Generator:new(item_id, amount, 1)
Generator.register_generator_at_city(dep_city_id, generator.generator_id)
--print('departure city info:'..inspect(cities[dep_city_id]))
local dep_seaport = Seaport.New()
local dep_seaport_id = dep_seaport:id()

--print('generators (before):'..inspect(generators))
Generator.tick_all()
--print('generators (after):'..inspect(generators))

local shipyard = Shipyard.New()

local route1 = Route.New()
    :add_shipyard(shipyard:id())        -- 1
    :add_city(dep_city:id())            -- 2
    :add_city(dest_city:id())           -- 3
local route2 = Route.New()

--print(route1:get_waypoint_count())

--print(inspect(route1))

local ship = Shipyard.Purchase_Ship(shipyard_entity:id(), user_entity:id(), 1)

Route.Place(route1:id(), ship:id())

assert(ship.route_pos == 0) ship:proceed_route() -- 0 -> 1
assert(ship.route_pos == 1) ship:proceed_route() -- 1 -> 2
assert(ship.route_pos == 2) ship:proceed_route() -- 2 -> 3
assert(ship.route_pos == 3) ship:proceed_route() -- 3 -> 2
assert(ship.route_pos == 2) ship:proceed_route() -- 2 -> 1
assert(ship.route_pos == 1) ship:proceed_route() -- 1 -> 2
assert(ship.route_pos == 2) ship:proceed_route() -- 2 -> 3
assert(ship.route_pos == 3)

Item.Register_Produced_At_City(dep_city_id, item_id, amount)
Item.Register_Produced_At_City(dep_city_id, item_id, amount)
Item.Register_Produced_At_City(dep_city_id, item_id, amount)

print(inspect(ship))

local dest_seaport = Seaport.New()
dest_seaport_id = dest_seaport:id()

Item.Register_Wanted_At_City(dest_city_id, item_id, amount)
--print('destination city info:'..inspect(cities[dest_city_id]))

local contract = contract_new(item_id, amount, dep_city_id, dep_seaport_id, dest_seaport_id, dest_city_id)
--print(inspect(contract))

--print('--- before ---')
--print(inspect(seaports[dep_seaport_id]))

contract_tick(contract.contract_id)

--print('--- after 1 ---')
--print(inspect(seaports[dep_seaport_id]))

local sea_object_id = 99999
local sea_object = Ship.New(sea_object_id)
dock_ship_object_at_seaport(sea_object_id, dep_seaport_id)

contract_tick(contract.contract_id)

--print('--- after 2 ---')
--print(inspect(seaports[dep_seaport_id]))

local item1 = Item.New()
--print('item1 id (before):'..item1.id)
item1.id = 100
--print('item1 id (after):'..item1.id)

local item2 = Item.New()
--print('item2 id (before):'..item2.id)
--print(inspect(item2))
item2.id = 200
--print('item2 id (after):'..item2.id)
--print(inspect(item2))

require('assets/l/test_dsl')

return 9999
