local inspect = require('inspect')
local neturl = require('neturl')
local json = require('json')

local c = lo.script_context()
lo.htmlui_set_online(c.htmlui, 0)

local select_context = {}
local custom_http_headers = {}
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

--lo.test_srp_main()

if c.tcp_ttl ~= nil then
    print('Destroying the previous TCP connection...')
    lo.destroy_tcp(c.tcp_ttl)
    c.tcp_ttl = nil
end

if c.tcp_ttl == nil then
    --lo.lw_new_tcp_ttl_custom(c, '54.175.243.215', '8000', 8000)
    --lo.lw_new_tcp_ttl_custom(c, '127.0.0.1', '8000', 8000)
    lo.lw_new_tcp_ttl_custom(c, c.tcp_ttl_host_addr.host, c.tcp_ttl_host_addr.port_str, tonumber(c.tcp_ttl_host_addr.port_str))
end

function on_ttl_enter()
    lo.script_cleanup_all_coros(c)
end

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

function add_custom_http_header(type, id)
    custom_http_headers[type] = id
end

function http_header()
    local r = ''
    -- Select Context
    for key, value in pairs(select_context) do
        r = r .. 'X-Select-' .. key:sub(1,1):upper()..key:sub(2) .. ': ' .. table.concat(value, ',') .. '\r\n'
    end
    -- Custom Headers
    for key, value in pairs(custom_http_headers) do
        r = r .. key .. ': ' .. value .. '\r\n'
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
    --print_single_cell_info(sc)
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

function open_hire_captain(ship_id)
    print('open_hire_captain')
    execute_anchor_click_with_history(string.format('/openHireCaptain?shipId=%d', ship_id))
end

function confirm_hire_captain(ship_id, captain_template_id)
    --lo.show_sys_msg(c.def_sys_msg, '히힛 미구현 기능...')
    print('confirm_hire_captain')
    lo.htmlui_execute_anchor_click(c.htmlui, string.format('/confirmHireCaptain?shipId=%d&captainTemplateId=%d', ship_id, captain_template_id))
end

function fire_captain(ship_id)
    print('fire_captain')
    lo.htmlui_execute_anchor_click(c.htmlui, string.format('/fireCaptain?shipId=%d', ship_id))
end

-- Shared constants regarding to auth
local alg = lo.SHARED_SRP_HASH
local ng_type = lo.SHARED_SRP_NG
local n_hex = nil
local g_hex = nil
local auth_context = {}

function start_account_auth()
    local errcode_username, username = lo.read_user_data_file_string(c, 'account-id')
    local errcode_s, bytes_s, len_s = lo.read_user_data_file_binary(c, 'account-s')
    local errcode_v, bytes_v, len_v = lo.read_user_data_file_binary(c, 'account-v')
    local errcode_pw, bytes_pw, len_pw = lo.read_user_data_file_binary(c, 'account-pw')
    print(username, bytes_s, len_s, bytes_v, len_v, bytes_pw, len_pw)
    if errcode_username == 0 and errcode_s == 0 and errcode_v == 0 and errcode_pw == 0 then
        print('Auth data loaded successfully.')
    else
        error('Auth data empty or corrupted!')
        lo.show_sys_msg(c.def_sys_msg, '계정 정보가 없거나 잘못됨')
        return
    end



    -- [2] Client: Create an account object for authentication
    auth_context.usr = lo.srp_user_new(alg, ng_type, username, bytes_pw, len_pw, n_hex, g_hex)
    local auth_username, bytes_A = lo.srp_user_start_authentication(auth_context.usr)
    local len_A = #bytes_A
    print('username:',auth_username, 'bytes_A:',bytes_A, 'len_A:',len_A)
    local hexstr_A = lo.srp_hexify(bytes_A)
    print('hexstr_A', hexstr_A)
    -- Send 'A' to server
    add_custom_http_header('X-Account-Id', username)
    add_custom_http_header('X-Account-A', hexstr_A)
    lo.htmlui_execute_anchor_click(c.htmlui, '/startAuth1')
    auth_context.bytes_s = bytes_s
    auth_context.len_s = len_s
    auth_context.bytes_v = bytes_v
    auth_context.len_v = len_v
    auth_context.bytes_pw = bytes_pw
    auth_context.len_pw = len_pw

    -- store 'A' for debugging purpose
    auth_context.bytes_A = bytes_A
    auth_context.len_A = len_A

    --[[
    local ver, bytes_B, len_B = lo.srp_verifier_new(alg, ng_type, username, bytes_s, len_s, bytes_v, len_v, bytes_A, len_A, n_hex, g_hex)
    print('ver:',ver, 'bytes_B:',bytes_B, 'len_B:',len_B)
    local hexstr_s = lo.srp_hexify(bytes_s, len_s)
    local hexstr_v = lo.srp_hexify(bytes_v, len_v)
    local hexstr_B = lo.srp_hexify(bytes_B, len_B)
    print('hexstr_s', hexstr_s)
    print('hexstr_v', hexstr_v)
    
    print('hexstr_B', hexstr_B)
    ]]--
