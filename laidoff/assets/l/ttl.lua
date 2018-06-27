local inspect = require('inspect')
local neturl = require('neturl')

local c = lo.script_context()
lo.htmlui_set_online(c.htmlui, 0)

local select_context = {}
local way_mode = 0
local cam_iso_top_mode = 0
local ttl_url_history = {}

local CELL_MENU_PURCHASE_NEW_PORT                   = 1
local CELL_MENU_DEMOLISH_PORT                       = 2
local CELL_MENU_TRANSFORM_SINGLE_CELL_WATER_TO_LAND = 3
local CELL_MENU_TRANSFORM_SINGLE_CELL_LAND_TO_WATER = 4
local CELL_MENU_PURCHASE_NEW_SHIPYARD               = 5
local CELL_MENU_DEMOLISH_SHIPYARD                   = 6
local CELL_MENU_SHIPYARD                            = 7
local CELL_MENU_SELECT_SEAPORT                      = 8

local CELL_MENU_MODE_NORMAL = 1
local CELL_MENU_MODE_SELECT_SEAPORT = 2
local cell_menu_mode = CELL_MENU_MODE_NORMAL
local select_var_name
local selected_seaport_id

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
    lo.tcp_request_landing_page(c.tcp_ttl, c.user_data_path, c.ttl)
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

function enable_water_way()
    way_mode = 0
end

function enable_land_way()
    way_mode = 1
end

function link()
    print('link')
    if way_mode == 0 then
        lo.htmlui_execute_anchor_click(c.htmlui, '/link')
    else
        lo.htmlui_execute_anchor_click(c.htmlui, '/linkland')
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
        lo.lwttl_send_ping_now(c.ttl)
    else
        print('No selection')
    end
end

function transform_single_cell_land_to_water()
    if lo.lwttl_selected(c.ttl, nil) == 1 then
        local xc0 = lo.lwttl_selected_int_x(c.ttl)
        local yc0 = lo.lwttl_selected_int_y(c.ttl)
        lo.lwttl_udp_send_ttltransformsinglecell(c.ttl, lo.lwttl_sea_udp(c.ttl), xc0, yc0, 1)
        lo.lwttl_send_ping_now(c.ttl)
    else
        print('No selection')
    end
end

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

function execute_anchor_click_with_history(url)
    -- prevent from the duplicated entry inserted to history
    if #ttl_url_history == 0 or (#ttl_url_history > 0 and ttl_url_history[#ttl_url_history] ~= url) then
        table.insert(ttl_url_history, url)
    end
    lo.htmlui_execute_anchor_click(c.htmlui, url)
end

function purchase_new_port()
    print('purchase_new_port')
    lo.htmlui_execute_anchor_click(c.htmlui, '/purchaseNewPort')
end

function demolish_port(port_id)
    print('demolish_port')
    lo.htmlui_execute_anchor_click(c.htmlui, string.format('/demolishPort?portId=%d', port_id))
end

function purchase_new_shipyard()
    print('purchase_new_shipyard')
    lo.htmlui_execute_anchor_click(c.htmlui, '/purchaseNewShipyard')
end

function demolish_shipyard(shipyard_id)
    print('demolish_shipyard')
    lo.htmlui_execute_anchor_click(c.htmlui, string.format('/demolishShipyard?shipyardId=%d', shipyard_id))
end

function open_shipyard(shipyard_id)
    print('open_shipyard')
    execute_anchor_click_with_history(string.format('/openShipyard?shipyardId=%d', shipyard_id))
end

function open_ship(ship_id)
    print('open_ship')
    execute_anchor_click_with_history(string.format('/openShip?shipId=%d', ship_id))
end

function open_ship_without_history(ship_id)
    print('open_ship_without_history')
    lo.htmlui_execute_anchor_click(c.htmlui, string.format('/openShip?shipId=%d', ship_id))
end

function sell_ship(ship_id)
    print('sell_ship')
    lo.htmlui_execute_anchor_click(c.htmlui, string.format('/sellShip?shipId=%d', ship_id))
end

