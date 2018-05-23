print("post init once lua")
print('lua version: ', _VERSION)
local function custom_searcher(mod)
	-- return a module loader function
	return function(mod)
		local filename = lo.script_prefix_path() .. mod .. '.lua'
		print('Loading lua module: ' .. filename)
		-- Should use 'pLwc' instead of 'c' for context variable
		-- since 'load_module' function is natively binded function.
		-- (not a component of lo lib.)
		return load_module(pLwc, filename)
	end
end
table.insert(package.searchers, 2, custom_searcher)

return 1
