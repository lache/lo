local inspect = require('inspect')
local neturl = require('neturl')
local json = require('json')

local c = lo.script_context()
lo.htmlui_set_online(c.htmlui, 0)
print('GAZZA ONLINE!!!!')

if c.tcp_ttl == nil then
    lo.lw_new_tcp_ttl_custom(c, '54.175.243.215', '8000', 8000)
end

function gazza_select_user(target_guid)
    print('target guid: ' .. target_guid)
    gazza_add_header('TargetGuid', target_guid)
    lo.htmlui_execute_anchor_click(c.htmlui, string.format('/act'))
    lo.htmlui_set_refresh_html_body(c.htmlui, 1)
end

function gazza_act(act_id)
    print('gazza_act: ' .. act_id)
    gazza_add_header('Action', act_id)
    lo.htmlui_execute_anchor_click(c.htmlui, string.format('/act'))
    lo.htmlui_set_refresh_html_body(c.htmlui, 1)
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

function on_nickname_change(nickname)
    print('on_nickname_change: ' .. nickname)
    gazza_add_header('Nickname', nickname)
    lo.htmlui_execute_anchor_click(c.htmlui, string.format('/register'))

    start_coro(function()
        while true do
            print('gaz-za!!!')
            lo.htmlui_set_online(c.htmlui, 1)
            lo.htmlui_execute_anchor_click(c.htmlui, '/ping')
            yield_wait_ms(1000)
        end
    end)
end

function unescape(text)
  for uchar in string.gmatch(text, "\\u([0-9a-f][0-9a-f][0-9a-f][0-9a-f])") do
    text = text:gsub("\\u"..uchar, utf8.char("0x"..uchar))
  end
  return text
end

local player_guid = ''

function on_json_body(json_body)
    json_body = unescape(json_body)
    print('on_json_body: ' .. json_body)
    local jb = json.parse(json_body)
    print('Turn: '..jb.turn)
    if jb.guid then
        print('Player Guid: '..jb.guid)
        player_guid = jb.guid
        gazza_add_header('Guid', jb.guid)
    end
    local user_loop_key = 'gazza-user'
    local player_loop_key = 'gazza-player'
    lo.htmlui_clear_loop(c.htmlui, user_loop_key)
    lo.htmlui_clear_loop(c.htmlui, player_loop_key)
    local target_nickname = ''
    local user_count = 0
    for i = 0, 100 do
        --print('*** User '..i)
        local user_guid = jb['user'..i..'-guid']
        if user_guid then
            local portrait_number = i + 1 --math.random(1,30)
            local portrait = string.format('atlas/captain/%02d.png', portrait_number)

            if user_guid == jb['selected-target'] then
                target_nickname = jb['user'..i..'-nickname']
            end

            if user_guid == player_guid then
                lo.htmlui_set_loop_key_value(c.htmlui, player_loop_key, 'user_nickname', jb['user'..i..'-nickname'])
                lo.htmlui_set_loop_key_value(c.htmlui, player_loop_key, 'user_portrait', portrait)
                lo.htmlui_set_loop_key_value(c.htmlui, player_loop_key, 'user_hp', jb['user'..i..'-hp'])
                lo.htmlui_set_loop_key_value(c.htmlui, player_loop_key, 'user_mp', jb['user'..i..'-mp'])
            elseif user_count < 8 then
                --print('GUID: ' .. jb['user'..i..'-guid'])
                --print('NICKNAME: ' .. jb['user'..i..'-nickname'])
                --print('HP: ' .. jb['user'..i..'-hp'])
                --print('MP: ' .. jb['user'..i..'-mp'])
                local scr = 'script:gazza_select_user(\''..user_guid..'\')'
                lo.htmlui_set_loop_key_value(c.htmlui, user_loop_key, 'user_anchor', scr)
                lo.htmlui_set_loop_key_value(c.htmlui, user_loop_key, 'user_nickname', jb['user'..i..'-nickname'])
                lo.htmlui_set_loop_key_value(c.htmlui, user_loop_key, 'user_portrait', portrait)
                lo.htmlui_set_loop_key_value(c.htmlui, user_loop_key, 'user_hp', jb['user'..i..'-hp'])
                lo.htmlui_set_loop_key_value(c.htmlui, user_loop_key, 'user_mp', jb['user'..i..'-mp'])
                user_count = user_count + 1
            end
        else
            break
        end
    end
    local remain_time = string.format('%.1f초', jb['turn-remain-time'])
    lo.htmlui_clear_loop(c.htmlui, 'gazza-turn')
    lo.htmlui_set_loop_key_value(c.htmlui, 'gazza-turn', 'turn', jb.turn..'턴')
    lo.htmlui_set_loop_key_value(c.htmlui, 'gazza-turn', 'turn_remain_time', remain_time)
    local selected_act = '미지정'
    if jb['selected-act'] == '1' then
        selected_act = '방어'
    elseif jb['selected-act'] == '2' then
        selected_act = '칼'
    elseif jb['selected-act'] == '3' then
        selected_act = '펜'
    end
    lo.htmlui_set_loop_key_value(c.htmlui, 'gazza-turn', 'selected_act', selected_act)
    lo.htmlui_set_loop_key_value(c.htmlui, 'gazza-turn', 'selected_target', target_nickname)

    lo.htmlui_set_online(c.htmlui, 0)
    lo.htmlui_execute_anchor_click(c.htmlui, 'Gazza.html')
end
