local inspect = require('inspect')
local neturl = require('neturl')

local c = lo.script_context()
lo.htmlui_set_online(c.htmlui, 0)
print('GAZZA!!!!')

local gazza_header = {}

function gazza_select_user(target_guid)
    print('target guid: ' .. target_guid)
    gazza_add_header('Target-Guid', target_guid)
    lo.htmlui_execute_anchor_click(c.htmlui, string.format('/act'))
end

function gazza_act(act_id)
    print('gazza_act: ' .. act_id)
    gazza_add_header('Action', act_id)
    lo.htmlui_execute_anchor_click(c.htmlui, string.format('/act'))
end

function gazza_add_header(key, value)
    print('[ui-select] key:'..key..',value:'..value)
    gazza_header[key] = value
    print('=== gazza_header ===')
    print(http_header())
end

function http_header()
    local r = ''
    for key, value in pairs(gazza_header) do
        r = r .. key .. ': ' .. value .. '\r\n'
    end
    return r
end
