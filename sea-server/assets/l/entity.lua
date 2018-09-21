local info = debug.getinfo(1,'S')
print('loading '..info.source)
local inspect = require('assets/l/inspect')
local entities = {}
local FUND_LIMIT = 10000000000
local CREDIT_RATING_LIMIT = 10000000000
Entity = {}

function Entity:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    o.entity_id = #entities+1
    o.credit_rating = 1000
    o.fund = 0
    table.insert(entities, o)
    return o
end

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

function Entity.Get(entity_id)
    return entities[entity_id] or error('entity cannot be found')
end

function Entity.inspect_all()
    return inspect(entities)
end
