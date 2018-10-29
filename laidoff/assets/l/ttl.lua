local inspect = require('inspect')
local neturl = require('neturl')
local json = require('json')
require('tablecopy')

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
local CELL_MENU_TAG_CITY                            = 9

local CELL_MENU_MODE_NORMAL = 1
local CELL_MENU_MODE_SELECT_SEAPORT = 2
local cell_menu_mode = CELL_MENU_MODE_NORMAL
local select_var_name
local selected_seaport_id

local secure_message_counter = 0

--lo.test_srp_main()

if c.tcp_ttl ~= nil then
    print('Destroying the previous TCP connection...')
    lo.destroy_tcp(c.tcp_ttl)
    c.tcp_ttl = nil
end

if c.tcp_ttl == nil then
    --lo.lw_new_tcp_ttl_custom(c, '54.175.243.215', '8000', 8000)
    --lo.lw_new_tcp_ttl_custom(c, '127.0.0.1', '8000', 8000)
    lo.lw_new_tcp_ttl_custom(c,
                             c.tcp_ttl_host_addr.host,
                             c.tcp_ttl_host_addr.port_str,
                             tonumber(c.tcp_ttl_host_addr.port_str))
end

function on_ttl_enter()
    lo.script_cleanup_all_coros(c)
end

function worldmap_scroll(dlng, dlat, dscale)
    lo.lwttl_worldmap_scroll(c.ttl, dlng/100, dlat/100, dscale)
end

function worldmap_scroll_to_cell_center(xc, yc)
    lo.lwttl_worldmap_scroll_to_cell_center(c.ttl,
                                            xc,
                                            yc,
                                            lo.lwttl_sea_udp(c.ttl))
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
        r = r .. 'X-Select-'
              .. key:sub(1,1):upper()
              .. key:sub(2)
              .. ': '
              .. table.concat(value, ',')
              .. '\r\n'
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
        lo.lwttl_udp_send_ttltransformsinglecell(c.ttl,
                                                 lo.lwttl_sea_udp(c.ttl),
                                                 xc0,
                                                 yc0,
                                                 0)
        lo.lwttl_send_ping_now(c.ttl)
    else
        print('No selection')
    end
end

