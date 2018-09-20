local Generator = {}

generators = {}

function Generator:new(item_id, amount, generate_interval_tick)
    o = o or { item_id = item_id, amount = amount, generate_interval_tick = generate_interval_tick, limit = 100, produced = 0, current_tick = 0 }
    setmetatable(o, self)
    self.__index = self
    table.insert(generators, o)
    o.generator_id = #generators
    return o
end

function Generator:tick()
    self.current_tick = self.current_tick + 1
    if self.current_tick % self.generate_interval_tick == 0 then
        self.produced = self.produced + self.amount
        if self.produced > self.limit then
            self.produced = self.limit
        end
    end
end

function Generator.register_generator_at_city(city_id, generator_id)
    cities[city_id].generators[generator_id] = generators[generator_id]
end

function Generator.tick_all()
    for _, generator in ipairs(generators) do
        generator:tick()
    end
end

return Generator
