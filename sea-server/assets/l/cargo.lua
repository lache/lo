local inspect = require('assets/l/inspect')
local cargo_id_nonce = 0
local cargos = {}
local Cargo = {} Cargo.__index = Cargo

function Cargo.New(owner, item_id, amount, origin)
    cargo_id_nonce = cargo_id_nonce + 1
    local cargo_id = cargo_id_nonce
    local cargo = {
        cargo_id = cargo_id,
        item_id = item_id,
        amount = amount,
        owner = owner,
        origin = origin,
    }
    setmetatable(cargo, Cargo)
    cargos[cargo_id] = cargo
    return cargo
end

return Cargo
