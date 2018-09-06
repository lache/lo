#include <string.h> // mac os strlen
#include "lwcontext.h"
#include "lwsg.h"
#include "render_solid.h"

const static float default_uv_offset[2] = { 0, 0 };
const static float default_uv_scale[2] = { 1, 1 };

static void update_view_proj(const vec3 cam_eye,
                             const vec3 cam_look_at,
                             const int viewport_width,
                             const int viewport_height,
                             mat4x4 view,
                             mat4x4 proj) {
    const float aspect_ratio = (float)viewport_width / viewport_height;
    // half_height := (how many cells in vertical axis) / 2
    // (relative to main viewport height)
    float half_height = 5.0f * (float)viewport_height / viewport_height;
    float near_z = 0.1f;
    float far_z = 1000.0f;
    // eye(camera) position
    vec3 eye = {
        cam_eye[0],
        cam_eye[1],
        cam_eye[2]
    };
    // look position
    vec3 center = {
        cam_look_at[0],
        cam_look_at[1],
        cam_look_at[2]
    };
    vec3 center_to_eye;
    vec3_sub(center_to_eye, eye, center);
    float cam_a = atan2f(center_to_eye[1], center_to_eye[0]);
    // right := rotate (1, 0, 0) by cam_a in +Z axis
    vec3 right = { 0, 0, 0 };
    if (center_to_eye[0] == 0 && center_to_eye[1] == 0) {
        right[0] = 1;
    } else {
        right[0] = cosf((float)(M_PI / 2) + cam_a);
        right[1] = sinf((float)(M_PI / 2) + cam_a);
    }
    vec3 eye_right;
    vec3_mul_cross(eye_right, center_to_eye, right);
    vec3 up;
    vec3_norm(up, eye_right);
    mat4x4_ortho(proj,
                 -half_height * aspect_ratio,
                 +half_height * aspect_ratio,
                 -half_height,
                 +half_height,
                 near_z,
                 far_z);
    mat4x4_look_at(view, eye, center, up);
}

static void render_sgobject(const LWCONTEXT* pLwc, const LWSGOBJECT* sgobj) {
    if (sgobj == 0 || sgobj->lvt == 0) {
        return;
    }
    vec3 cam_eye = { 0, 0, 10 };
    vec3 cam_look_at = { 0, 0, 0 };
    mat4x4 view, proj;
    update_view_proj(cam_eye, cam_look_at, pLwc->viewport_width, pLwc->viewport_height, view, proj);
    render_solid_general(pLwc,
                         sgobj->pos[0],
                         sgobj->pos[1],
                         sgobj->pos[2],
                         sgobj->scale[0],
                         sgobj->scale[1],
                         sgobj->scale[2],
                         pLwc->tex_atlas[LAE_PUCK_PLAYER_KTX],
                         0,
                         sgobj->lvt,
                         1.0f,
                         1.0f,
                         1.0f,
                         1.0f,
                         0.0f,
                         default_uv_offset,
                         default_uv_scale,
                         LWST_DEFAULT,
                         0,
                         view,
                         proj);
}

static void render_sgobject_recursive(const LWCONTEXT* pLwc, const LWSGOBJECT* sgobj) {
    if (pLwc == 0 || sgobj == 0) {
        return;
    }
    render_sgobject(pLwc, sgobj);
    render_sgobject_recursive(pLwc, sgobj->child);
    render_sgobject_recursive(pLwc, sgobj->sibling);
}

void lwc_render_sg(const LWCONTEXT* pLwc, const LWSG* sg) {
    if (sg == 0) {
        return;
    }
    render_sgobject_recursive(pLwc, sg->root);
}
