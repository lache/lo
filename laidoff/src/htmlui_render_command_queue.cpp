#include "htmlui_render_command_queue.h"
#include "lwsolid.h"
#include "lwtextblock.h"
#include "render_solid.h"
#include "render_text_block.h"

void htmlui_render_command_queue::add_solid(const LWSOLID* solid) {
    render_commands.push_back(LRC_RENDER_SOLID);
    solids.push_back(*solid);
}

void htmlui_render_command_queue::add_text_block(const LWTEXTBLOCK* text_block) {
    render_commands.push_back(LRC_TEXT_BLOCK);
    text_blocks.push_back(*text_block);
    auto rbeg = text_blocks.rbegin();
    text_block_strings.push_back(rbeg->text);
    rbeg->text = text_block_strings.rbegin()->c_str();
}

void htmlui_render_command_queue::clear() {
    render_commands.clear();
    solids.clear();
    text_blocks.clear();
    text_block_strings.clear();
}

void htmlui_render_command_queue::render(const LWCONTEXT* pLwc) {
    int solid_index = 0;
    int text_block_index = 0;
    for (int i = 0; i < render_commands.size(); i++) {
        if (render_commands[i] == LRC_RENDER_SOLID) {
            const LWSOLID* solid = &solids[solid_index];
            render_solid_general(pLwc,
                                 solid->x,
                                 solid->y,
                                 solid->z,
                                 solid->sx,
                                 solid->sy,
                                 solid->sz,
                                 solid->tex_index,
                                 solid->tex_alpha_index,
                                 solid->lvt,
                                 solid->alpha_multiplier,
                                 solid->over_r,
                                 solid->over_g,
                                 solid->over_b,
                                 solid->oratio,
                                 solid->uv_offset,
                                 solid->uv_scale,
                                 solid->shader_index,
                                 solid->rot_z,
                                 solid->view,
                                 solid->proj);
            solid_index++;
        } else if (render_commands[i] == LRC_TEXT_BLOCK) {
            LWTEXTBLOCK* text_block = &text_blocks[text_block_index];
            text_block->text = text_block_strings[text_block_index].c_str();
            render_text_block_two_pass_color(pLwc, text_block);
            text_block_index++;
        }
    }
}
