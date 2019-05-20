local info = debug.getinfo(1,'S')
print('loading '..info.source)
local inspect = require('assets/l/inspect')
local entities = {} -- entities keyed by entity ID
local entities_by_name = {} -- entities keyed by entity name
local FUND_LIMIT = 10000000000
local CREDIT_RATING_LIMIT = 10000000000
local Entity = {}

function Entity:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    o.entity_id = #entities+1
    o.credit_rating = 1000
    o.fund = 0
    o.ships = {}
    o.abilities = {}
    table.insert(entities, o)
    return o
end

function Entity:add_shipyard_ability() self.abilities.shipyard = true return self end
function Entity:has_shipyard_ability() return self.abilities.shipyard or false end
function Entity:add_city_ability() self.abilities.city = true return self end
function Entity:has_city_ability() return self.abilities.city or false end
function Entity:add_bank_ability() self.abilities.bank = true return self end
function Entity:has_bank_ability() return self.abilities.bank or false end

function Entity:add_fund(amount)
    self:check_add_fund(amount)
    self.fund = self.fund + amount
    return self
end

function Entity:check_add_fund(amount)
    if amount <= 0 then error('amount should be positive') end
    if self.fund + amount > FUND_LIMIT then error('fund limit exceed') end
end

function Entity:transfer_fund(other_entity_id, amount)
    if self.fund - amount < 0 then error('insufficient fund') end
    local other_entity = Entity.Get(other_entity_id)
    other_entity:add_fund(amount)
    self.fund = self.fund - amount
    return self
end

function Entity:add_credit_rating(amount)
    self:check_add_credit_rating(amount)
    self.credit_rating = self.credit_rating + amount
    return self
end

function Entity:check_add_credit_rating(amount)
    if amount <= 0 then error('amount should be positive') end
    if self.credit_rating + amount > CREDIT_RATING_LIMIT then error('credit rating limit exceed') end
end

function Entity:transfer_credit_rating(other_entity_id, amount)
    if self.credit_rating - amount < 0 then error('insufficient credit rating') end
    local other_entity = Entity.Get(other_entity_id)
    other_entity:add_credit_rating(amount)
    self.credit_rating = self.credit_rating - amount
    return self
end

function Entity:remove_credit_rating(amount)
    if self.credit_rating - amount < 0 then error('insufficient credit rating') end
    self.credit_rating = self.credit_rating - amount
end

function Entity:id()
    return self.entity_id
end

function Entity:set_name(name)
    -- empty name case
    if #name == 0 then error('setting empty name not allowed') end
    -- the same name case
    if self.name == name then return end
    -- duplicated name case
    if entities_by_name[name] ~= nil then error('duplicated name not allowed') end
    -- ok case
    -- [1] delete previous entry
    if self.name ~= nil then entities_by_name[self.name] = nil end
    -- [2] change name
    self.name = name
    -- [3] set a new entiry
    entities_by_name[name] = self
end

function Entity.Get(entity_id)
    return entities[entity_id] or error('entity cannot be found by id '..entity_id)
end

function Entity.Get_By_Name(entity_name)
    return Entity.Get_By_Name_No_Error(entity_name) or error('entity cannot be found by name '..entity_name)
end

function Entity.Get_By_Name_No_Error(entity_name)
    return entities_by_name[entity_name]
end

function Entity.inspect_all()
    return inspect(entities)
end

return Entity
