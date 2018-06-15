local inspect = require('inspect')

local c = lo.script_context()
lo.htmlui_set_online(c.htmlui, 0)

function worldmap_scroll(dlng, dlat, dscale)
    lo.lwttl_worldmap_scroll(c.ttl, dlng/100, dlat/100, dscale)
end

function worldmap_scroll_to_cell_center(xc, yc)
    lo.lwttl_worldmap_scroll_to_cell_center(c.ttl, xc, yc, lo.lwttl_sea_udp(c.ttl))
end

function go_online()
    if c.tcp_ttl == nil then
        lo.lw_new_tcp_ttl(c)
    end
    lo.htmlui_set_online(c.htmlui, 1)
    lo.tcp_request_landing_page(c.tcp_ttl, c.user_data_path, c.ttl);
end

function set_track_object_id(id)
    lo.lwttl_set_track_object_id(c.ttl, id)
end

function set_track_object_ship_id(ship_id)
    lo.lwttl_set_track_object_ship_id(c.ttl, ship_id)
end

function request_waypoints(ship_id)
    lo.lwttl_request_waypoints(c.ttl, ship_id)
end

local select_context = {}
function select(type, id)
    print('[ui-select] type:'..type..',id:'..id)
    if select_context[type] == nil then
        select_context[type] = {}
    end
    table.insert(select_context[type], id)
    print('=== select_context ===')
    print(http_header())
end

function http_header()
    local r = ''
    for key, value in pairs(select_context) do
        r = r .. 'X-Select-' .. key:sub(1,1):upper()..key:sub(2) .. ': ' .. table.concat(value, ',') .. '\r\n'
    end
    return r
end

function track(ship_id)
    set_track_object_ship_id(ship_id)
    request_waypoints(ship_id)
end

local way_mode = 0
function enable_water_way()
    way_mode = 0
end

function enable_land_way()
    way_mode = 1
end

function link()
    print('link')
    if way_mode == 0 then
        lo.htmlui_execute_anchor_click(c.htmlui, "/link");
    else
        lo.htmlui_execute_anchor_click(c.htmlui, "/linkland");
    end
end

function toggle_grid()
    lo.lwttl_toggle_cell_grid(c.ttl)
end

function transform_single_cell_water_to_land()
    if lo.lwttl_selected(c.ttl, nil) == 1 then
        local xc0 = lo.lwttl_selected_int_x(c.ttl)
        local yc0 = lo.lwttl_selected_int_y(c.ttl)
        lo.lwttl_udp_send_ttltransformsinglecell(c.ttl, lo.lwttl_sea_udp(c.ttl), xc0, yc0, 0)
    else
        print('No selection')
    end
end

function transform_single_cell_land_to_water()
    if lo.lwttl_selected(c.ttl, nil) == 1 then
        local xc0 = lo.lwttl_selected_int_x(c.ttl)
        local yc0 = lo.lwttl_selected_int_y(c.ttl)
        lo.lwttl_udp_send_ttltransformsinglecell(c.ttl, lo.lwttl_sea_udp(c.ttl), xc0, yc0, 1)
    else
        print('No selection')
    end
end

local cam_iso_top_mode = 0
function toggle_cam_iso_top_mode()
    cam_iso_top_mode = ~cam_iso_top_mode
    local eye
    if cam_iso_top_mode == 0 then
        eye = lo.new_vec3(0, 0, 10)
    else
        eye = lo.new_vec3(10, -10, 10)
    end
    lo.lwttl_set_cam_eye(c.ttl, eye)
    lo.lwttl_update_aspect_ratio(c.ttl, c.viewport_width, c.viewport_height)
    lo.delete_vec3(eye)
end

function purchase_new_port()
    print('purchase_new_port')
    lo.htmlui_execute_anchor_click(c.htmlui, "/purchaseNewPort");
end

function demolish_port(port_id)
    print('demolish_port')
    lo.htmlui_execute_anchor_click(c.htmlui, "/demolishPort?portId=" .. math.floor(port_id));
end

local CELL_MENU_PURCHASE_NEW_PORT = 1
local CELL_MENU_DETAILS = 2
local CELL_MENU_DEMOLISH_PORT = 3

function reset_cell_menu()
    lo.lwttl_clear_cell_menu(c.ttl);
    local sc = lo.lwttl_single_cell(c.ttl)
    local is_land = sc.attr & 1
    local is_water = sc.attr & 2
    local is_seawater = sc.attr & 4
    if is_land and sc.port_id <= 0 then
        lo.lwttl_add_cell_menu(c.ttl, CELL_MENU_PURCHASE_NEW_PORT, "항구건설");
    end
    lo.lwttl_add_cell_menu(c.ttl, CELL_MENU_DETAILS, "상세정보");
    if sc.port_id > 0 then
        lo.lwttl_add_cell_menu(c.ttl, CELL_MENU_DEMOLISH_PORT, "철거");
    end
end

function print_single_cell_info(sc)
    print('--single cell info--')
    for key,value in pairs(getmetatable(sc)[".get"]) do
        print(key .. ": " .. tostring(value(sc)))
    end
end

function on_cell_menu(index, command_id)
    print(string.format('on_cell_menu:%d,%d', index, command_id))
    local sc = lo.lwttl_single_cell(c.ttl)
    print_single_cell_info(sc)
    if command_id == CELL_MENU_PURCHASE_NEW_PORT then
        purchase_new_port()
    elseif command_id == CELL_MENU_DEMOLISH_PORT and sc.port_id > 0 then
        demolish_port(sc.port_id)
    end
end

function on_lwttl_prerender_mutable_context()
    print("on_lwttl_prerender_mutable_context")
    lo.lwttl_send_ping_now(c.ttl)
    lo.lwttl_send_ttlpingsinglecell_on_selected(c.ttl)
    
end

function on_ttl_single_cell()
    reset_cell_menu()
end
