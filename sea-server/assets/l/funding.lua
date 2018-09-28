local info = debug.getinfo(1,'S')
print('loading '..info.source)
local inspect = require('assets/l/inspect')
local Entity = require('assets/l/entity')
local World = require('assets/l/world')

local fundings = {}
local Funding = {}

function Funding:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    table.insert(fundings, o)
    return o
end

function Funding.Try_Grant(from_entity_id, to_entity_id, amount, due, repay_amount)
    -- condition check
    if amount <= 0 then error('amount should be positive') end
    if due <= 0 then error('due should be positive') end
    if repay_amount <= amount then error('repay amount should be greater than amount') end
    local from_entity = Entity.Get(from_entity_id)
    if from_entity.fund <= 0 then error('from entity not sufficient fund') end
    if from_entity.credit_rating < amount then error('funding amount exceeds \'from entity\' credit rating') end
    local to_entity = Entity.Get(to_entity_id)
    if to_entity.credit_rating < amount then error('funding amount exceeds \'to entity\' credit rating') end
    -- transaction
    from_entity:transfer_fund(to_entity:id(), amount)
    to_entity:remove_credit_rating(amount)
    from_entity:remove_credit_rating(amount)
    -- create and register funding object
    return Funding:new{ funding_id=#fundings+1, amount=amount, due=due, from_entity=from_entity, to_entity=to_entity, repay_amount=repay_amount, start_time=World.Time(), repay_amount_remained=repay_amount }
end

function Funding:repay(amount)
    -- condition check
    if self.to_entity.fund < amount then error('repaying amount exceeds \'to entity\' fund') end
    if self.repay_amount_remained - amount < 0 then error('cannot repay larger than remained') end
    self.to_entity:check_add_credit_rating(amount)
    self.from_entity:check_add_credit_rating(amount)
    -- transaction
    self.to_entity:transfer_fund(self.from_entity:id(), amount)
    self.to_entity:add_credit_rating(amount)
    self.from_entity:add_credit_rating(amount)
    self.repay_amount_remained = self.repay_amount_remained - amount
end

function Funding.Inspect_All()
    return inspect(fundings)
end

return Funding
