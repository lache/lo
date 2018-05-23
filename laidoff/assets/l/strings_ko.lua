local TT = {
	["STR_POINTS_ACQUIRED"]="%d%s을 획득했습니다!",
	["STR_POINTS_LOST"]="%d%s을 잃었습니다!",
	["STR_POINTS_SINGULAR"]="점",
	["STR_POINTS_PLURAL"]="점",
}
return setmetatable({}, {__index = function (t, k)
	if TT[k] ~= nil then return TT[k] end
	return k
end})