function transform_single_cell_land_to_water()
    if lo.lwttl_selected(c.ttl, nil) == 1 then
        local xc0 = lo.lwttl_selected_int_x(c.ttl)
        local yc0 = lo.lwttl_selected_int_y(c.ttl)
        lo.lwttl_udp_send_ttltransformsinglecell(c.ttl,
                                                 lo.lwttl_sea_udp(c.ttl),
                                                 xc0,
                                                 yc0,
                                                 1)
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
    if #ttl_url_history == 0 or (#ttl_url_history > 0
        and ttl_url_history[#ttl_url_history] ~= url) then
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
    lo.htmlui_execute_anchor_click(c.htmlui,
                                   string.format('/demolishPort?portId=%d',
                                                 port_id))
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
    --print('reset_cell_menu')
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
        if sc.city_id > 0 then
            lo.lwttl_add_cell_menu(c.ttl, CELL_MENU_TAG_CITY, '태그')
        end
        if sc.contract_id > 0 then
            lo.lwttl_add_cell_menu(c.ttl, CELL_MENU_TAG_CITY, '계약')
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

function on_ttl_single_cell(city_lua_data, ship_lua_data, seaport_lua_data)
    --print('on_ttl_single_cell')
    if cell_menu_mode == CELL_MENU_MODE_NORMAL then
        reset_cell_menu()
    elseif cell_menu_mode == CELL_MENU_MODE_SELECT_SEAPORT then
        local sc = lo.lwttl_single_cell(c.ttl)
        if sc.port_id > 0 then
            cell_menu_mode = CELL_MENU_MODE_SELECT_NORMAL
            reexecute_anchor_click_append_qs(select_var_name, math.floor(sc.port_id))
        end
    end
    if #city_lua_data > 0 then
        local city_lua_table = load('return '..city_lua_data)()
        print('city id',city_lua_table.city_id)
    end
    if #ship_lua_data > 0 then
        local ship_lua_table = load('return '..ship_lua_data)()
        print('ship id',ship_lua_table.ship_id)
    end
    if #seaport_lua_data > 0 then
        local seaport_lua_table = load('return '..seaport_lua_data)()
        print('seaport id',seaport_lua_table.seaport_id)
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
        lo.show_sys_msg(c.def_sys_msg, '계정 정보가 없거나 잘못됨')
        error('Auth data empty or corrupted!')
    end



    -- [2] Client: Create an account object for authentication
    print('previous auth_context.usr', auth_context.usr)
    auth_context.usr = lo.srp_user_new(alg, ng_type, username, bytes_pw, n_hex, g_hex)
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
    local errcode_s, bytes_s = lo.read_user_data_file_binary(c, 'account-s')
    local len_s = #bytes_s
    local errcode_v, bytes_v = lo.read_user_data_file_binary(c, 'account-v')
    local len_v = #bytes_v
    local errcode_pw, bytes_pw = lo.read_user_data_file_binary(c, 'account-pw')
    local len_pw = #bytes_pw
    print('[CACHED] username',username,'s',bytes_s,'#s',len_s,'v',bytes_v,'#v',len_v,'pw',bytes_pw,'#pw',len_pw)
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
        bytes_pw = {0x12,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0}
        len_pw = #bytes_pw
        bytes_s, bytes_v = lo.srp_create_salted_verification_key(alg,
                                                                 ng_type,
                                                                 username,
                                                                 bytes_pw,
                                                                 nil,
                                                                 nil)
        len_s = #bytes_s
        len_v = #bytes_v
        print('s',bytes_s,'#s',len_s,'v',bytes_v,'#v',len_v)

        -- *** User -> Host: (username, s, v) ***
    end

    local hexstr_s = lo.srp_hexify(bytes_s)
    local hexstr_v = lo.srp_hexify(bytes_v)
    print('#s',len_s,'s',hexstr_s)
    print('#v',len_v,'v',hexstr_v)
    
    -- [2] Client: Create an account object for authentication
    -- INPUT: username, password
    -- OUTPUT: usr, A
    local usr = lo.srp_user_new(alg, ng_type, username, bytes_pw, n_hex, g_hex)
    local auth_username, bytes_A = lo.srp_user_start_authentication(usr)
    local len_A = #bytes_A
    print(auth_username, bytes_A, len_A)

    local hexstr_A = lo.srp_hexify(bytes_A)
    print('#A',len_A,'A',hexstr_A)

    -- *** User -> Host: (username, A) ***

    -- [3] Server: Create a verifier
    -- INPUT: username, s, v, A
    -- OUTPUT: ver, B, K(internal)
    local ver, bytes_B = lo.srp_verifier_new(alg, ng_type, username,
                                             bytes_s,
                                             bytes_v,
                                             bytes_A,
                                             n_hex, g_hex)
    local len_B = #bytes_B
    print('ver',ver,'B',bytes_B,'#B',len_B)
    if bytes_B == nil or len_B == 0 then
        error('Verifier SRP-6a safety check violated!')
        return
    end

    local hexstr_B = lo.srp_hexify(bytes_B)
    print('len_B:',len_B,'B:',hexstr_B)

    -- *** Host -> User: (s, B) ***

    -- [4] Client
    -- INPUT: s, A(in usr), B
    -- OUTPUT: M, K(in usr)
    local bytes_M = lo.srp_user_process_challenge(usr, bytes_s, bytes_B)
    local len_M = #bytes_M
    print('M',bytes_M,'#M',len_M)
    if bytes_M == nil or len_M == 0 then
        error('User SRP-6a safety check violation!')
    end

    local hexstr_M = lo.srp_hexify(bytes_M)
    print('len_M:',len_M,'M:',hexstr_M)

    -- *** User -> Host: (username, M) ***

    -- [5] Server
    -- INPUT: ver, M
    -- OUTPUT: HAMK
    local bytes_HAMK = lo.srp_verifier_verify_session(ver, bytes_M)
    print(bytes_HAMK)
    if bytes_HAMK == nil or #bytes_HAMK == 0 then
        error('User authentication failed!')
    end

    -- *** Host -> User: (HAMK) ***

    -- [6] Client
    -- INTPUT: usr, HAMK
    -- OUTPUT: is authed?
    lo.srp_user_verify_session(usr, bytes_HAMK)
    if lo.srp_user_is_authenticated(usr) ~= 1 then
        error('Server authentication failed!')
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
end

function on_json_body(json_body)
    local jb = json.parse(json_body)
    if jb.id ~= nil and jb.result ~= nil then
        -- registration step result
        if jb.result == 'ok' and jb.id == auth_context.username then
            -- Save I(username), s, v and pw to client disk
            lo.write_user_data_file_string(c, 'account-id', auth_context.username)
            lo.write_user_data_file_binary(c, 'account-s', auth_context.bytes_s)
            lo.write_user_data_file_binary(c, 'account-v', auth_context.bytes_v)
            lo.write_user_data_file_binary(c, 'account-pw', auth_context.bytes_pw)
            lo.show_sys_msg(c.def_sys_msg, '새 계정 \''..auth_context.username..'\' 생성')
        else
            lo.show_sys_msg(c.def_sys_msg, '새 계정 \''..auth_context.username..'\' 생성 실패!!')
        end
    elseif jb.B ~= nil then
        -- startAuth1 step result
        print('B', jb.B)
        auth_context.bytes_B = lo.srp_unhexify(jb.B)
        auth_context.len_B = #auth_context.bytes_B
        print('bytes_B:',auth_context.bytes_B,'len_B:',auth_context.len_B)
        -- [4] Client
        local bytes_M = lo.srp_user_process_challenge(auth_context.usr,
                                                      auth_context.bytes_s,
                                                      auth_context.bytes_B)
        if bytes_M == nil or #bytes_M == 0 then
            error('User SRP-6a safety check violation!')
            lo.show_sys_msg(c.def_sys_msg, '인증 실패! :(')
            return
        end
        local len_M = #bytes_M
        print('bytes_M',bytes_M,'len_M',len_M)

        local hexstr_M = lo.srp_hexify(bytes_M)
        print('len_M:',len_M,'M:',hexstr_M)
        
        local hexstr_B = lo.srp_hexify(auth_context.bytes_B)

        add_custom_http_header('X-Account-B', hexstr_B)
        add_custom_http_header('X-Account-M', hexstr_M)
        lo.htmlui_execute_anchor_click(c.htmlui, '/startAuth2')
    elseif jb.HAMK ~= nil then
        -- startAuth2 step result
        print('HAMK', jb.HAMK)
        local bytes_HAMK = lo.srp_unhexify(jb.HAMK)
        local len_HAMK = #bytes_HAMK
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

local CIPHER_BLOCK_PADDING_SENTINEL = 0x80
local CIPHER_BLOCK_BYTES = 16

function add_padding_bytes_inplace(bytes_plaintext)
    --print('before add padding length',#bytes_plaintext)
    table.insert(bytes_plaintext, CIPHER_BLOCK_PADDING_SENTINEL)
    local remainder = #bytes_plaintext % CIPHER_BLOCK_BYTES
    if remainder > 0 then
        for i=1,CIPHER_BLOCK_BYTES - remainder do
            table.insert(bytes_plaintext, 0)
        end
    end
    --print('after add padding length',#bytes_plaintext)
end

function remove_padding_bytes_inplace(bytes_ciphertext)
    for i=#bytes_ciphertext,1,-1 do
        if bytes_ciphertext[i] == CIPHER_BLOCK_PADDING_SENTINEL then
            bytes_ciphertext[i] = nil
            break
        end
        bytes_ciphertext[i] = nil
    end
end

function add_padding_bytes_inplace_custom(bytes, block_size, padding_value)
    --print('before add_padding_bytes_inplace_custom() length',#bytes)
    local remainder = #bytes % block_size
    if remainder > 0 then
        for i=1,block_size - remainder do
            table.insert(bytes, padding_value)
        end
    end
    --print('after add_padding_bytes_inplace_custom() length',#bytes)
end

local function test_encrypt_decrypt(bytes_key, bytes_plaintext)
    local aes_context = lo.mbedtls_aes_context()
    lo.mbedtls_aes_init(aes_context)
    local keybits = 256 -- keybits max is 256. bytes_key may be longer then this...see comments on 'mbedtls_aes_setkey_enc()'
    if lo.mbedtls_aes_setkey_enc(aes_context, bytes_key) ~= 0 then
        lo.show_sys_msg(c.def_sys_msg, 'aes enc key set fail')
        error('aes enc key set fail')
    end
    
    local len_iv = 16 -- fixed to 16-byte
    if #bytes_plaintext % 16 ~= 0 then
        lo.show_sys_msg(c.def_sys_msg, 'input plaintext length should be multiple of 16-byte')
        error('input plaintext length should be multiple of 16-byte')
    end
    
    local result_iv, bytes_iv = lo.srp_alloc_random_bytes(len_iv)
    if result_iv ~= 0 then
        lo.show_sys_msg(c.def_sys_msg, 'cannot seed iv')
        error('cannot seed iv')
    end
    --print('bytes_iv',bytes_iv,'len_iv',len_iv)
    
    
    local block_count = #bytes_plaintext / 16
    --print('blocks', block_count)

    
    --local hexstr_iv = lo.srp_hexify(bytes_iv)
    --print('iv (before):', hexstr_iv)

    
    local encrypt_result, bytes_iv_after, bytes_ciphertext = lo.mbedtls_aes_crypt_cbc(aes_context,
                                                                                      lo.MBEDTLS_AES_ENCRYPT,
                                                                                      bytes_iv,
                                                                                      bytes_plaintext)
    if encrypt_result ~= 0 then
        lo.show_sys_msg(c.def_sys_msg, 'encrypt failed')
        error('encrypt failed')
    end

    --local hexstr_iv_after = lo.srp_hexify(bytes_iv_after)
    --print('iv (after):', hexstr_iv_after)
    --local hexstr_plaintext = lo.srp_hexify(bytes_plaintext)
    --print('plaintext', hexstr_plaintext)
    --local hexstr_ciphertext = lo.srp_hexify(bytes_ciphertext)
    --print('ciphertext', hexstr_ciphertext)
    
    -- decryption

    local aes_dec_context = lo.mbedtls_aes_context()
    lo.mbedtls_aes_init(aes_dec_context)
    if lo.mbedtls_aes_setkey_dec(aes_dec_context, bytes_key) ~= 0 then
        lo.show_sys_msg(c.def_sys_msg, 'aes dec key set fail')
        error('aes dec key set fail')
    end
    
    local decrypt_result, bytes_dec_iv_after, bytes_dec_plaintext = lo.mbedtls_aes_crypt_cbc(aes_dec_context,
                                                                                             lo.MBEDTLS_AES_DECRYPT,
                                                                                             bytes_iv,
                                                                                             bytes_ciphertext)
    if decrypt_result ~= 0 then
        lo.show_sys_msg(c.def_sys_msg, 'decrypt failed')
        error('decrypt failed')
    end

    --local hexstr_dec_plaintext = lo.srp_hexify(bytes_dec_plaintext)
    --print('dec plaintext', hexstr_dec_plaintext)

    return bytes_iv, bytes_ciphertext, bytes_dec_plaintext
end

function utf8_from(t)
  local bytearr = {}
  for _, v in ipairs(t) do
    local utf8byte = v < 0 and (0xff + v + 1) or v
    table.insert(bytearr, string.char(utf8byte))
  end
  return table.concat(bytearr)
end

function test_aes()
    if auth_context.usr == nil or lo.srp_user_is_authenticated(auth_context.usr) ~= 1 then
        lo.show_sys_msg(c.def_sys_msg, 'Invalid auth.')
        error('Invalid auth.')
    end
    local bytes_key = lo.srp_user_get_session_key(auth_context.usr)
    bytes_key = {table.unpack(bytes_key, 1, 256/8)} -- truncate bytes_key to 256-bit (32-byte)
    --local hexstr_key = lo.srp_hexify(bytes_key)
    --print('Session key (truncated):', hexstr_key)
    
    local json_plaintext = json.stringify({c=secure_message_counter,hello=10,world=20,dict={1,2,3},str='hello my friend',kor='한국루아의 표준 함수 나 제공된 문자열 기능은 utf-8을 인식하지 못한다.'})
    secure_message_counter = secure_message_counter + 1
    --print('json_plaintext', json_plaintext)
    local bytes_plaintext = { string.byte(json_plaintext, 1, -1) }
    --local hexstr_plaintext = lo.srp_hexify(bytes_plaintext)
    --print('hexstr_plaintext',hexstr_plaintext)
    add_padding_bytes_inplace(bytes_plaintext)
    
    local bytes_iv, bytes_ciphertext, bytes_dec_plaintext = test_encrypt_decrypt(bytes_key, bytes_plaintext)
    
    remove_padding_bytes_inplace(bytes_dec_plaintext)
    --local hexstr_dec_plaintext = lo.srp_hexify(bytes_dec_plaintext)
    --print('hexstr_dec_plaintext',hexstr_dec_plaintext)
    local json_dec_plaintext = utf8_from(bytes_dec_plaintext)
    --print('json_dec_plaintext', json_dec_plaintext)
    --local hexstr_iv = lo.srp_hexify(bytes_iv)
    --print('iv', hexstr_iv)
    lo.show_sys_msg(c.def_sys_msg,
                    'plaintext:' .. json_plaintext .. '\n'
                    ..'plaintext:' .. json_dec_plaintext)
    local bytes_account_id = { string.byte(auth_context.username, 1, -1) }
    table.insert(bytes_account_id, 0) -- null terminate
    add_padding_bytes_inplace_custom(bytes_account_id, 4, 0)
    local msg = table.copy({lo.LPGP_LWPTTLJSON, 0, 0, 0}, bytes_account_id, bytes_iv, bytes_ciphertext)
    lo.udp_send(lo.lwttl_sea_udp(c.ttl), msg)
end

function test_sea_spawn_encrypted()
    if auth_context.usr == nil or lo.srp_user_is_authenticated(auth_context.usr) ~= 1 then
        lo.show_sys_msg(c.def_sys_msg, 'Invalid auth.')
        error('Invalid auth.')
    end
    local bytes_key = lo.srp_user_get_session_key(auth_context.usr)
    bytes_key = {table.unpack(bytes_key, 1, 256/8)} -- truncate bytes_key to 256-bit (32-byte)

    local xc0 = lo.lwttl_selected_int_x(c.ttl)
    local yc0 = lo.lwttl_selected_int_y(c.ttl)
    local ship_id = 99
    local m = 'sea_spawn_without_id'
    local json_plaintext = json.stringify({c=secure_message_counter,m=m,a1=xc0,a2=yc0})
    secure_message_counter = secure_message_counter + 1
    local bytes_plaintext = { string.byte(json_plaintext, 1, -1) }
    add_padding_bytes_inplace(bytes_plaintext)
    
    local bytes_iv, bytes_ciphertext, bytes_dec_plaintext = test_encrypt_decrypt(bytes_key, bytes_plaintext)
    
    remove_padding_bytes_inplace(bytes_dec_plaintext)
    local json_dec_plaintext = utf8_from(bytes_dec_plaintext)
    --[[lo.show_sys_msg(c.def_sys_msg,
                    'plaintext:' .. json_plaintext .. '\n'
                    ..'plaintext:' .. json_dec_plaintext)
                    ]]
    local bytes_account_id = { string.byte(auth_context.username, 1, -1) }
    table.insert(bytes_account_id, 0) -- null terminate
    add_padding_bytes_inplace_custom(bytes_account_id, 4, 0)
    local msg = table.copy({lo.LPGP_LWPTTLJSON, 0, 0, 0}, bytes_account_id, bytes_iv, bytes_ciphertext)
    lo.udp_send(lo.lwttl_sea_udp(c.ttl), msg)
end

function on_chat(line)
    print('on_chat:'..line)
    lo.lwttl_udp_send_ttlchat(c.ttl, lo.lwttl_sea_udp(c.ttl), line)
    c.show_chat_window = 0
    c.focus_chat_input = 0
    on_nickname_change(line)
end
