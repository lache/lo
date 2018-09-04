#pragma once

#include "lwasfbone.h"

#define MAX_BONES_IN_ASF_FILE 256

typedef struct _LWASF {
    LWASFBONE bones[MAX_BONES_IN_ASF_FILE];
    LWASFBONE* root_bone;
    int bone_count;
    int moving_bone_count;
} LWASF;

LWASF* lwasf_new_from_file(const char* filename);
