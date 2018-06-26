#pragma once

#include "linmath.h"

#define TTL_MAX_SELECTABLE_COUNT (32)
#define TTL_SELECTABLE_TYPE_VEHICLE (1)

typedef struct _LWTTLRENDERSELECTABLE {
    int id;
    int type;
    vec2 ui_bound_min;
    vec2 ui_bound_max;
} LWTTLRENDERSELECTABLE;

typedef struct _LWTTLRENDERCONTEXT {
    int selectable_count;
    LWTTLRENDERSELECTABLE selectable[TTL_MAX_SELECTABLE_COUNT];
} LWTTLRENDERCONTEXT;
