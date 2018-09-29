

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

local test_source = read_all_file('assets/l/test_source.txt')
local test_source_func, test_source_errmsg = load('return '..test_source)
assert(type(test_source_func) == 'function')
assert(test_source_errmsg == nil)
print(render_html(function() return hello { my { friend { 1+2 } }} end))
print(render_html(test_source_func))
