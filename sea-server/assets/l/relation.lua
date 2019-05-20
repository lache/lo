local Relation = {}

function Relation.Calculate_Points_From_Unloaded_Items(items)
    local points = 0
    for item_id, item in pairs(items) do
        points = points + item.amount
    end
    return points
end

return Relation
