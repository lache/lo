#include <stdlib.h>
#include <string.h>
#include "lwanim.h"
#include "file.h"
#include "lwlog.h"

const char* action_filename[] = {
    ASSETS_BASE_PATH "action" PATH_SEPARATOR "TriangleAction.act",
    ASSETS_BASE_PATH "action" PATH_SEPARATOR "TreePlaneAction.act",
    ASSETS_BASE_PATH "action" PATH_SEPARATOR "HumanAction_WalkPolish.act",
    ASSETS_BASE_PATH "action" PATH_SEPARATOR "HumanAction_Idle.act",
    ASSETS_BASE_PATH "action" PATH_SEPARATOR "HumanAction_Attack.act",
    ASSETS_BASE_PATH "action" PATH_SEPARATOR "HumanAction_Stand_Aim.act",
    ASSETS_BASE_PATH "action" PATH_SEPARATOR "HumanAction_Stand_Unaim.act",
    ASSETS_BASE_PATH "action" PATH_SEPARATOR "HumanAction_Stand_Fire.act",
    ASSETS_BASE_PATH "action" PATH_SEPARATOR "HumanAction_Death.act",
    ASSETS_BASE_PATH "action" PATH_SEPARATOR "DetachPlaneAction.act",
    ASSETS_BASE_PATH "action" PATH_SEPARATOR "DetachPlaneAction_ChildTrans.act",
    ASSETS_BASE_PATH "action" PATH_SEPARATOR "Recoil.act",
    ASSETS_BASE_PATH "action" PATH_SEPARATOR "turret-recoil.act",
    ASSETS_BASE_PATH "action" PATH_SEPARATOR "crossbow-fire.act",
    ASSETS_BASE_PATH "action" PATH_SEPARATOR "catapult-fire.act",
    ASSETS_BASE_PATH "action" PATH_SEPARATOR "pyro-idle.act",
};

void load_action(const char* filename, LWANIMACTION* action) {
    size_t s;
    char* p = create_binary_from_file(filename, &s);
    if (p) {
        action->d = p;
        
        action->fps = *(float*)p;
        p += sizeof(float);
        action->last_key_f = *(float*)p;
        p += sizeof(float);
        action->anim_curve_num = *(int*)p;
        p += sizeof(int);
        action->anim_curve = action->anim_curve_num ? (LWANIMCURVE*)p : 0;
        p += sizeof(LWANIMCURVE) * action->anim_curve_num;
        action->anim_key_num = *(int*)p;
        p += sizeof(int);
        action->anim_key = action->anim_key_num ? (LWANIMKEY*)p : 0;
        p += sizeof(LWANIMKEY) * action->anim_key_num;
        action->anim_marker_num = *(int*)p;
        p += sizeof(int);
        action->anim_marker = action->anim_marker_num ? (LWANIMMARKER*)p : 0;
    } else {
        LOGE("load_action: %s FILE NOT FOUND", filename);
        memset(action, 0, sizeof(LWANIMACTION));
    }
}

void unload_action(LWANIMACTION* action) {
    free(action->d);
    memset(action, 0, sizeof(LWANIMACTION));
}

float float_value_with_stride(const float* a, size_t stride, int i) {
    return *(float*)((const char*)a + stride * i);
}

int lower_bound_float(const float* a, int len, size_t stride, float v) {

    if (len <= 0) {
        LOGE("lower_bound_float: len is %d.", len);
        return -1;
    }

    // Lower then the first element
    float beg_value = float_value_with_stride(a, stride, 0);
    if (v < beg_value) {
        return -1;
    }

    // Higher than the last element
    float end_value = float_value_with_stride(a, stride, len - 1);
    if (v >= end_value) {
        return len - 1;
    }

    int beg = 0;
    int end = len - 1;
    while (end - beg > 1) {
        int mid = (beg + end) / 2;
        float mid_value = float_value_with_stride(a, stride, mid);
        if (mid_value < v) {
            beg = mid;
        } else if (v < mid_value) {
            end = mid;
        } else {
            return mid;
        }
    }

    return beg;
}

int get_curve_value(const LWANIMKEY* key, int key_len, float f, float* v) {

    if (key->interpolation != LACT_LINEAR) {
        LOGE("get_curve_value: Only LACT_LINEAR supported. A curve value of 0 always returned.");
        *v = 0;
        return -1;
    }

    if (key_len <= 0) {
        LOGE("get_curve_value: key_len is %d.", key_len);
        *v = 0;
        return -1;
    }

    int idx = lower_bound_float(&key[0].co[0], key_len, sizeof(LWANIMKEY), f);

    if (idx == -1) {
        *v = key[0].co[1];
    } else if (idx == key_len - 1) {
        *v = key[key_len - 1].co[1];
    } else {
        float dx = key[idx + 1].co[0] - key[idx].co[0];
        float dy = key[idx + 1].co[1] - key[idx].co[1];
        float slope = dy / dx;
        float tr = f - key[idx].co[0];

        *v = key[idx].co[1] + slope * tr;
    }

    return 0;
}

float lwanimaction_animtime_to_f(const LWANIMACTION* action, float animtime) {
    return animtime * action->fps;
}
