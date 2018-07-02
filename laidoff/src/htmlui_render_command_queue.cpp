#include "htmlui_render_command_queue.h"
#include "lwsolid.h"
#include "lwtextblock.h"

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
