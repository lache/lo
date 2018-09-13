print('loading admin.lua...')

function seaport(id, xc, yc)
    local msg = ss.spawn_port_command()
    local msg_header = ss.command()
    msg_header.type = string.char(6)
    msg._ = msg_header
    msg.expected_db_id = id
    msg.xc = xc
    msg.yc = yc
    msg.name = '스크립트 항구'
    return ss.post_admin_message(msg)
end

function despawn_seaport(id)
    local msg = ss.delete_port_command()
    local msg_header = ss.command()
    msg_header.type = string.char(8)
    msg._ = msg_header
    msg.port_id = id
    return ss.post_admin_message(msg)
end

function city(id, xc, yc)
    local msg = ss.spawn_city_command()
    local msg_header = ss.command()
    msg_header.type = string.char(13)
    msg._ = msg_header
    msg.expected_db_id = id
    msg.xc = xc
    msg.yc = yc
    msg.name = '스크립트 씨티'
    return ss.post_admin_message(msg)
end

function endpoints()
    return ss.endpoints()
end
