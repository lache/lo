#pragma once
#include "linmath.h"

typedef struct _LWCONTEXT LWCONTEXT;

typedef struct _LWSGOBJECT LWSGOBJECT;
typedef struct _LWSGOBJECT {
    char name[256];
    vec3 pos; // local position
    mat4x4 rot; // local rotation
    vec3 scale; // local scale
    LWSGOBJECT* child;
    LWSGOBJECT* sibling;
    LWSGOBJECT* parent;
    int child_count;
    int lvt;
    int lae;
    int active;
} LWSGOBJECT;

typedef struct _LWSG {
    LWSGOBJECT* root;
    vec3 cam_eye;
    vec3 cam_look_at;
    float half_height;
} LWSG;

LWSG* lwsg_new();
void lwsg_delete(LWSG* sg);
LWSGOBJECT* lwsg_new_object(LWSG* sg, const char* objname, LWSGOBJECT* parent);
LWSGOBJECT* lwsg_find(const char* objname);
void lwsg_set_local_pos(LWSGOBJECT* obj, float x, float y, float z);
void lwsg_set_local_euler(LWSGOBJECT* obj, float rx, float ry, float rz);
void lwsg_set_local_rot(LWSGOBJECT* obj,
                        float rc00, float rc01, float rc02, float rc03,
                        float rc10, float rc11, float rc12, float rc13,
                        float rc20, float rc21, float rc22, float rc23,
                        float rc30, float rc31, float rc32, float rc33);
void lwsg_set_local_scale(LWSGOBJECT* obj, float sx, float sy, float sz);
void lwsg_delete_object(LWSG* sg, LWSGOBJECT* sgobj);
void lwsg_refresh(LWCONTEXT* pLwc, LWSG* sg);
void lwsg_cam_eye(LWSG* sg, float x, float y, float z);
void lwsg_cam_look_at(LWSG* sg, float x, float y, float z);
