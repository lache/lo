local City = require('assets/l/city')

local Item = {item_id = 0, amount = 0} Item.__index = Item

local ITEM_AMOUNT_LIMIT = 10000000000

function Item.New(o)
    o = o or {}
    setmetatable(o, Item)
    return o
end

function Item:check_add_amount(amount)
    if (self.amount or 0) + amount > ITEM_AMOUNT_LIMIT then error('item amount limit error') end
end

function Item:add_amount(amount)
    self:check_add_amount(amount)
    self.amount = (self.amount or 0) + amount
end

function Item.Register_Produced_At_City(city_id, item_id, amount)
    local city = City.Get(city_id)
    city.produced_items[item_id] = city.produced_items[item_id] or Item.New()
    city.produced_items[item_id]:add_amount(amount)
end

function Item.Register_Wanted_At_City(city_id, item_id, amount)
    local city = City.Get(city_id)
    city.wanted_items[item_id] = {item_id=item_id,amount=amount}
end

function Item.Move_All(left_items, right_items)
    -- check preconditions
    for right_item_id, right_item in pairs(right_items) do
        left_items[right_item_id] = left_items[right_item_id] or Item.New()
        left_items[right_item_id]:check_add_amount(right_item.amount)
    end
    for right_item_id, right_item in pairs(right_items) do
        left_items[right_item_id]:add_amount(right_item.amount)
        right_items[right_item_id] = nil
    end
end

function Item.Move_All_Wanted(left_items, right_items, wanted_items)
    -- check preconditions & store move items
    local move_items = {}
    for right_item_id, right_item in pairs(right_items) do
        local wanted_item = wanted_items[right_item_id]
        local amount = math.min(right_item.amount, wanted_item and wanted_item.amount or 0)
        if amount > 0 then
            left_items[right_item_id] = left_items[right_item_id] or Item.New()
            left_items[right_item_id]:check_add_amount(amount)
            move_items[right_item_id] = Item.New{item_id=right_item_id, amount=amount}
        end
    end
    -- move!!!
    for right_item_id, right_item in pairs(right_items) do
        local amount = move_items[right_item_id] and move_items[right_item_id].amount or 0
        if amount > 0 then
            left_items[right_item_id]:add_amount(amount)
            right_item.amount = right_item.amount - amount
            if right_item.amount <= 0 then
                right_items[right_item_id] = nil
            end
            local wanted_item = wanted_items[right_item_id]
            if wanted_item then
                wanted_item.amount = wanted_item.amount - amount
                if wanted_item.amount <= 0 then
                    wanted_items[right_item_id] = nil
                end
            end
        end
    end
    return move_items
end

return Item
