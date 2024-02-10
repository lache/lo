//
// Created by gb on 2024/02/11.
//

#include "node_render.h"
#include <yoga/Yoga.h>
#include "lwcontext.h"

static float convert_screen_coords_to_normalized_x(const LWCONTEXT *pLwc, float x) {
    return ((x / pLwc->window_width) - 0.5f) * 2 * pLwc->viewport_rt_x;
}

static float convert_screen_coords_to_normalized_y(const LWCONTEXT *pLwc, float y) {
    return ((y / pLwc->window_height) - 0.5f) * (-2) * pLwc->viewport_rt_y;
}

static float convert_screen_size_to_normalized_x(const LWCONTEXT *pLwc, float x) {
    return (x / pLwc->window_width) * 2 * pLwc->viewport_rt_x;
}

static float convert_screen_size_to_normalized_y(const LWCONTEXT *pLwc, float y) {
    return (y / pLwc->window_height) * 2 * pLwc->viewport_rt_y;
}

LWNODERENDER convert_node_to_render(const LWCONTEXT *pLwc, YGNodeConstRef node) {
    float l = YGNodeLayoutGetLeft(node);
    float t = YGNodeLayoutGetTop(node);
    const float w = YGNodeLayoutGetWidth(node);
    const float h = YGNodeLayoutGetHeight(node);

    YGNodeConstRef parent = YGNodeGetParent(node);
    while (parent != NULL) {
        l += YGNodeLayoutGetLeft(parent);
        t += YGNodeLayoutGetTop(parent);
        parent = YGNodeGetParent(parent);
    }

    return (LWNODERENDER) {
            .x =convert_screen_coords_to_normalized_x(pLwc, l),
            .y =convert_screen_coords_to_normalized_y(pLwc, t),
            .w =convert_screen_size_to_normalized_x(pLwc, w),
            .h =convert_screen_size_to_normalized_y(pLwc, h),
    };
}
