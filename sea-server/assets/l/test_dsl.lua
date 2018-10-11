local info = debug.getinfo(1,'S')
print('loading '..info.source)
local inspect = require('assets/l/inspect')

local number = 15
local function lucky()
    print("your lucky number: "..number)
end
local function lucky2()
    print('your other number: '..number)
end

lucky()

local idx = 1
while true do
    local name, val = debug.getupvalue(lucky, idx)
    if not name then break end
    print(name, val)
    idx = idx + 1
end

local function get_upvalue(fn, search_name)
    local idx = 1
    while true do
        local name, val = debug.getupvalue(fn, idx)
        if not name then break end
        if name == search_name then
            return idx, val
        end
        idx = idx + 1
    end
end

debug.setupvalue(lucky, get_upvalue(lucky, 'number'), 22)
lucky()

lucky()
lucky2()

local new_env = { print = function() print('no lucky number for you!') end }
debug.upvaluejoin(lucky, get_upvalue(lucky, '_ENV'), function() return new_env end, 1)
debug.setupvalue(lucky, get_upvalue(lucky, 'number'), 2122)
lucky()
lucky2()
print('number=',number)

local function setfenv(fn, env)
    local i = 1
    while true do
        local name = debug.getupvalue(fn, i)
        if name == "_ENV" then
            debug.upvaluejoin(fn, i, (function() return env end), 1)
            break
        elseif not name then
            break
        end
  
        i = i + 1
    end
    return fn
end

local function getfenv(fn)
    local i = 1
    while true do
        local name, val = debug.getupvalue(fn, i)
        if name == "_ENV" then
            return val
        elseif not name then
            break
        end
        i = i + 1
    end
end

local function run_with_env(env, fn, ...)
    setfenv(fn, env)
    fn(...)
end

local dsl_env = {
    move = function(x, y) print('I moved to', x, y) end,
    speak = function(message) print('I said', message) end
}
run_with_env(dsl_env, function()
    move(10, 10)
    speak('I am hungry!')
end)

local void_tags = {
    img = true,
}

local function append_all(buffer, ...)
    for i=1,select('#', ...) do
        table.insert(buffer, (select(i, ...)))
    end
end

local function build_tag(tag_name, opts)
    local buffer = {'<', tag_name}
    if type(opts) == 'table' then
        for k,v in pairs(opts) do
            if type(k) ~= 'number' then
                append_all(buffer, ' ', k, '="', v, '"')
            end
        end
    end

    if void_tags[tag_name] then
        append_all(buffer, ' />')
    else
        append_all(buffer, '>')
        if type(opts) == 'table' then
            append_all(buffer, table.unpack(opts))
        else
            append_all(buffer, opts)
        end
        append_all(buffer, '</', tag_name, '>')
    end
    return table.concat(buffer)
end


local function render_html(fn)
    setfenv(fn, setmetatable({}, {
        __index = function(self, tag_name)
            return function(opts)
                return build_tag(tag_name, opts)
            end
        end
    }))
    return fn()
end

function read_all_file(file)
    local f = assert(io.open(file, "r"))
    local content = f:read("*all")
    f:close()
    return content
end

local function append_all_required_resources(resource, ...)
    -- i=1, i=2 reserved for id and name
    if select('#', ...) > 3 then resource.required_resources = {} end
    for i=3,select('#', ...) do
        table.insert(resource.required_resources, (select(i, ...)))
    end
end

local function build_combination_tag(combination_def, tag_name, opts)
    if tag_name == 'def' then
        --print('def shown')
        local resource_id = opts[1]
        local resource_name = opts[2]
        local resource_required = opts[3]
        if combination_def[resource_id] then
            error('Duplicated resource def. ID='..resource_id..', name='..resource_name)
        else
            local resource = { resource_id = resource_id, name = resource_name }
            append_all_required_resources(resource, table.unpack(opts))
            combination_def[resource_id] = resource
            return resource
        end
    elseif tag_name == 'req' then
        --print('req shown')
        local resource_id = opts[1]
        local resource_amount = opts[2]
        if combination_def[resource_id] == nil then
            error('Cannot find resource definition of ID '..resource_id)
        else
            return { resource_id = resource_id, amount = resource_amount }
        end
    elseif tag_name == 'combination' then
        --print('combination shown')
        return combination_def
    else
        error('Unknown tag name on combination source: '..tag_name)
    end
end

local function create_combination_data(fn)
    local combination_def = {}
    setfenv(fn, setmetatable({}, {
        __index = function(self, tag_name)
            return function(opts)
                return build_combination_tag(combination_def, tag_name, opts)
            end
        end
    }))
    fn()
    return combination_def
end

local test_source = read_all_file('assets/l/test_source.txt')
local test_source_func, test_source_errmsg = load('return '..test_source)
assert(type(test_source_func) == 'function')
assert(test_source_errmsg == nil)
print(render_html(function() return hello { my { friend { 1+2 } }} end))
print(render_html(test_source_func))


local combination_source = read_all_file('assets/l/combination_source.txt')
local combination_source_func, combination_source_errmsg = load('return '..combination_source)
assert(type(combination_source_func) == 'function')
assert(combination_source_errmsg == nil)
print(inspect(create_combination_data(function() return combination { def { 100, 'Test Resource ID One Hundred' } } end)))
print(inspect(create_combination_data(combination_source_func)))
