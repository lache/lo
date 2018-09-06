local c = lo.script_context()
lo.htmlui_set_online(c.htmlui, 0)

function on_mocap_enter()
    lo.script_cleanup_all_coros(c)
end

local asf = lo.lwasf_new_from_file('assets/asf/07-walk.asf')
local amc = lo.lwamc_new_from_file('assets/amc/07_05-walk.amc', asf)
