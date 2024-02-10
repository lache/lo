//
// Created by gb on 2024/02/10.
//

#include <yoga/Yoga.h>
#include "lwcontext.h"
#include "layout_puckgame.h"
#include "lwlog.h"

static YGNodeRef create_leaderboard_row() {
    YGNodeRef root = YGNodeNew();
    YGNodeStyleSetFlexDirection(root, YGFlexDirectionRow);
    YGNodeRef children[] = {YGNodeNew(), YGNodeNew(), YGNodeNew()};
    YGNodeSetChildren(root, children, ARRAY_SIZE(children));

    YGNodeStyleSetWidthPercent(children[0], 15);
    YGNodeStyleSetFlexGrow(children[1], 1);
    YGNodeStyleSetWidthPercent(children[2], 15);

    return root;
}

YGNodeRef create_layout_puckgame(const LWCONTEXT *pLwc) {
    if (pLwc == NULL) {
        return NULL;
    }

    YGNodeRef root = YGNodeNew();
    YGNodeStyleSetPadding(root, YGEdgeAll, 10);

    YGNodeStyleSetFlexDirection(root, YGFlexDirectionColumn);

    YGNodeRef leaderboard = YGNodeNew();
    YGNodeRef profile = YGNodeNew();
    YGNodeRef menu = YGNodeNew();

    YGNodeStyleSetFlexGrow(leaderboard, 1);
    YGNodeStyleSetHeight(profile, 100);
    YGNodeStyleSetFlexGrow(menu, 1);

    const float margin = 10;

//    YGNodeStyleSetBorder(profile, YGEdgeAll, margin);
//    YGNodeStyleSetBorder(leaderboard, YGEdgeAll, margin);
//    YGNodeStyleSetBorder(menu, YGEdgeAll, margin);

    YGNodeStyleSetPadding(profile, YGEdgeAll, margin);
    YGNodeStyleSetPadding(leaderboard, YGEdgeAll, margin);
    YGNodeStyleSetPadding(menu, YGEdgeAll, margin);

    YGNodeStyleSetMargin(profile, YGEdgeAll, margin);
    YGNodeStyleSetMargin(leaderboard, YGEdgeAll, margin);
    YGNodeStyleSetMargin(menu, YGEdgeAll, margin);

    YGNodeRef root_children[] = {profile, leaderboard, menu};
    YGNodeSetChildren(root, root_children, ARRAY_SIZE(root_children));

    YGNodeStyleSetFlexDirection(profile, YGFlexDirectionRow);
    //YGNodeStyleSetBorder(profile, YGEdgeAll, 20);
    YGNodeStyleSetAlignItems(profile, YGAlignStretch);
    YGNodeStyleSetAlignContent(profile, YGAlignStretch);

    YGNodeRef profile_icon = YGNodeNew();
    YGNodeRef profile_text = YGNodeNew();
    YGNodeRef profile_children[] = {profile_icon, profile_text};
    YGNodeSetChildren(profile, profile_children, ARRAY_SIZE(profile_children));

    YGNodeStyleSetAspectRatio(profile_icon, 1);

    YGNodeStyleSetMargin(profile_text, YGEdgeAll, 10);

    YGNodeRef leaderboard_children[] = {create_leaderboard_row()};
    YGNodeSetChildren(leaderboard, leaderboard_children, ARRAY_SIZE(leaderboard_children));
    YGNodeStyleSetHeightPercent(leaderboard_children[0], 100/15);

    calculate_layout(root, pLwc->window_width, pLwc->window_height);

    LOGI("Leaderboard: (%f,%f) WH(%f,%f)",
         YGNodeLayoutGetLeft(leaderboard),
         YGNodeLayoutGetTop(leaderboard),
         YGNodeLayoutGetWidth(leaderboard),
         YGNodeLayoutGetHeight(leaderboard)
    );
    LOGI("Profile: (%f,%f) WH(%f,%f)",
         YGNodeLayoutGetLeft(profile),
         YGNodeLayoutGetTop(profile),
         YGNodeLayoutGetWidth(profile),
         YGNodeLayoutGetHeight(profile)
    );
    LOGI("Menu: (%f,%f) WH(%f,%f)",
         YGNodeLayoutGetLeft(menu),
         YGNodeLayoutGetTop(menu),
         YGNodeLayoutGetWidth(menu),
         YGNodeLayoutGetHeight(menu)
    );

    return root;
}

void calculate_layout(YGNodeRef node, float window_width, float window_height) {
    if (node == NULL) {
        return;
    }
    YGNodeCalculateLayout(node, window_width, window_height, YGDirectionLTR);
}

void destroy_layout_puckgame(YGNodeRef node) {
    YGNodeFreeRecursive(node);
}