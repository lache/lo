local inspect = require('inspect')
local linmath = reload_require('linmath')
local c = lo.script_context()
lo.htmlui_set_online(c.htmlui, 0)

function on_mocap_enter()
    -- calling 'script_cleanup_all_coros()' here
    -- prevents all coroutines immediately aborted
    -- even if they are started from this script......
    -- disable for now

    --lo.script_cleanup_all_coros(c)
end

--[[
local asf = lo.lwasf_new_from_file(lo.script_full_asset_path('asf', '07-walk.asf'))
local amc = lo.lwamc_new_from_file(lo.script_full_asset_path('amc', '07_05-walk.amc'), asf)
]]--
local asf = lo.lwasf_new_from_file(lo.script_full_asset_path('asf', '135-martialArts.asf'))
local amc = lo.lwamc_new_from_file(lo.script_full_asset_path('amc', '135_06-martialArts.amc'), asf)

local sg = lo.lwsg_new()
local sgobj = lo.lwsg_new_object(sg, 'test obj a', sg.root)
sgobj.lvt = lo.LVT_PUCK_PLAYER
sgobj.lae = lo.LAE_PUCK_PLAYER_KTX
sgobj.active = 0
lo.lwsg_set_local_pos(sgobj,0,0,-5)
lo.lwsg_set_local_euler(sgobj,3.14/4,0,0.1)
start_coro(function()
    local idx = 1
    while true do
        idx = idx + 1
        lo.lwsg_set_local_euler(sgobj,3.14/4,0,idx * 0.1)
        yield_wait_ms(100)
    end
end)

lo.lwsg_set_local_scale(sgobj,1,1,1)

local sgobj2 = lo.lwsg_new_object(sg, 'test obj a-a', sgobj)
sgobj2.lvt = lo.LVT_SUZANNE
lo.lwsg_set_local_pos(sgobj2,2,0,0)
lo.lwsg_set_local_euler(sgobj2,0,0,0)

local sgobj22 = lo.lwsg_new_object(sg, 'test obj a-b', sgobj)
sgobj22.lvt = lo.LVT_SUZANNE
lo.lwsg_set_local_pos(sgobj22,0,2,0)
lo.lwsg_set_local_euler(sgobj22,0,0,0)

local sgobj3 = lo.lwsg_new_object(sg, 'test obj a-a-a', sgobj2)
sgobj3.lvt = lo.LVT_SUZANNE
lo.lwsg_set_local_pos(sgobj3,-2,-2,0)
lo.lwsg_set_local_euler(sgobj3,0,0,0)
lo.lwsg_set_local_scale(sgobj3,1,1,1)
start_coro(function()
    local idx = 1
    while true do
        idx = idx + 1
        lo.lwsg_set_local_euler(sgobj3,0,0,0.1*idx)
        yield_wait_ms(100)
    end
end)

local sgobj4 = lo.lwsg_new_object(sg, 'test obj b', sg.root)
--sgobj4.active = 0
sgobj4.lvt = lo.LVT_EARTH
sgobj4.lae = lo.LAE_WORLD_MAP
lo.lwsg_set_local_pos(sgobj4,-2,2,0)


--print('ASF bone count: '..asf.bone_count)
--lo.show_sys_msg(c.def_sys_msg, 'ASF bone count: '..asf.bone_count)

local bone_sgbone_map = {}
local bone_sgbonecontrol_map = {}
local bone_sgbonechildslot_map = {}

local function get_rot_parent_current(bone)
    local a = linmath.mat4x4_new_identity()
    for ri=1,4 do
        for ci=1,4 do
            a[ri][ci] = lo.lwasfbone_rot_parent_current(bone, ri-1, ci-1)
        end
    end
    return a
