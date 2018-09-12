#include "lwsg.h"
#include "lwcontext.h"
#include "lwlog.h"
#include <stdlib.h> // ios free()
#include <string.h> // ios strcpy()
LWSG* lwsg_new() {
    LWSG* sg = (LWSG*)calloc(1, sizeof(LWSG));
    sg->root = (LWSGOBJECT*)calloc(1, sizeof(LWSGOBJECT));
    sg->root->scale[0] = sg->root->scale[1] = sg->root->scale[2] = 1;
    lwsg_set_local_euler(sg->root, 0, 0, 0);
    sg->root->active = 1;
    sg->cam_eye[0] = 0;
    sg->cam_eye[1] = 0;
    sg->cam_eye[2] = 10;
    sg->half_height = 5.0f;
    return sg;
}

static void delete_recursive(LWSGOBJECT* sgobj) {
    if (sgobj == 0) {
        return;
    }
    delete_recursive(sgobj->child);
    delete_recursive(sgobj->sibling);
    LOGI("Releasing scene graph object '%s'...", sgobj->name);
    free(sgobj);
}

void lwsg_delete(LWSG* sg) {
    delete_recursive(sg->root);
    free(sg);
}

LWSGOBJECT* lwsg_new_object(LWSG* sg, const char* objname, LWSGOBJECT* parent) {
    if (parent == 0) {
        LOGEP("parent should be not null");
        return 0;
    }
    LWSGOBJECT* sgobj = (LWSGOBJECT*)calloc(1, sizeof(LWSGOBJECT));
    strncpy(sgobj->name, objname, sizeof(sgobj->name));
    sgobj->parent = parent;
    sgobj->scale[0] = sgobj->scale[1] = sgobj->scale[2] = 1;
    lwsg_set_local_euler(sgobj, 0, 0, 0);
    sgobj->active = 1;
    if (parent->child) {
        LWSGOBJECT* sibling = parent->child;
        while (sibling->sibling) {
            sibling = sibling->sibling;
        }
        sibling->sibling = sgobj;
    } else {
        parent->child = sgobj;
    }
    parent->child_count++;
    return sgobj;
}

LWSGOBJECT* lwsg_find(const char* objname) {
    return 0;
}

void lwsg_set_local_pos(LWSGOBJECT* obj, float x, float y, float z) {
    obj->pos[0] = x;
    obj->pos[1] = y;
    obj->pos[2] = z;
}

void lwsg_set_local_euler(LWSGOBJECT* obj, float rx, float ry, float rz) {
    mat4x4_identity(obj->rot);
    mat4x4_rotate_Z(obj->rot, obj->rot, rz);
    mat4x4_rotate_Y(obj->rot, obj->rot, ry);
    mat4x4_rotate_X(obj->rot, obj->rot, rx);
}

void lwsg_set_local_rot(LWSGOBJECT* obj,
                        float rc00, float rc01, float rc02, float rc03,
                        float rc10, float rc11, float rc12, float rc13,
                        float rc20, float rc21, float rc22, float rc23,
                        float rc30, float rc31, float rc32, float rc33) {
    obj->rot[0][0] = rc00; obj->rot[0][1] = rc01; obj->rot[0][2] = rc02; obj->rot[0][3] = rc03;
    obj->rot[1][0] = rc10; obj->rot[1][1] = rc11; obj->rot[1][2] = rc12; obj->rot[1][3] = rc13;
    obj->rot[2][0] = rc20; obj->rot[2][1] = rc21; obj->rot[2][2] = rc22; obj->rot[2][3] = rc23;
    obj->rot[3][0] = rc30; obj->rot[3][1] = rc31; obj->rot[3][2] = rc32; obj->rot[3][3] = rc33;
}

void lwsg_set_local_scale(LWSGOBJECT* obj, float sx, float sy, float sz) {
    obj->scale[0] = sx;
    obj->scale[1] = sy;
    obj->scale[2] = sz;
}

void lwsg_delete_object(LWSG* sg, LWSGOBJECT* sgobj) {
}

void lwsg_refresh(LWCONTEXT* pLwc, LWSG* sg) {
}

void lwsg_cam_eye(LWSG* sg, float x, float y, float z) {
    sg->cam_eye[0] = x;
    sg->cam_eye[1] = y;
    sg->cam_eye[2] = z;
}

void lwsg_cam_look_at(LWSG* sg, float x, float y, float z) {
    sg->cam_look_at[0] = x;
    sg->cam_look_at[1] = y;
    sg->cam_look_at[2] = z;
}
