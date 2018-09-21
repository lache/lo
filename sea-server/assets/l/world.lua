local world = {}
local world_time = 0
World = {}
function World.Time()
    return world_time
end
function World.Tick()
    world_time = world_time + 1
end
