local M = {}
print('testmod.lua visible')
function M.foo()
    return "Hello Module World! 5!!!"
end

function M.testcoro()
    print('testcoro 1')
    print('testcoro 2')
end

return M