end

function create_account_force()
    print('create_account_force')
    auth_context.force_create = true
    lo.start_nickname_text_input_activity(c)
end

function create_account()
    print('create_account')
    auth_context.force_create = false
    lo.start_nickname_text_input_activity(c)
end

function on_nickname_change(nickname)
    print('on_nickname_change: ' .. nickname)
    if #nickname > 0 then
        if nickname:match("%W") then
            lo.show_sys_msg(c.def_sys_msg, '닉네임은 알파벳과 숫자만 허용됩니다.')
        else
            auth_context.username = nickname
            create_account_ex(auth_context.force_create)
        end
    else
        lo.show_sys_msg(c.def_sys_msg, '닉네임을 지정해야합니다.')
    end
end

function create_account_ex(force)
    print('create_account_ex')
    print('alg:'..alg)
    print('ng_type:'..ng_type)
    local errcode_username, username = lo.read_user_data_file_string(c, 'account-id')
    local errcode_s, bytes_s, len_s = lo.read_user_data_file_binary(c, 'account-s')
    local errcode_v, bytes_v, len_v = lo.read_user_data_file_binary(c, 'account-v')
    local errcode_pw, bytes_pw, len_pw = lo.read_user_data_file_binary(c, 'account-pw')
    print('username(cached):',username, bytes_s, len_s, bytes_v, len_v, bytes_pw, len_pw)
    if force == false
       and errcode_username == 0
       and errcode_s == 0
       and errcode_v == 0
       and errcode_pw == 0 then
        lo.show_sys_msg(c.def_sys_msg, '이미 \''..username..'\' 계정 캐시 존재!')
        auth_context.username = username
    else
        username = auth_context.username --'testuser2'

        -- [1] Client: Register a new account
        -- INPUT: username, password
        -- OUTPUT: s, v
        len_pw = 8
        bytes_pw = lo.new_LWUNSIGNEDCHAR(len_pw)

        lo.LWUNSIGNEDCHAR_setitem(bytes_pw, 0, 0x12)
        lo.LWUNSIGNEDCHAR_setitem(bytes_pw, 1, 0x34)
        lo.LWUNSIGNEDCHAR_setitem(bytes_pw, 2, 0x56)
        lo.LWUNSIGNEDCHAR_setitem(bytes_pw, 3, 0x78)
        lo.LWUNSIGNEDCHAR_setitem(bytes_pw, 4, 0x9a)
        lo.LWUNSIGNEDCHAR_setitem(bytes_pw, 5, 0xbc)
        lo.LWUNSIGNEDCHAR_setitem(bytes_pw, 6, 0xde)
        lo.LWUNSIGNEDCHAR_setitem(bytes_pw, 7, 0xf0)
        lo.LWUNSIGNEDCHAR_setitem(bytes_pw, 8, 0x00) -- debugging purpose (null-terminated string)
        bytes_s, len_s, bytes_v, len_v = lo.srp_create_salted_verification_key(
            alg,
            ng_type,
            username,
            bytes_pw,
            len_pw,
            nil,
            nil)
        print(bytes_s, len_s, bytes_v, len_v)

        -- *** User -> Host: (username, s, v) ***
    end

    local hexstr_s = lo.srp_hexify(bytes_s, len_s)
    local hexstr_v = lo.srp_hexify(bytes_v, len_v)
    print('len_s:',len_s,'s:',hexstr_s)
    print('len_v:',len_v,'v:',hexstr_v)
    
    -- [2] Client: Create an account object for authentication
    -- INPUT: username, password
    -- OUTPUT: usr, A
    local usr = lo.srp_user_new(alg, ng_type, username, bytes_pw, len_pw, n_hex, g_hex)
    local auth_username, bytes_A, len_A = lo.srp_user_start_authentication(usr)
    print(auth_username, bytes_A, len_A)

    local hexstr_A = lo.srp_hexify(bytes_A, len_A)
    print('len_A:',len_A,'A:',hexstr_A)

    -- *** User -> Host: (username, A) ***

    -- [3] Server: Create a verifier
    -- INPUT: username, s, v, A
    -- OUTPUT: ver, B, K(internal)
    local ver, bytes_B, len_B = lo.srp_verifier_new(alg, ng_type, username,
                                                    bytes_s, len_s,
                                                    bytes_v, len_v,
                                                    bytes_A, len_A,
                                                    n_hex, g_hex)
    print(ver, bytes_B, len_B)
    if bytes_B == nil then
        error('Verifier SRP-6a safety check violated!')
        return
    end

    local hexstr_B = lo.srp_hexify(bytes_B, len_B)
    print('len_B:',len_B,'B:',hexstr_B)

    -- *** Host -> User: (s, B) ***

    -- [4] Client
    -- INPUT: s, A(in usr), B
    -- OUTPUT: M, K(in usr)
    local bytes_M, len_M = lo.srp_user_process_challenge(usr, bytes_s, len_s, bytes_B, len_B)
    print(bytes_M, len_M)
    if bytes_M == nil then
        error('User SRP-6a safety check violation!')
        return
    end

    local hexstr_M = lo.srp_hexify(bytes_M, len_M)
    print('len_M:',len_M,'M:',hexstr_M)

    -- *** User -> Host: (username, M) ***

    -- [5] Server
    -- INPUT: ver, M
    -- OUTPUT: HAMK
    local bytes_HAMK = lo.srp_verifier_verify_session(ver, bytes_M)
    print(bytes_HAMK)
    if bytes_HAMK == nil then
        error('User authentication failed!')
        return
    end

    -- *** Host -> User: (HAMK) ***

    -- [6] Client
    -- INTPUT: usr, HAMK
    -- OUTPUT: is authed?
    lo.srp_user_verify_session(usr, bytes_HAMK)
    if lo.srp_user_is_authenticated(usr) ~= 1 then
        error('Server authentication failed!')
        return
    end

    -- save these values to disk only after successful response received.
    auth_context.bytes_s = bytes_s
    auth_context.len_s = len_s
    auth_context.bytes_v = bytes_v
    auth_context.len_v = len_v
    auth_context.bytes_pw = bytes_pw
    auth_context.len_pw = len_pw

    -- Send I(username), s and v to server
    add_custom_http_header('X-Account-Id', username)
    add_custom_http_header('X-Account-S', hexstr_s)
    add_custom_http_header('X-Account-V', hexstr_v)
    lo.htmlui_execute_anchor_click(c.htmlui, '/createAccount')


    --lo.delete_LWUNSIGNEDCHAR(bytes_pw)
    --len_pw = 0
