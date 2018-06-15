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

function reset_cell_menu()
    lo.lwttl_clear_cell_menu(c.ttl);
    lo.lwttl_add_cell_menu(c.ttl, "항구건설");
    lo.lwttl_add_cell_menu(c.ttl, "상세정보");
    lo.lwttl_add_cell_menu(c.ttl, "철거");
end

function on_cell_menu(index)
    print(string.format('on_cell_menu:%d', index))
    print(inspect(getmetatable(lo.lwttl_single_cell(c.ttl))))
    if index == 0 then
        purchase_new_port()
    end
end
