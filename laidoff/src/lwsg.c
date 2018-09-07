#include "lwsg.h"
#include "lwcontext.h"
#include "lwlog.h"

LWSG* lwsg_new() {
    LWSG* sg = (LWSG*)calloc(1, sizeof(LWSG));
    sg->root = (LWSGOBJECT*)calloc(1, sizeof(LWSGOBJECT));
    sg->cam_eye[0] = 0;
    sg->cam_eye[1] = 0;
    sg->cam_eye[2] = 10;
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
        return 0;
    }
    LWSGOBJECT* sgobj = (LWSGOBJECT*)calloc(1, sizeof(LWSGOBJECT));
    strncpy(sgobj->name, objname, sizeof(sgobj->name));
    sgobj->parent = parent;
    sgobj->scale[0] = sgobj->scale[1] = sgobj->scale[2] = 1;
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
    obj->rot[0] = rx;
    obj->rot[1] = ry;
    obj->rot[2] = rz;
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
