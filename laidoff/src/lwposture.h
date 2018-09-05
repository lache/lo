#pragma once
#include "lwasfmacro.h"

typedef struct _LWPOSTURE {
    double root_pos[3];
    double bone_rotation[MAX_BONES_IN_ASF_FILE][3];
    double bone_translation[MAX_BONES_IN_ASF_FILE][3];
    double bone_length[MAX_BONES_IN_ASF_FILE];
} LWPOSTURE;
