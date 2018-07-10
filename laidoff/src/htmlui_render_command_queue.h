#pragma once
#include <vector>
#include <string>

typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWSOLID LWSOLID;
typedef struct _LWTEXTBLOCK LWTEXTBLOCK;

class htmlui_render_command_queue {
public:
    void add_solid(const LWSOLID* solid);
    void add_text_block(const LWTEXTBLOCK* text_block);
    void clear();
    void render(const LWCONTEXT* pLwc, float scroll_y);
private:
    enum LW_RENDER_COMMAND_TYPE {
        LRC_RENDER_SOLID,
        LRC_TEXT_BLOCK,
    };
    std::vector<LW_RENDER_COMMAND_TYPE> render_commands;
    std::vector<LWSOLID> solids;
    std::vector<LWTEXTBLOCK> text_blocks;
    std::vector<std::string> text_block_strings;
};
