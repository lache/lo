local c = lo.script_context()

local before_coro_count = lo.script_running_coro_count(c.script)
print('before_coro_count:'..before_coro_count)

-- Script for testing runtime script errors are properly shown in the console
local error_test_count = 0
-- 1. Error within coroutine
start_coro(function ()
    error_test_1()
end)

-- 2. Error within coroutine within coroutine
start_coro(function ()
    start_coro(function ()
      error_test_2()
    end)
end)

-- 3. Error within coroutine within function
function testfunc()
  start_coro(function ()
      error_test_3_and_4_and_5()
  end)  
end
testfunc()

-- 4. Error within coroutine within function within function
function testfuncparent()
  testfunc()
end
testfuncparent()

-- 5. Error within function within coroutine
start_coro(function ()
  testfunc()
end)

-- 6. Error within function within function
function testfunc2()
  error_test_6()
end
function testfunc3()
  testfunc2()
end

function reload_require(modname)
	package.loaded[modname] = nil
	return require(modname)
end

start_coro(function()
    -- should also counting this coroutine; thus '+ 1'
    while lo.script_running_coro_count(c.script) ~= before_coro_count + 1 do
        yield_wait_ms(0)
        print('Waiting error_test.lua to be finished...')
    end
    print('Waiting error_test.lua to be finished...FINISHED!')
    reload_require('post_init')
    lo.lwcontext_set_safe_to_start_render(c, 1);
end)

return 1
