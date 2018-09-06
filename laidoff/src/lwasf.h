#pragma once

#include "lwasfbone.h"
#include "lwasfmacro.h"

typedef struct _LWASF {
    LWASFBONE bones[MAX_BONES_IN_ASF_FILE];
    LWASFBONE* root_bone;
    int bone_count;
    int moving_bone_count;
} LWASF;

LWASF* lwasf_new_from_file(const char* filename);
void lwasf_delete(LWASF* asf);
void lwasf_enable_all_rotational_dofs(LWASF* asf);
int lwasf_name2idx(const LWASF* asf, const char* name);