end

function on_json_body(json_body)
    local jb = json.parse(json_body)
    if jb.id ~= nil and jb.result ~= nil then
        -- registration step result
        if jb.result == 'ok' and jb.id == auth_context.username then
            -- Save I(username), s, v and pw to client disk
            lo.write_user_data_file_string(c, 'account-id', auth_context.username)
            lo.write_user_data_file_binary(c, 'account-s', auth_context.bytes_s, auth_context.len_s)
            lo.write_user_data_file_binary(c, 'account-v', auth_context.bytes_v, auth_context.len_v)
            lo.write_user_data_file_binary(c, 'account-pw', auth_context.bytes_pw, auth_context.len_pw)
            lo.show_sys_msg(c.def_sys_msg, '새 계정 \''..auth_context.username..'\' 생성')
        else
            lo.show_sys_msg(c.def_sys_msg, '새 계정 \''..auth_context.username..'\' 생성 실패!!')
        end
    elseif jb.B ~= nil then
        -- startAuth1 step result
        print('B', jb.B)
        auth_context.bytes_B, auth_context.len_B = lo.srp_unhexify(jb.B)
        print('bytes_B:',auth_context.bytes_B,'len_B:',auth_context.len_B)
        -- [4] Client
        local bytes_M = lo.srp_user_process_challenge(auth_context.usr,
                                                      auth_context.bytes_s,
                                                      auth_context.len_s,
                                                      auth_context.bytes_B,
                                                      auth_context.len_B)
        if bytes_M == nil then
            error('User SRP-6a safety check violation!')
            lo.show_sys_msg(c.def_sys_msg, '인증 실패! :(')
            return
        end
        local len_M = #bytes_M
        print('bytes_M',bytes_M,'len_M',len_M)

        local hexstr_M = lo.srp_hexify(bytes_M)
        print('len_M:',len_M,'M:',hexstr_M)

        add_custom_http_header('X-Account-M', hexstr_M)
        lo.htmlui_execute_anchor_click(c.htmlui, '/startAuth2')
    elseif jb.HAMK ~= nil then
        -- startAuth2 step result
        print('HAMK', jb.HAMK)
        local bytes_HAMK, len_HAMK = lo.srp_unhexify(jb.HAMK)
        print(bytes_HAMK, len_HAMK)
        lo.srp_user_verify_session(auth_context.usr, bytes_HAMK)
        if lo.srp_user_is_authenticated(auth_context.usr) ~= 1 then
            error('Server authentication failed!')
            lo.show_sys_msg(c.def_sys_msg, '인증 실패!! :(')
            return
        end
        print('Authed :)')
        

        auth_context.username = lo.srp_user_get_username(auth_context.usr)
        lo.show_sys_msg(c.def_sys_msg, '계정 \''..auth_context.username..'\' 인증 성공 :)')
        
    else
        error('Unknown JSON body')
    end
