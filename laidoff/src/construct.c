#include "construct.h"
#include "lwmacro.h"
#include "lwlog.h"

void construct_set_preview_enable(LWCONSTRUCT* construct, int enable) {
    construct->preview_enable = enable;
}

void construct_set_preview(LWCONSTRUCT* construct, LW_ATLAS_ENUM atlas, LW_SKIN_VBO_TYPE skin_vbo, LW_ARMATURE armature, LW_ACTION anim_action_id) {
    construct->preview.atlas = atlas;
    construct->preview.skin_vbo = skin_vbo;
    construct->preview.armature = armature;
    construct->preview.anim_action_id = anim_action_id;
}
