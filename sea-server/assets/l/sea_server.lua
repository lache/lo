local inspect = require('assets/l/inspect')
local info = debug.getinfo(1,'S')
print('loading '..info.source)
local json = require('assets/l/json')

function make_reply_json(message_counter, result_code, note)
    return json.stringify({c=message_counter,rc=result_code,note=note})
end

return 2016
