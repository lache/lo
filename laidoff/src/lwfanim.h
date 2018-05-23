#pragma once

typedef struct _LWCONTEXT LWCONTEXT;

typedef enum _LW_F_ANIM_TYPE {
    LFAT_TOWER_COLLAPSE,
    LFAT_TOWER_COLLAPSE_OCTAGON,
    LFAT_COUNT,
} LW_F_ANIM_TYPE;

typedef struct _LWFANIMKEY {
    float x, y, z, qw, qx, qy, qz;
} LWFANIMKEY;

// Cell fracture animation (sequence of x, y, z, qw, qx, qy, qz for multiple cell fracture objects)
typedef struct _LWFANIM
{
    int total_cell_count;
    int total_frame_count;
    LWFANIMKEY* anim_key;
    void* data; // should be released
} LWFANIM;

void load_fanim(LWCONTEXT* pLwc, const char* filename, LWFANIM* fanim);
