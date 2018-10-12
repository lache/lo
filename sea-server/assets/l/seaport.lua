local inspect = require('assets/l/inspect')
local info = debug.getinfo(1,'S')
print('loading '..info.source)

local seaport_id_nonce = 0
local seaports = {}

local Seaport = {} Seaport.__index = Seaport

function Seaport.New(specified_id)
    -- prevent duplicated ID
    if specified_id ~= nil and seaports[specified_id] ~= nil then
        error('Duplicated seaport ID. Cannot create a new seaport instance')
    end
    -- jump seaport_id_nonce if it is less than specified_id
    if specified_id ~= nil and seaport_id_nonce < specified_id then
        seaport_id_nonce = specified_id - 1
    end
    seaport_id_nonce = seaport_id_nonce + 1
    local seaport_id = seaport_id_nonce
    local seaport = {
        seaport_id = seaport_id,
        items = {},
        docked_sea_objects = {},
    }
    setmetatable(seaport, Seaport)
    seaports[seaport_id] = seaport
    return seaport
end

function Seaport.Get(seaport_id) return seaports[seaport_id] or error('seaport nil') end
function Seaport:id() return self.seaport_id end

function seaport_update(seaport_id)
    local seaport = seaports[seaport_id]
    if seaport == nil then
        error('No seaport with ID '..seaport_id)
    end
end

function seaport_debug_query(seaport_id)
    print(seaport_id,inspect(seaports[seaport_id]))
end

function seaport_new(specified_id)
    Seaport.New(specified_id)
end

return Seaport
