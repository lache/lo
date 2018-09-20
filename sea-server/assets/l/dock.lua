function dock_ship_object_at_seaport(sea_object_id, seaport_id)
    local sea_object = sea_objects[sea_object_id]
    local seaport = seaports[seaport_id]
    seaport.docked_sea_objects[sea_object_id] = sea_object
    sea_object.docked_at = seaport
end
