local c = lo.script_context()
lo.htmlui_set_online(c.htmlui, 0)

function on_mocap_enter()
    lo.script_cleanup_all_coros(c)
end

local asf = lo.lwasf_new_from_file('assets/asf/07-walk.asf')
local amc = lo.lwamc_new_from_file('assets/amc/07_05-walk.amc', asf)

local sg = lo.lwsg_new()
local sgobj = lo.lwsg_new_object(sg, 'test obj', sg.root)
sgobj.lvt = lo.LVT_PUCK_PLAYER
lo.lwsg_set_local_pos(sgobj, 0,1,0)
lo.lwsg_set_local_pos(sgobj, 1,1,0)
lo.lwsg_set_local_pos(sgobj, 1,2,0)

local sgobj2 = lo.lwsg_new_object(sg, 'test obj 2', sg.root)
sgobj2.lvt = lo.LVT_SUZANNE
lo.lwsg_set_local_pos(sgobj2, -1,-2,0)

print(sg.root.child_count)

c.sg = sg
print('hello')

