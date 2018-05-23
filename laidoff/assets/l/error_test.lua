-- Script for testing runtime script errors are properly shown in the console

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
testfunc3()

return 1
