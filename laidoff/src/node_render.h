//
// Created by gb on 2024/02/11.
//

#ifndef CLIENT_NODE_RENDER_H
#define CLIENT_NODE_RENDER_H

typedef struct _LWCONTEXT LWCONTEXT;
typedef const struct YGNode* YGNodeConstRef;

typedef struct _LWNODERENDER {
    float x, y, w, h;
} LWNODERENDER;

LWNODERENDER convert_node_to_render(const LWCONTEXT *pLwc, YGNodeConstRef node);

#endif //CLIENT_NODE_RENDER_H
