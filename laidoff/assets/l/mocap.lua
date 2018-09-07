local c = lo.script_context()
lo.htmlui_set_online(c.htmlui, 0)

function on_mocap_enter()
    lo.script_cleanup_all_coros(c)
end

local asf = lo.lwasf_new_from_file('assets/asf/07-walk.asf')
local amc = lo.lwamc_new_from_file('assets/amc/07_05-walk.amc', asf)

local sg = lo.lwsg_new()
local sgobj = lo.lwsg_new_object(sg, 'test obj a', sg.root)
sgobj.lvt = lo.LVT_PUCK_PLAYER
sgobj.lae = lo.LAE_PUCK_PLAYER_KTX
lo.lwsg_set_local_pos(sgobj,0,0,0)
lo.lwsg_set_local_euler(sgobj,0,-0.5,0)
lo.lwsg_set_local_scale(sgobj,1,1,1)

local sgobj2 = lo.lwsg_new_object(sg, 'test obj a-a', sgobj)
sgobj2.lvt = lo.LVT_SUZANNE
lo.lwsg_set_local_pos(sgobj2,2,0,0)
lo.lwsg_set_local_euler(sgobj2,0,0,0)

--[[local sgobj22 = lo.lwsg_new_object(sg, 'test obj a-b', sgobj)
sgobj22.lvt = lo.LVT_SUZANNE
lo.lwsg_set_local_pos(sgobj22,0,2,0)
lo.lwsg_set_local_euler(sgobj22,0,0,0)]]--

local sgobj3 = lo.lwsg_new_object(sg, 'test obj a-a-a', sgobj2)
sgobj3.lvt = lo.LVT_SUZANNE
lo.lwsg_set_local_pos(sgobj3,-2,-2,0)
lo.lwsg_set_local_euler(sgobj3,0,0,0)
lo.lwsg_set_local_scale(sgobj3,1,1,1)

local sgobj4 = lo.lwsg_new_object(sg, 'test obj b', sg.root)
sgobj4.lvt = lo.LVT_EARTH
sgobj4.lae = lo.LAE_WORLD_MAP
lo.lwsg_set_local_pos(sgobj4,-2,2,0)

print(sg.root.child_count)

lo.lwsg_cam_eye(sg, 0, 0, 10)

lo.lwcontext_set_sg(c, sg)

print('hello')