end

function test_aes()
    if auth_context.usr == nil or lo.srp_user_is_authenticated(auth_context.usr) ~= 1 then
        lo.show_sys_msg(c.def_sys_msg, 'Invalid auth.')
        error('Invalid auth.')
    end
    local bytes_key = lo.srp_user_get_session_key(auth_context.usr)
    local len_key = #bytes_key
    print('bytes_key',bytes_key,'len_key',len_key)
    local hexstr_key = lo.srp_hexify(bytes_key)
    print('Session key:', hexstr_key)
    
    local aes_context = lo.mbedtls_aes_context()
    local keybits = 256 -- see comments on 'mbedtls_aes_setkey_enc()'
    if lo.mbedtls_aes_setkey_enc(aes_context, bytes_key, keybits) ~= 0 then
        lo.show_sys_msg(c.def_sys_msg, 'aes key set fail')
        error('aes key set fail')
    end
    
    local len_iv = 16 -- fixed to 16
    local len_input = 16 -- should be multiple of 16
    local len_output = len_input

    local result_iv, bytes_iv = lo.srp_alloc_random_bytes(len_iv)
    if result_iv ~= 0 then
        lo.show_sys_msg(c.def_sys_msg, 'cannot seed iv')
        error('cannot seed iv')
    end
    print('bytes_iv',bytes_iv,'len_iv',len_iv)
    
    --[[local bytes_input = lo.new_LWUNSIGNEDCHAR(len_input)
    lo.LWUNSIGNEDCHAR_setitem(bytes_input, 0, 0x00)
    lo.LWUNSIGNEDCHAR_setitem(bytes_input, 1, 0x00)
    lo.LWUNSIGNEDCHAR_setitem(bytes_input, 2, 0x00)
    lo.LWUNSIGNEDCHAR_setitem(bytes_input, 3, 0x01)
    lo.LWUNSIGNEDCHAR_setitem(bytes_input, 4, 0x00)
    lo.LWUNSIGNEDCHAR_setitem(bytes_input, 5, 0x00)
    lo.LWUNSIGNEDCHAR_setitem(bytes_input, 6, 0x00)
    lo.LWUNSIGNEDCHAR_setitem(bytes_input, 7, 0x02)
    lo.LWUNSIGNEDCHAR_setitem(bytes_input, 8, 0x00)
    lo.LWUNSIGNEDCHAR_setitem(bytes_input, 9, 0x00)
    lo.LWUNSIGNEDCHAR_setitem(bytes_input, 10, 0x00)
    lo.LWUNSIGNEDCHAR_setitem(bytes_input, 11, 0x03)
    lo.LWUNSIGNEDCHAR_setitem(bytes_input, 12, 0x00)
    lo.LWUNSIGNEDCHAR_setitem(bytes_input, 13, 0x00)
    lo.LWUNSIGNEDCHAR_setitem(bytes_input, 14, 0x00)
    lo.LWUNSIGNEDCHAR_setitem(bytes_input, 15, 0x04)]]--
    local bytes_input = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f}
    
    local bytes_output = lo.new_LWUNSIGNEDCHAR(len_output)
    
    local hexstr_iv = lo.srp_hexify(bytes_iv)
    print('iv (before):', hexstr_iv)
    
    local crypt_result, bytes_iv_after, bytes_output = lo.mbedtls_aes_crypt_cbc(aes_context,
                                                                                lo.MBEDTLS_AES_ENCRYPT,
                                                                                bytes_iv,
                                                                                bytes_input)
    if lo.mbedtls_aes_crypt_cbc(aes_context, lo.MBEDTLS_AES_ENCRYPT,
                                16, bytes_iv, bytes_input, bytes_output) ~= 0 then
        lo.show_sys_msg(c.def_sys_msg, 'encrypt failed')
        error('encrypt failed')
    end
    
    hexstr_iv = lo.srp_hexify(bytes_iv, len_iv)
    print('iv (after):', hexstr_iv)
    local hexstr_input = lo.srp_hexify(bytes_input, len_input)
    print('input:', hexstr_input)
    local hexstr_output = lo.srp_hexify(bytes_output, len_output)
    print('output:', hexstr_output)
    lo.show_sys_msg(c.def_sys_msg, 'input:'..hexstr_input..'\n'..'output:'..hexstr_output)
end

function on_chat(line)
    print('on_chat:'..line)
    lo.lwttl_udp_send_ttlchat(c.ttl, lo.lwttl_sea_udp(c.ttl), line)
    c.show_chat_window = 0
    c.focus_chat_input = 0
    on_nickname_change(line)
end
