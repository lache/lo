local inspect = require('assets/l/inspect')
local info = debug.getinfo(1,'S')
print('loading '..info.source)

seaports = {}

function new_seaport_instance(seaport_id)
    local seaport = {
        seaport_id = seaport_id,
        items = {},
        docked_sea_objects = {},
    }
    return seaport
end

function seaport_update(seaport_id)
    local seaport = seaports[seaport_id]
    if seaport == nil then
        seaport = new_seaport_instance(seaport_id)
        seaports[seaport_id] = seaport
    end
end

function seaport_debug_query(seaport_id)
    print(seaport_id,inspect(seaports[seaport_id]))
end

return 2018
