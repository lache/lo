//
// Created by gb on 2024/02/10.
//

#ifndef CLIENT_LAYOUT_PUCKGAME_H
#define CLIENT_LAYOUT_PUCKGAME_H

typedef struct _LWCONTEXT LWCONTEXT;
typedef struct YGNode* YGNodeRef;

YGNodeRef create_layout_puckgame(const LWCONTEXT* pLwc);
void calculate_layout(YGNodeRef node, float window_width, float window_height);
void destroy_layout_puckgame(YGNodeRef node);

#endif //CLIENT_LAYOUT_PUCKGAME_H
