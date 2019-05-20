local inspect = require('assets/l/inspect')
local info = debug.getinfo(1,'S')
print('loading '..info.source)
function collection_collect(city_id, seaport_id, item_id, amount)
    local city = cities[city_id]
    --print('city:'..inspect(city))
    if city == nil then error('city nil') end
    local seaport = seaports[seaport_id]
    if seaport == nil then error('seaport nil') end
    if item_id <= 0 then error('item id not positive') end
    if amount <= 0 then error('amount not positive') end
    local city_item = city.produced_items[item_id]
    if city_item == nil then error('city item nil') end
    --print('city_item:'..inspect(city_item))
    if city_item.amount < amount then error('city item not sufficient') end
    city_item.amount = city_item.amount - amount
    local seaport_item = seaport.items[item_id] or {item_id=item_id,amount=0}
    seaport.items[item_id] = seaport_item
    seaport_item.amount = seaport_item.amount + amount
end
return 1955
