print('linmath.lua visible ^__^;')
local inspect = require('inspect')

-- create a new matrix
local function mat4x4_new_identity()
    return {{1,0,0,0},
            {0,1,0,0},
            {0,0,1,0},
            {0,0,0,1}}
end

-- get row from matrix (reference)
local function mat4x4_row_as_vec4(a, ri)
    return a[ri]
end

-- get column from matrix (deep copy)
local function mat4x4_col_as_vec4(a, ci)
    return {a[1][ci],a[2][ci],a[3][ci],a[4][ci]}
end

-- compute v * w (dot product)
local function vec4_dot_product(v, w)
    return v[1]*w[1]+v[2]*w[2]+v[3]*w[3]+v[4]*w[4]
end

-- compute v * w (dot product)
local function vec3_dot_product(v, w)
    return v[1]*w[1]+v[2]*w[2]+v[3]*w[3]
end

-- compute a * b (matrix-matrix)
local function mat4x4_multiply(a, b)
    local c = mat4x4_new_identity()
    for ri=1,4 do
        local row = mat4x4_row_as_vec4(a, ri)
        for ci=1,4 do
            local col = mat4x4_col_as_vec4(b, ci)
            c[ri][ci] = vec4_dot_product(row, col)
        end
    end
    return c
end

-- get zero point vector
local function vec4_new_zero()
    return {0,0,0,1}
end

-- compute a * v (matrix-vector)
local function mat4x4_multiply_vec4(a, v)
    local av = vec4_new_zero()
    for ri=1,4 do
        local row = mat4x4_row_as_vec4(a, ri)
        av[ri] = vec4_dot_product(row, v)
    end
    return av
end

-- compute rotation matrix around X axis
local function mat4x4_rotate_x(r)
    local c = math.cos(r)
    local s = math.sin(r)
    return {{ 1, 0, 0, 0},
            { 0, c,-s, 0},
            { 0, s, c, 0},
            { 0, 0, 0, 1}}
end

-- compute rotation matrix around y axis
local function mat4x4_rotate_y(r)
    local c = math.cos(r)
    local s = math.sin(r)
    return {{ c, 0, s, 0},
            { 0, 1, 0, 0},
            {-s, 0, c, 0},
            { 0, 0, 0, 1}}
end

-- compute rotation matrix around Z axis
local function mat4x4_rotate_z(r)
    local c = math.cos(r)
    local s = math.sin(r)
    return {{ c,-s, 0, 0},
            { s, c, 0, 0},
            { 0, 0, 1, 0},
            { 0, 0, 0, 1}}
end

-- compute v X w (cross product)
local function vec3_cross(v, w)
    return { v[2]*w[3]-v[3]*w[2], v[3]*w[1]-v[1]*w[3], v[1]*w[2]-v[2]*w[1] }
end

local function vec3_magnitude(v)
    return math.sqrt(v[1]*v[1]+v[2]*v[2]+v[3]*v[3])
end

local function vec3_normalized(v)
    local vmag = vec3_magnitude(v)
    return {v[1]/vmag,v[2]/vmag,v[3]/vmag}
end

-- get the angle and rotation axis between vector v to vector w
local function vec3_angle_axis(v, w)
    local angle = math.acos(vec3_dot_product(v, w))
    local axis = vec3_normalized(vec3_cross(v, w))
    return angle, axis
end

local function mat4x4_rotate_around(r, x, y, z)
    local c = math.cos(r)
    local s = math.sin(r)
    return {{x*x*(1-c)+c  , x*y*(1-c)-z*s, x*z*(1-c)+y*s, 0},
            {y*x*(1-c)+z*s, y*y*(1-c)+c  , y*z*(1-c)-x*s, 0},
            {x*z*(1-c)-y*s, y*z*(1-c)+x*s, z*z*(1-c)+c  , 0},
            {0,             0,             0,             1}}
end

local M = {
    mat4x4_new_identity = mat4x4_new_identity,
    mat4x4_row_as_vec4 = mat4x4_row_as_vec4,
    mat4x4_col_as_vec4 = mat4x4_col_as_vec4,
    vec4_dot_product = vec4_dot_product,
    mat4x4_multiply = mat4x4_multiply,
    vec4_new_zero = vec4_new_zero,
    mat4x4_multiply_vec4 = mat4x4_multiply_vec4,
    mat4x4_rotate_x = mat4x4_rotate_x,
    mat4x4_rotate_y = mat4x4_rotate_y,
    mat4x4_rotate_z = mat4x4_rotate_z,
    vec3_cross = vec3_cross,
    vec3_angle_axis = vec3_angle_axis,
    mat4x4_rotate_around = mat4x4_rotate_around,
}
M.__index = M
return M
