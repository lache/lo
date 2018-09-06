#pragma once
#include "linmath.h"

typedef struct _LWCONTEXT LWCONTEXT;

typedef struct _LWSGOBJECT LWSGOBJECT;
typedef struct _LWSGOBJECT {
    char name[256];
    vec3 pos;
    vec4 quat;
    vec3 scale;
    LWSGOBJECT* child;
    LWSGOBJECT* sibling;
    LWSGOBJECT* parent;
    int child_count;
    int lvt;
} LWSGOBJECT;

typedef struct _LWSG {
    LWSGOBJECT* root;
} LWSG;

LWSG* lwsg_new();
void lwsg_delete(LWSG* sg);
LWSGOBJECT* lwsg_new_object(LWSG* sg, const char* objname, LWSGOBJECT* parent);
LWSGOBJECT* lwsg_find(const char* objname);
void lwsg_set_local_pos(LWSGOBJECT* obj, float x, float y, float z);
void lwsg_set_local_quat(LWSGOBJECT* obj, float w, float x, float y, float z);
void lwsg_set_local_scale(LWSGOBJECT* obj, float sx, float sy, float sz);
void lwsg_delete_object(LWSG* sg, LWSGOBJECT* sgobj);
void lwsg_refresh(LWCONTEXT* pLwc, LWSG* sg);
