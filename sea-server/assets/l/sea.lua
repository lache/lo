local inspect = require('assets/l/inspect')
local info = debug.getinfo(1,'S')
print('loading '..info.source)

sea_objects = {}

function new_sea_object_instance(sea_object_id)
    print('Creating a new lua sea_object instance id '..sea_object_id)
    local sea_object = {
        sea_object_id = sea_object_id,
        item = 0,
        amount = 0,
        max_amount = 0,
    }
    return sea_object
end

function sea_object_update(sea_object_id)
    local sea_object = sea_objects[sea_object_id]
    if sea_object == nil then
        sea_object = new_sea_object_instance(sea_object_id)
        sea_objects[sea_object_id] = sea_object
    end
end

function sea_object_update_route(sea_object_id)
    local sea_object = sea_objects[sea_object_id]
    if sea_object == nil then
        sea_object = new_sea_object_instance(sea_object_id)
        sea_objects[sea_object_id] = sea_object
    end
end

function sea_debug_query(sea_object_id)
    print(sea_objects,inspect(sea_objects[sea_id]))
end

return 2016