function return_to_idle()
    print('return_to_idle')
    execute_anchor_click_with_history('/idle')
end

function purchase_ship_at_shipyard(shipyard_id, ship_template_id)
    print('purchase_ship_at_shipyard')
    -- no url history should be created for this request since it is redirected to the same url
    lo.htmlui_execute_anchor_click(c.htmlui, string.format('/purchaseShipAtShipyard?shipyardId=%d&shipTemplateId=%d', shipyard_id, ship_template_id))
end

function ttl_refresh(qs)
    if #ttl_url_history > 0 then
        local url = ttl_url_history[#ttl_url_history]
        if qs then
            local qbeg, qend = string.find(url, '?')
            if qbeg and qend then
                -- if previous_url already has its query string,
                -- qs's prepended ? should to changed to &
                qs = '&' .. string.gsub(qs, '?', '')
            end
            url = url .. qs
        end
        lo.htmlui_execute_anchor_click(c.htmlui, url)
    end
end

function ttl_go_back(qs)
    print(string.format('ttl_go_back - %s - (%d entries)', qs, #ttl_url_history))
    -- for i, v in ipairs(ttl_url_history) do print(i, v) end
    -- remove last url entry on history (which is the current url)
    if #ttl_url_history > 0 then
        table.remove(ttl_url_history, #ttl_url_history)
    end
    -- get previous url from history; or go to /idle if history is empty
    local previous_url
    if #ttl_url_history > 0 then
        previous_url = ttl_url_history[#ttl_url_history]
    else
        previous_url = '/idle'
    end
    -- check for query string on previous_url
    if qs then
        local qbeg, qend = string.find(previous_url, '?')
        if qbeg and qend then
            -- if previous_url already has its query string,
            -- qs's prepended ? should to changed to &
            qs = '&' .. string.gsub(qs, '?', '')
        end
        previous_url = previous_url .. qs
    end
    lo.htmlui_execute_anchor_click(c.htmlui, previous_url)
    -- revert cell menu mode
    cell_menu_mode = CELL_MENU_MODE_NORMAL
    reset_cell_menu()
end

function reset_cell_menu()
    print('reset_cell_menu')
    lo.lwttl_clear_cell_menu(c.ttl)
    local sc = lo.lwttl_single_cell(c.ttl)
    local is_land = sc.attr & 1
    local is_water = sc.attr & 2
    local is_seawater = sc.attr & 4
    local empty_cell = sc.port_id <= 0 and sc.city_id <= 0 and sc.shipyard_id <= 0
    if cell_menu_mode == CELL_MENU_MODE_NORMAL then
        if is_water ~= 0 and empty_cell then
            lo.lwttl_add_cell_menu(c.ttl, CELL_MENU_PURCHASE_NEW_PORT, '항구건설')
            lo.lwttl_add_cell_menu(c.ttl, CELL_MENU_PURCHASE_NEW_SHIPYARD, '조선소건설')
        end
        if empty_cell then
            if is_water ~= 0 then
                lo.lwttl_add_cell_menu(c.ttl, CELL_MENU_TRANSFORM_SINGLE_CELL_WATER_TO_LAND, '땅으로 변환')
            elseif is_land ~= 0 then
                lo.lwttl_add_cell_menu(c.ttl, CELL_MENU_TRANSFORM_SINGLE_CELL_LAND_TO_WATER, '물로 변환')
            end
        end
        if sc.port_id > 0 then
            lo.lwttl_add_cell_menu(c.ttl, CELL_MENU_DEMOLISH_PORT, '항구철거')
        end
        if sc.shipyard_id > 0 then
            lo.lwttl_add_cell_menu(c.ttl, CELL_MENU_DEMOLISH_SHIPYARD, '조선소철거')
            lo.lwttl_add_cell_menu(c.ttl, CELL_MENU_SHIPYARD, '상세메뉴')
        end
    end
end

function print_single_cell_info(sc)
    print('--single cell info--')
    for key,value in pairs(getmetatable(sc)['.get']) do
        print(key .. ': ' .. tostring(value(sc)))
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
    elseif command_id == CELL_MENU_TRANSFORM_SINGLE_CELL_WATER_TO_LAND then
        transform_single_cell_water_to_land()
    elseif command_id == CELL_MENU_TRANSFORM_SINGLE_CELL_LAND_TO_WATER then
        transform_single_cell_land_to_water()
    elseif command_id == CELL_MENU_PURCHASE_NEW_SHIPYARD then
        purchase_new_shipyard()
    elseif command_id == CELL_MENU_DEMOLISH_SHIPYARD then
        demolish_shipyard(sc.shipyard_id)
    elseif command_id == CELL_MENU_SHIPYARD then
        open_shipyard(sc.shipyard_id)
    elseif command_id == CELL_MENU_SELECT_SEAPORT then
        --local url = ttl_url_history[#ttl_url_history]
        --local parsed_url = neturl.parse(url)
        --parsed_url.query[select_var_name] = math.floor(selected_seaport_id)
        --local new_url = parsed_url:build()
        --lo.htmlui_execute_anchor_click(c.htmlui, new_url)
        --ttl_url_history[#ttl_url_history] = new_url
    end
end

function reexecute_anchor_click_append_qs(var_name, var_val)
    if #ttl_url_history > 0 then
        local url = ttl_url_history[#ttl_url_history]
        local parsed_url = neturl.parse(url)
        parsed_url.query[var_name] = var_val
        local new_url = parsed_url:build()
        lo.htmlui_execute_anchor_click(c.htmlui, new_url)
        ttl_url_history[#ttl_url_history] = new_url
    end
end

function on_set_refresh_html_body()
    print('on_set_refresh_html_body')
    lo.lwttl_send_ping_now(c.ttl)
    lo.lwttl_send_ttlpingsinglecell_on_selected(c.ttl)
end

function on_ttl_single_cell()
    print('on_ttl_single_cell')
    if cell_menu_mode == CELL_MENU_MODE_NORMAL then
        reset_cell_menu()
    elseif cell_menu_mode == CELL_MENU_MODE_SELECT_SEAPORT then
        local sc = lo.lwttl_single_cell(c.ttl)
        if sc.port_id > 0 then
            cell_menu_mode = CELL_MENU_MODE_SELECT_NORMAL
            reexecute_anchor_click_append_qs(select_var_name, math.floor(sc.port_id))
        end
    end
end

function on_ttl_static_state2()
    lo.lwttl_send_ttlpingsinglecell_on_selected(c.ttl)
end

function select_seaport(var_name)
    select_var_name = var_name
    cell_menu_mode = CELL_MENU_MODE_SELECT_SEAPORT
    lo.lwttl_clear_selected(c.ttl)
    reset_cell_menu()
end

function confirm_new_route(ship_id, seaport1_id, seaport2_id)
    print('confirm_new_route')
    lo.htmlui_execute_anchor_click(c.htmlui, string.format('/confirmNewRoute?shipId=%d&seaport1Id=%d&seaport2Id=%d', ship_id, seaport1_id, seaport2_id))
end

function delete_route(ship_id)
    print('delete_route')
    lo.htmlui_execute_anchor_click(c.htmlui, string.format('/deleteRoute?shipId=%d', ship_id))
end

function start_route(ship_id)
    print('start_route')
    lo.htmlui_execute_anchor_click(c.htmlui, string.format('/startRoute?shipId=%d', ship_id))
end

function string.starts(String,Start)
   return string.sub(String,1,string.len(Start))==Start
end

function on_ttl_ship_selected(ship_id)
    print('on_ttl_ship_selected')
    if cell_menu_mode == CELL_MENU_MODE_NORMAL then
        if #ttl_url_history == 0 then
            open_ship(ship_id)
        elseif string.starts(ttl_url_history[#ttl_url_history], '/openShip') then
            open_ship_without_history(ship_id)
        end
    end
end

function move_to_nearest_shipyard(ship_id)
    print('move_to_nearest_shipyard')
    lo.htmlui_execute_anchor_click(c.htmlui, string.format('/moveToNearestShipyard?shipId=%d', ship_id))
end