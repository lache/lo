#ifndef render_leaderboard_h
#define render_leaderboard_h

typedef struct _LWCONTEXT LWCONTEXT;

void lwc_render_leaderboard(const LWCONTEXT* pLwc);
void render_leaderboard_table(const LWCONTEXT* pLwc, float x0, float y0, float ui_alpha);

#endif /* render_leaderboard_h */
