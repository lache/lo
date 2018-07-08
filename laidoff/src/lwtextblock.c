#include "render_text_block.h"

void lwtextblock_query_only_text_block(const LWCONTEXT* pLwc, const LWTEXTBLOCK* text_block, LWTEXTBLOCKQUERYRESULT* query_result) {
    render_query_text_block_alpha(pLwc, text_block, 0, query_result, 1);
}