end
--[[
bone lhipjoint -- rot_parent_current
    bonecontrol lhipjoint -- AMC
        bonemesh lhipjoint -- bone_dir rotation
        bonejoint lhipjoint
        bonechild lhipjoint -- bone_dir * length translation
]]--
local function create_bone(bone, depth, bone_parent)
    -- bone local direction
    local bone_dir = { lo.lwasfbone_dir(bone, 0),
                       lo.lwasfbone_dir(bone, 1),
                       lo.lwasfbone_dir(bone, 2) }
    local indent = string.rep('  ', depth)
    --[[
    print(string.format('[%2d]%s%s (dof=%d) (length=%f) [%f,%f,%f]',
                        bone.idx,
                        indent,
                        bone.name,
                        bone.dof,
                        bone.length,
                        bone_dir[1],
                        bone_dir[2],
                        bone_dir[3]))]]--
    --local rot_parent_current = get_rot_parent_current(bone)
    local rot_parent_current = get_rot_parent_current(bone)
    --print('    '..indent..'rot_parent_current='..inspect(rot_parent_current))
    local sgbone_parent = sg.root
    if bone_parent ~= nil then sgbone_parent = bone_sgbonechildslot_map[bone_parent] end
    local sgbone = lo.lwsg_new_object(sg, 'bone '..bone.name, sgbone_parent)
    local sgbonecontrol = lo.lwsg_new_object(sg, 'bonecontrol '..bone.name, sgbone)
    local sgbonechildslot = lo.lwsg_new_object(sg, 'bonechildslot '..bone.name, sgbonecontrol)
    -- translation from bone to its child (in local coordinate system of bone)
    local trans = { bone.length*bone_dir[1],
                    bone.length*bone_dir[2],
                    bone.length*bone_dir[3] }
    lo.lwsg_set_local_pos(sgbonechildslot,
                          trans[1],
                          trans[2],
                          trans[3])
    
    lo.lwsg_set_local_pos(sgbonecontrol, bone.tx, bone.ty, bone.tz)
    lo.lwsg_set_local_euler(sgbonecontrol, bone.rx, bone.ry, bone.rz)

    lo.lwsg_set_local_rot(sgbone,
                          rot_parent_current[1][1], rot_parent_current[1][2], rot_parent_current[1][3], rot_parent_current[1][4],
                          rot_parent_current[2][1], rot_parent_current[2][2], rot_parent_current[2][3], rot_parent_current[2][4],
                          rot_parent_current[3][1], rot_parent_current[3][2], rot_parent_current[3][3], rot_parent_current[3][4],
                          rot_parent_current[4][1], rot_parent_current[4][2], rot_parent_current[4][3], rot_parent_current[4][4])
    
    local sgbonemesh
    if bone.idx ~= 0 then
        sgbonemesh = lo.lwsg_new_object(sg, 'bonemesh '..bone.name, sgbonecontrol)
        sgbonemesh.lvt = lo.LVT_ZBONE
        local bone_thickness = 0.05
        lo.lwsg_set_local_scale(sgbonemesh,bone_thickness,bone_thickness,bone.length)
            
        local sgjointmesh = lo.lwsg_new_object(sg, 'jointmesh '..bone.name, sgbonecontrol)
        sgjointmesh.lvt = lo.LVT_SUZANNE
        local joint_scale = 0.025
        lo.lwsg_set_local_scale(sgjointmesh,joint_scale,joint_scale,joint_scale)
    end
    
    bone_sgbone_map[bone] = sgbone
    bone_sgbonecontrol_map[bone] = sgbonecontrol
    bone_sgbonechildslot_map[bone] = sgbonechildslot

    if bone.idx ~= 0 then
        local zvec = {0,0,1}
        local angle, axis = linmath.vec3_angle_axis(zvec, bone_dir)
        local bone_rot = linmath.mat4x4_rotate_around(angle, axis[1], axis[2], axis[3])
        --print('    '..indent..'bone_rot='..inspect(bone_rot))
        --print('    '..indent..'angle='..angle)
        --print('    '..indent..'axis='..inspect(axis))
        --print('    '..indent..'trans='..inspect(trans))
        local r = linmath.mat4x4_transposed(bone_rot)
        lo.lwsg_set_local_rot(sgbonemesh,
                              r[1][1], r[1][2], r[1][3], r[1][4],
                              r[2][1], r[2][2], r[2][3], r[2][4],
                              r[3][1], r[3][2], r[3][3], r[3][4],
                              r[4][1], r[4][2], r[4][3], r[4][4])
    end
end

local function create_bone_hierarchy(bone, depth, bone_parent)
    --if depth > 3 then return end
    if bone == nil then return end
    create_bone(bone, depth, bone_parent)
    create_bone_hierarchy(bone.child, depth + 1, bone)
    create_bone_hierarchy(bone.sibling, depth, bone_parent)
end

create_bone_hierarchy(asf.root_bone, 0, nil)

lo.lwsg_set_local_rot(sg.root,
                      1, 0, 0, 0,
                      0, 0, 1, 0,
                      0,-1, 0, 0,
                      0, 0, 0, 1)

start_coro(function()
    for i=0,amc.frame_count-1 do
        lo.lwamc_apply_posture(amc, asf, i)
        for bone, sgbonecontrol in pairs(bone_sgbonecontrol_map) do
            lo.lwsg_set_local_pos(sgbonecontrol, bone.tx, bone.ty, bone.tz)
            lo.lwsg_set_local_euler(sgbonecontrol, bone.rx, bone.ry, bone.rz)
        end
        yield_wait_ms(8)
    end
end)



--print(sg.root.child_count)
lo.lwsg_cam_eye(sg, 10, -10, 10)
sg.half_height = 5
lo.lwcontext_set_sg(c, sg)
--print('hello')

local a = linmath.mat4x4_new_identity()
local b = linmath.mat4x4_new_identity()
local c = linmath.mat4x4_multiply(a,b)
local rx90 = linmath.mat4x4_rotate_x(math.rad(90))
local ry90 = linmath.mat4x4_rotate_y(math.rad(90))
local rz90 = linmath.mat4x4_rotate_z(math.rad(90))
print(inspect(c))
local v = linmath.vec4_new_zero(); v[1] = 1 -- v = (1,0,0)
print(inspect(linmath.mat4x4_multiply_vec4(rz90, v))) -- (0,1,0)
print(inspect(linmath.mat4x4_multiply_vec4(ry90, v))) -- (0,0,-1)
print(inspect(linmath.mat4x4_multiply_vec4(rx90, v))) -- (1,0,0)
