local inspect = require('assets/l/inspect')
local info = debug.getinfo(1,'S')
print('loading '..info.source)
function loadunload_load(seaport_id, sea_object_id, item_id, amount)
    local seaport = seaports[seaport_id]
    if seaport == nil then error('seaport nil') end
    local sea_object = sea_objects[sea_object_id]
    if sea_object == nil then error('sea object nil') end
    if item_id <= 0 then error('item id not positive') end
    if amount <= 0 then error('amount not positive') end
    local seaport_item = seaport.items[item_id]
    if seaport_item == nil then error('sesaport item nil') end
    if seaport_item.amount < amount then error('seaport item amount not sufficient') end
    seaport_item.amount = seaport_item.amount - amount
    local sea_object_item = sea_object.items[item_id] or {item_id=item_id,amount=0}
    sea_object.items[item_id] = sea_object_item
    sea_object_item.amount = sea_object_item.amount + amount
end
return 1955
