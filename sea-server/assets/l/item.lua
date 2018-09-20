local item = {id = 0, amount = 0}

function item:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function item.register_produced_at_city(city_id, item_id, amount)
    cities[city_id].produced_items[item_id] = {item_id=item_id,amount=amount}
end

function item.register_wanted_at_city(city_id, item_id, amount)
    cities[city_id].wanted_items[item_id] = {item_id=item_id,amount=amount}
end

return item
