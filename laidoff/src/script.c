#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>
#include "lwcontext.h"
#include "script.h"
#include "file.h"
#include "lwlog.h"
#include "laidoff.h"
#include <assert.h>
#include "zmq.h"
#include "logic.h"

#define LW_SCRIPT_PREFIX_PATH ASSETS_BASE_PATH "l" PATH_SEPARATOR
#define LW_MAX_CORO (32)

enum LW_SCRIPT_METADATA_TABLE_KEY {
    // Coroutine pool index (lua uses 1-based index)
    LSMTK_COROUTINE_INDEX = 1,
    // Recursive traceback string for debugging
    LSMTK_DEBUG_TRACEBACK = 2,
    // Coroutine lua object reference (prevent garbage collected automatically)
    LSMTK_COROUTINE_REF = 3,
};

// Defined at lo_wrap.c
int luaopen_lo(lua_State* L);

static LWCONTEXT* context;

typedef struct _LWCORO {
    int valid;
    lua_State* L;
    double yield_remain;
    int yield_wait_near_puck_player; // reset when near_puck_player is called
    int yield_wait_near_puck_player_with_dash; // reset when near_puck_player is called with player-dash state
    int yield_wait_player_attack; // reset when player attacked enemy(target) tower
    int yield_wait_target_attack; // reset when target attacked player tower
} LWCORO;

typedef struct _LWSCRIPT {
    LWCORO coro[LW_MAX_CORO];
} LWSCRIPT;

int l_ink(lua_State *L) {
    int x;
    if (lua_gettop(L) >= 0) {
        x = (int)lua_tonumber(L, -1);
        lua_pushnumber(L, 2 * x + 1);
    }
    return 1;
}

static int lua_cb(lua_State *L) {
    //printf("called lua_cb\n");
    return 0;
}

int l_yield_wait_target_attack(lua_State* L) {
    if (!lua_isinteger(L, 1)) {
        lua_pushliteral(L, "incorrect argument");
    }
    // Get wait ms from function argument (lua integer is 64-bit)
    int attack_count = (int)lua_tointeger(L, 1);
    // Get coroutine metadata table from registry
    lua_pushlightuserdata(L, L);
    lua_gettable(L, LUA_REGISTRYINDEX);
    int table_index = lua_gettop(L);
    if (!lua_istable(L, -1)) {
        lua_pushliteral(L, "incorrect argument");
        lua_error(L);
    }
    // Get first table value from metadata table (which is coroutine index)
    lua_pushinteger(L, LSMTK_COROUTINE_INDEX);
    lua_gettable(L, table_index);
    int coro_index = (int)lua_tointeger(L, -1);
    LWCONTEXT* pLwc = context;
    LWSCRIPT* script = (LWSCRIPT*)pLwc->script;
    script->coro[coro_index].yield_wait_target_attack = attack_count;
    //LOGI("Coro[%d] Set yield remain %f", coro_index, script->coro[coro_index].yield_remain);
    lua_pushnumber(L, 1);
    lua_pushnumber(L, 2);
    lua_pushcfunction(L, lua_cb);
    return lua_yield(L, 3);
}

int l_yield_wait_player_attack(lua_State* L) {
    if (!lua_isinteger(L, 1)) {
        lua_pushliteral(L, "incorrect argument");
    }
    // Get wait ms from function argument (lua integer is 64-bit)
    int attack_count = (int)lua_tointeger(L, 1);
    // Get coroutine metadata table from registry
    lua_pushlightuserdata(L, L);
    lua_gettable(L, LUA_REGISTRYINDEX);
    int table_index = lua_gettop(L);
    if (!lua_istable(L, -1)) {
        lua_pushliteral(L, "incorrect argument");
        lua_error(L);
    }
    // Get first table value from metadata table (which is coroutine index)
    lua_pushinteger(L, LSMTK_COROUTINE_INDEX);
    lua_gettable(L, table_index);
    int coro_index = (int)lua_tointeger(L, -1);
    LWCONTEXT* pLwc = context;
    LWSCRIPT* script = (LWSCRIPT*)pLwc->script;
    script->coro[coro_index].yield_wait_player_attack = attack_count;
    //LOGI("Coro[%d] Set yield remain %f", coro_index, script->coro[coro_index].yield_remain);
    lua_pushnumber(L, 1);
    lua_pushnumber(L, 2);
    lua_pushcfunction(L, lua_cb);
    return lua_yield(L, 3);
}

int l_yield_wait_near_puck_player_with_dash(lua_State* L) {
    if (!lua_isinteger(L, 1)) {
        lua_pushliteral(L, "incorrect argument");
    }
    // Get wait ms from function argument (lua integer is 64-bit)
    int near_count = (int)lua_tointeger(L, 1);
    // Get coroutine metadata table from registry
    lua_pushlightuserdata(L, L);
    lua_gettable(L, LUA_REGISTRYINDEX);
    int table_index = lua_gettop(L);
    if (!lua_istable(L, -1)) {
        lua_pushliteral(L, "incorrect argument");
        lua_error(L);
    }
    // Get first table value from metadata table (which is coroutine index)
    lua_pushinteger(L, LSMTK_COROUTINE_INDEX);
    lua_gettable(L, table_index);
    int coro_index = (int)lua_tointeger(L, -1);
    LWCONTEXT* pLwc = context;
    LWSCRIPT* script = (LWSCRIPT*)pLwc->script;
    script->coro[coro_index].yield_wait_near_puck_player_with_dash = near_count;
    //LOGI("Coro[%d] Set yield remain %f", coro_index, script->coro[coro_index].yield_remain);
    lua_pushnumber(L, 1);
    lua_pushnumber(L, 2);
    lua_pushcfunction(L, lua_cb);
    return lua_yield(L, 3);
}

int l_yield_wait_near_puck_player(lua_State* L) {
    if (!lua_isinteger(L, 1)) {
        lua_pushliteral(L, "incorrect argument");
    }
    // Get wait ms from function argument (lua integer is 64-bit)
    int near_count = (int)lua_tointeger(L, 1);
    // Get coroutine metadata table from registry
    lua_pushlightuserdata(L, L);
    lua_gettable(L, LUA_REGISTRYINDEX);
    int table_index = lua_gettop(L);
    if (!lua_istable(L, -1)) {
        lua_pushliteral(L, "incorrect argument");
        lua_error(L);
    }
    // Get first table value from metadata table (which is coroutine index)
    lua_pushinteger(L, LSMTK_COROUTINE_INDEX);
    lua_gettable(L, table_index);
    int coro_index = (int)lua_tointeger(L, -1);
    LWCONTEXT* pLwc = context;
    LWSCRIPT* script = (LWSCRIPT*)pLwc->script;
    script->coro[coro_index].yield_wait_near_puck_player = near_count;
    //LOGI("Coro[%d] Set yield remain %f", coro_index, script->coro[coro_index].yield_remain);
    lua_pushnumber(L, 1);
    lua_pushnumber(L, 2);
    lua_pushcfunction(L, lua_cb);
    return lua_yield(L, 3);
}

int l_yield_wait_ms(lua_State* L) {
    if (!lua_isinteger(L, 1)) {
        lua_pushliteral(L, "incorrect argument");
    }
    // Get wait ms from function argument (lua integer is 64-bit)
    int wait_ms = (int)lua_tointeger(L, 1);
    // Get coroutine metadata table from registry
    lua_pushlightuserdata(L, L);
    lua_gettable(L, LUA_REGISTRYINDEX);
    int table_index = lua_gettop(L);
    if (!lua_istable(L, -1)) {
        lua_pushliteral(L, "incorrect argument");
        lua_error(L);
    }
    // Get first table value from metadata table (which is coroutine index)
    lua_pushinteger(L, LSMTK_COROUTINE_INDEX);
    lua_gettable(L, table_index);
    int coro_index = (int)lua_tointeger(L, -1);
    LWCONTEXT* pLwc = context;
    LWSCRIPT* script = (LWSCRIPT*)pLwc->script;
    script->coro[coro_index].yield_remain = wait_ms / 1000.0;
    //LOGI("Coro[%d] Set yield remain %f", coro_index, script->coro[coro_index].yield_remain);
    lua_pushnumber(L, 1);
    lua_pushnumber(L, 2);
    lua_pushcfunction(L, lua_cb);
    return lua_yield(L, 3);
}

void push_recursive_traceback(lua_State* L_coro, lua_State* L) {
    // Get coroutine metadata table of parent (L) from registry
    lua_pushlightuserdata(L, L);
    lua_gettable(L, LUA_REGISTRYINDEX);
    if (!lua_istable(L, -1)) {
        // No metadata table exists (L is main thread)
        // Pop everything and add main thread's traceback
        lua_pop(L, 1);
        // Push L's traceback to L_coro
        luaL_traceback(L_coro, L, NULL, 1);
    } else {
        int table_index = lua_gettop(L);
        // Get second table value from metadata table (which is parent thread traceback debug info)
        lua_pushinteger(L, LSMTK_DEBUG_TRACEBACK);
        lua_gettable(L, table_index);
        if (lua_isstring(L, -1)) {
            const char* parent_parent_traceback = lua_tostring(L, -1);
            // Push L's traceback to L_coro
            luaL_traceback(L_coro, L, NULL, 1);
            const char* parent_traceback = lua_tostring(L_coro, -1);
            char tb[2048];
            tb[0] = '\0';
            strncat(tb, parent_traceback, sizeof(tb) - 1);
            strncat(tb, "\n", sizeof(tb) - 1);
            strncat(tb, parent_parent_traceback, sizeof(tb) - 1);
            lua_pop(L, 1);
            lua_pop(L_coro, 1);
            lua_pushstring(L_coro, tb);
        } else {
            LOGEP("coro metadata table corrupted");
            abort();
        }
    }
}

int l_start_coro(lua_State* L) {
    if (lua_gettop(L) >= 1) {
        LWCONTEXT* pLwc = lua_touserdata(L, lua_upvalueindex(1));
        if (!lua_isfunction(L, 1)) {
            lua_pushliteral(L, "incorrect argument");
            lua_error(L);
            return 0;
        }
        LWSCRIPT* script = (LWSCRIPT*)pLwc->script;
        for (int i = 0; i < LW_MAX_CORO; i++) {
            if (!script->coro[i].valid) {
                script->coro[i].valid = 1;
                // Create coroutine; created coroutine will be pushed on top of L's stack
                lua_State* L_coro = lua_newthread(L);
                int coro_index = lua_gettop(L);
                assert(lua_isthread(L, coro_index));
                //LOGI("New coroutine started: stack top = %d", coro_index);
                // Save coroutine pointer in coro pool
                script->coro[i].L = L_coro;
                // Create coroutine custom data table
                lua_createtable(L_coro, 2, 0);
                int table_index = lua_gettop(L_coro);
                // Set coroutine array index to registry (key: LSMTK_COROUTINE_INDEX, value: i)
                lua_pushinteger(L_coro, LSMTK_COROUTINE_INDEX);
                lua_pushinteger(L_coro, i);
                lua_settable(L_coro, table_index);
                // Set coroutine traceback of parent(main) thread for debugging purpose (key: LSMTK_DEBUG_TRACEBACK, value: string)
                lua_pushinteger(L_coro, LSMTK_DEBUG_TRACEBACK);
                push_recursive_traceback(L_coro, L);
                lua_settable(L_coro, table_index);
                // Set coroutine reference (key: LSMTK_COROUTINE_REF, value: coroutine object)
                lua_pushinteger(L_coro, LSMTK_COROUTINE_REF);
                lua_pushvalue(L, coro_index);
                lua_xmove(L, L_coro, 1);
                lua_settable(L_coro, table_index);
                // Set metadata table to registry (key: L_coro, value: metadata table)
                lua_pushlightuserdata(L_coro, L_coro);
                lua_pushvalue(L_coro, table_index);
                lua_settable(L_coro, LUA_REGISTRYINDEX);
                // Copy coroutine entry function (first argument of 'start_coro')
                // and add onto the top of stack.
                lua_pushvalue(L, 1);
                // Move coroutine entry function to new thread's stack
                lua_xmove(L, L_coro, 1);
                // Return coroutine object
                lua_pushvalue(L, coro_index);
                return 1;
            }
        }
        //lua_pushinteger(L, 0);
    }
    return 0;
}

int l_spawn_blue_cube_wall_2(lua_State* L) {
    if (lua_gettop(L) >= 2) {
        LWCONTEXT* pLwc = lua_touserdata(L, lua_upvalueindex(1));
        float x = (float)lua_tonumber(L, 1);
        float y = (float)lua_tonumber(L, 2);
        int r = spawn_field_object(pLwc->field, x, y, 1, 1, LVT_CUBE_WALL, pLwc->tex_programmed[LPT_SOLID_BLUE], 1, 1, 1, 0, 0);
        lua_pushinteger(L, r);
    }
    return 1;
}

int l_spawn_blue_cube_wall(lua_State* L) {
    if (lua_gettop(L) >= 3) {
        LWCONTEXT* pLwc = lua_touserdata(L, 1);
        float x = (float)lua_tonumber(L, 2);
        float y = (float)lua_tonumber(L, 3);
        int r = spawn_field_object(pLwc->field, x, y, 1, 1, LVT_CUBE_WALL, pLwc->tex_programmed[LPT_SOLID_BLUE], 1, 1, 1, 0, 0);
        lua_pushinteger(L, r);
    }
    return 1;
}

int l_spawn_red_cube_wall(lua_State* L) {
    if (lua_gettop(L) >= 4) {
        LWCONTEXT* pLwc = lua_touserdata(L, 1);
        float x = (float)lua_tonumber(L, 2);
        float y = (float)lua_tonumber(L, 3);
        int field_event_id = (int)lua_tonumber(L, 4);
        int r = spawn_field_object(pLwc->field, x, y, 1, 1, LVT_CUBE_WALL, pLwc->tex_programmed[LPT_SOLID_RED], 1, 1, 0.5f, field_event_id, 0);
        lua_pushinteger(L, r);
    }
    return 1;
}

int l_spawn_pump(lua_State* L) {
    if (lua_gettop(L) >= 4) {
        LWCONTEXT* pLwc = lua_touserdata(L, 1);
        float x = (float)lua_tonumber(L, 2);
        float y = (float)lua_tonumber(L, 3);
        int field_event_id = (int)lua_tonumber(L, 4);
        int r = spawn_field_object(pLwc->field, x, y, 1, 1, LVT_PUMP, pLwc->tex_programmed[LPT_SOLID_RED], 1, 1, 0.5f, field_event_id, 0);
        lua_pushinteger(L, r);
    }
    return 1;
}

int l_spawn_oil_truck(lua_State* L) {
    if (lua_gettop(L) >= 4) {
        LWCONTEXT* pLwc = lua_touserdata(L, 1);
        float x = (float)lua_tonumber(L, 2);
        float y = (float)lua_tonumber(L, 3);
        int field_event_id = (int)lua_tonumber(L, 4);
        lw_load_tex_async(pLwc, LAE_3D_OIL_TRUCK_TEX_KTX); // TODO safe?
        int r = spawn_field_object(pLwc->field,
                                   x,
                                   y,
                                   1,
                                   1,
                                   LVT_OIL_TRUCK,
                                   pLwc->tex_atlas[LAE_3D_OIL_TRUCK_TEX_KTX],
                                   1,
                                   1,
                                   1.0f,
                                   field_event_id,
                                   0);
        lua_pushinteger(L, r);
    }
    return 1;
}

int l_spawn_devil(lua_State* L) {
    if (lua_gettop(L) >= 4) {
        LWCONTEXT* pLwc = lua_touserdata(L, 1);
        float x = (float)lua_tonumber(L, 2);
        float y = (float)lua_tonumber(L, 3);
        int field_event_id = (int)lua_tonumber(L, 4);
        lw_load_tex_async(pLwc, LAE_DEVIL_KTX);
        int r = spawn_field_object(pLwc->field,
                                   x,
                                   y,
                                   1,
                                   1,
                                   LVT_DEVIL,
                                   pLwc->tex_atlas[LAE_DEVIL_KTX],
                                   1,
                                   1,
                                   1.0f,
                                   field_event_id,
                                   0);
        lua_pushinteger(L, r);
    }
    return 1;
}

int l_load_module(lua_State* L) {
    if (lua_gettop(L) >= 2) {
        LWCONTEXT* pLwc = lua_touserdata(L, 1);
        const char* filename = lua_tostring(L, 2);
        // returns 0 if no error, -1 otherwise
        int result = script_run_file_ex(pLwc, filename, 0);
        // returning the return value count
        return result == 0 ? 1 : 0;
    }
    return 0;
}

static int coro_gc(lua_State* L) {
    puts("__gc called");
    return 0;
}

void deinit_lua(LWCONTEXT* pLwc) {
    // Clear all coroutines
    script_cleanup_all_coros(pLwc);
    // Close lua instance
    if (pLwc->L) {
        lua_close(pLwc->L);
        pLwc->L = 0;
    }
    // Free LWSCRIPT instance
    if (pLwc->script) {
        free(pLwc->script);
        pLwc->script = 0;
    }
}

static int l_my_print(lua_State* L) {
    int n = lua_gettop(L);  /* number of arguments */
    int i;
    lua_getglobal(L, "tostring");
    char strbuf[4096] = { 0, };
    for (i = 1; i <= n; i++) {
        const char *s;
        size_t l;
        lua_pushvalue(L, -1);  /* function to be called */
        lua_pushvalue(L, i);   /* value to print */
        lua_call(L, 1, 1);
        s = lua_tolstring(L, -1, &l);  /* get result */
        if (s == NULL)
            return luaL_error(L, "'tostring' must return a string to 'print'");
        if (i > 1) strcat(strbuf, "\t"); // lua_writestring("\t", 1);
        strncat(strbuf, s, l); // lua_writestring(s, l);
        lua_pop(L, 1);  /* pop result */
    }
    LOGI("%s", strbuf);
    return 0;
}

void init_lua(LWCONTEXT* pLwc) {
    deinit_lua(pLwc);

    LWSCRIPT* script = (LWSCRIPT*)calloc(1, sizeof(LWSCRIPT));
    pLwc->script = script;

    lua_State* L = luaL_newstate();
    // Load lua standard libs
    luaL_openlibs(L);
    // override default 'print()' function on lua
    {
        lua_getglobal(L, "_G");
        static const struct luaL_Reg printlib[] = {
            { "print", l_my_print },
        { NULL, NULL } /* end of array */
        };
        // luaL_register(L, NULL, printlib); // for Lua versions < 5.2
        luaL_setfuncs(L, printlib, 0);  // for Lua versions 5.2 or greater
        lua_pop(L, 1);
    }
    // ink
    lua_register(L, "ink", l_ink);
    // spawn_blue_cube_wall
    lua_register(L, "spawn_blue_cube_wall", l_spawn_blue_cube_wall);
    // spawn_blue_cube_wall_2 (first upvalue is pLwc)
    lua_pushlightuserdata(L, pLwc);
    lua_pushcclosure(L, l_spawn_blue_cube_wall_2, 1);
    lua_setglobal(L, "spawn_blue_cube_wall_2");
    // start_coro (first upvalue is pLwc)
    lua_pushlightuserdata(L, pLwc);
    lua_pushcclosure(L, l_start_coro, 1);
    lua_setglobal(L, "start_coro");
    // yield_wait_ms (first upvalue is pLwc)
    lua_pushlightuserdata(L, pLwc);
    lua_pushcclosure(L, l_yield_wait_ms, 1);
    lua_setglobal(L, "yield_wait_ms");
    // yield_near_puck_player
    lua_pushlightuserdata(L, pLwc);
    lua_pushcclosure(L, l_yield_wait_near_puck_player, 1);
    lua_setglobal(L, "yield_wait_near_puck_player");
    // yield_near_puck_player_with_dash
    lua_pushlightuserdata(L, pLwc);
    lua_pushcclosure(L, l_yield_wait_near_puck_player_with_dash, 1);
    lua_setglobal(L, "yield_wait_near_puck_player_with_dash");
    // yield_wait_player_attack
    lua_pushlightuserdata(L, pLwc);
    lua_pushcclosure(L, l_yield_wait_player_attack, 1);
    lua_setglobal(L, "yield_wait_player_attack");
    // yield_wait_target_attack
    lua_pushlightuserdata(L, pLwc);
    lua_pushcclosure(L, l_yield_wait_target_attack, 1);
    lua_setglobal(L, "yield_wait_target_attack");
    // spawn_red_cube_wall
    lua_register(L, "spawn_red_cube_wall", l_spawn_red_cube_wall);
    // spawn_pump
    lua_register(L, "spawn_pump", l_spawn_pump);
    // spawn_oil_truck
    lua_register(L, "spawn_oil_truck", l_spawn_oil_truck);
    // spawn_oil_truck
    lua_register(L, "spawn_devil", l_spawn_devil);
    // load_module
    lua_register(L, "load_module", l_load_module);
    // 'pLwc'
    lua_pushlightuserdata(L, pLwc);
    lua_setglobal(L, "pLwc");
    // Load 'lo' library generated from swig
    luaopen_lo(L);
    // Register coroutine(lua thread) gc callback
    struct luaL_Reg coro_metatable_funcs[] = {
        { "__gc", coro_gc },
        {NULL, NULL},
    };
    luaL_newmetatable(L, "coro_metatable");
    luaL_setfuncs(L, coro_metatable_funcs, 0);
    // Set pLwc context to shared static variable for acessing from scripts
    script_set_context(pLwc);

    {
        int result = luaL_dostring(L, "return ink(1000)");
        if (result) {
            fprintf(stderr, "Failed to run lua: %s\n", lua_tostring(L, -1));
        } else {
            LOGIx("Lua result: %lld", lua_tointeger(L, -1));
        }
        lua_pop(L, 1);
    }

    pLwc->L = L;
    //lua_close(L);

    script_run_file(pLwc, ASSETS_BASE_PATH "l" PATH_SEPARATOR "post_init_once.lua");
}

static int traceback(lua_State *L) {
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 2);
    lua_call(L, 2, 1);
    //fprintf(stderr, "%s\n", lua_tostring(L, -1));
    return 1;
}

int script_run_file_ex(LWCONTEXT* pLwc, const char* filename, int pop_result) {
    char* script = create_string_from_file(filename);
    if (script) {
        //int result = luaL_dostring(pLwc->L, script);
        char prefixed_filename[256];
        prefixed_filename[0] = '\0';
        strncat(prefixed_filename, "@", sizeof(prefixed_filename) - 1);
        strncat(prefixed_filename, filename, sizeof(prefixed_filename) - 1);
        lua_pushcfunction(pLwc->L, traceback);
        int result = (luaL_loadbuffer(pLwc->L, script, strlen(script), prefixed_filename) || lua_pcall(pLwc->L, 0, LUA_MULTRET, lua_gettop(pLwc->L) - 1));
        release_string(script);
        if (result) {
            LOGE("ERROR: %s", lua_tostring(pLwc->L, -1));
            // Push traceback information
            //luaL_traceback(pLwc->L, pLwc->L, NULL, 1);
            //LOGE(lua_tostring(pLwc->L, -1));
        } else {
            LOGIx("Lua result: %lld", lua_tointeger(pLwc->L, -1));
        }
        if (pop_result) {
            lua_pop(pLwc->L, 1);
        }
        return 0;
    }

    LOGE("script_run_file: loading script %s failed", filename);
    return -1;
}

void script_run_file(LWCONTEXT* pLwc, const char* filename) {
    script_run_file_ex(pLwc, filename, 1);
}

void spawn_all_field_object(LWCONTEXT* pLwc) {
    script_run_file(pLwc, LW_SCRIPT_PREFIX_PATH "spawn.lua");
}

void script_set_context(LWCONTEXT* pLwc) {
    context = pLwc;
}

LWCONTEXT* script_context() {
    return context;
}

const char* script_prefix_path() {
    return LW_SCRIPT_PREFIX_PATH;
}

static void s_cleanup_coro(LWSCRIPT* script, int idx) {
    if (!script->coro[idx].valid) {
        LOGFP("invalid pool entry to be cleaned up");
    }
    lua_State* L = script->coro[idx].L;
    if (L) {
        // Get coroutine custom data table from registry
        lua_pushlightuserdata(L, L);
        lua_gettable(L, LUA_REGISTRYINDEX);
        if (!lua_istable(L, -1)) {
            lua_pushliteral(L, "incorrect argument");
            lua_error(L);
            abort();
        }
        // Remove entry from registry
        lua_pushlightuserdata(L, L);
        lua_pushnil(L);
        lua_settable(L, LUA_REGISTRYINDEX);
    }
    memset(script->coro + idx, 0, sizeof(LWCORO));
}

void script_cleanup_all_coros(LWCONTEXT* pLwc) {
    LWSCRIPT* script = (LWSCRIPT*)pLwc->script;
    if (script) {
        for (int i = 0; i < LW_MAX_CORO; i++) {
            if (script->coro[i].valid) {
                s_cleanup_coro(script, i);
            }
        }
    }
}

void script_update(LWCONTEXT* pLwc) {
    if (!pLwc->script) {
        return;
    }
    LWSCRIPT* script = (LWSCRIPT*)pLwc->script;
    for (int i = 0; i < LW_MAX_CORO; i++) {
        if (script->coro[i].valid) {
            // Only process if yield-able
            if (script->coro[i].yield_remain > 0) {
                // Wait some time
                script->coro[i].yield_remain -= lwcontext_delta_time(pLwc);
            } else if (script->coro[i].yield_wait_near_puck_player > 0) {
                // Wait yield_wait_near_puck_player flag to be reset
            } else if (script->coro[i].yield_wait_near_puck_player_with_dash > 0) {
                // Wait yield_wait_near_puck_player_with_dash flag to be reset
            } else if (script->coro[i].yield_wait_player_attack > 0) {
                // Wait yield_wait_player_attack flag to be reset
            } else if (script->coro[i].yield_wait_target_attack > 0) {
                // Wait yield_wait_target_attack flag to be reset
            } else {
                // Resume immediately
                lua_State* L_coro = script->coro[i].L;
                int status = lua_resume(L_coro, NULL, 0);
                if (status == LUA_YIELD) {
                    lua_CFunction f = lua_tocfunction(L_coro, -1);
                    f(L_coro);
                } else if (status == LUA_OK) {
                    // Coroutine execution completed
                    s_cleanup_coro(script, i);
                } else {
                    // Coroutine has errors
                    // Print debug message
                    LOGE("ERROR in coroutine: %s", lua_tostring(L_coro, -1));
                    // Push traceback information
                    luaL_traceback(L_coro, L_coro, NULL, 1);
                    LOGE("%s", lua_tostring(L_coro, -1));

                    // Get coroutine metadata table from registry
                    lua_pushlightuserdata(L_coro, L_coro);
                    lua_gettable(L_coro, LUA_REGISTRYINDEX);
                    int table_index = lua_gettop(L_coro);
                    if (!lua_istable(L_coro, -1)) {
                        lua_pushliteral(L_coro, "incorrect argument");
                        lua_error(L_coro);
                    }
                    // Get second table value from metadata table (which is parent thread traceback debug info)
                    lua_pushinteger(L_coro, LSMTK_DEBUG_TRACEBACK);
                    lua_gettable(L_coro, table_index);
                    LOGE("%s", lua_tostring(L_coro, -1));
                    LOGE("end of stack traceback");
                    // Coroutine execution aborted
                    s_cleanup_coro(script, i);
                }
            }
        }
    }
}

int script_emit_anim_marker(void* L, int key, const char* name) {
    int ret;
    // push functions and arguments
    lua_getglobal(L, "on_anim_marker"); // function to be called
    lua_pushinteger(L, key); // push 1st argument
    lua_pushstring(L, name); // push 2nd argument
    // do the call (2 arguments, 1 result)
    if (lua_pcall(L, 2, 1, 0) != 0) {
        LOGEP("error: %s", lua_tostring(L, -1));
    }
    /* retrieve result */
    if (!lua_isnumber(L, -1)) {
        LOGEP("error: %s", lua_tostring(L, -1));
    }
    ret = (int)lua_tonumber(L, -1);
    lua_pop(L, 1); // pop returned value
    return ret;
}

int script_emit_near(void* L, int key1, int key2) {
    int ret;
    // push functions and arguments
    lua_getglobal(L, "on_near"); // function to be called
    lua_pushinteger(L, key1); // push 1st argument
    lua_pushinteger(L, key2); // push 1st argument
    // do the call (2 arguments, 1 result)
    if (lua_pcall(L, 2, 1, 0) != 0) {
        LOGEP("error: %s", lua_tostring(L, -1));
    }
    // retrieve result
    if (!lua_isnumber(L, -1)) {
        LOGEP("error: %s", lua_tostring(L, -1));
    }
    ret = (int)lua_tonumber(L, -1);
    lua_pop(L, 1); // pop returned value
    return ret;
}

int script_emit_logic_frame_finish(void* L, float delta_time) {
    if (L == 0) {
        return 0;
    }
    int ret;
    // push functions and arguments
    lua_getglobal(L, "on_logic_frame_finish"); // function to be called
    lua_pushnumber(L, delta_time); // push 1st argument
    // do the call (0 arguments, 1 result)
    if (lua_pcall(L, 1, 1, 0) != 0) {
        LOGEP("error: %s", lua_tostring(L, -1));
    }
    // retrieve result
    if (!lua_isnumber(L, -1)) {
        LOGEP("error: %s", lua_tostring(L, -1));
    }
    ret = (int)lua_tonumber(L, -1);
    lua_pop(L, 1); // pop returned value
    return ret;
}

int script_emit_ui_event(void* L, const char* id, float w_ratio, float h_ratio) {
    int ret;
    // push functions and arguments
    lua_getglobal(L, "on_ui_event"); // function to be called
    lua_pushstring(L, id); // push 2nd argument
    lua_pushnumber(L, w_ratio); // push 3rd argument
    lua_pushnumber(L, h_ratio); // push 4th argument
    // do the call (3 arguments, 1 result)
    if (lua_pcall(L, 3, 1, 0) != 0) {
        LOGEP("error: %s", lua_tostring(L, -1));
    }
    /* retrieve result */
    if (!lua_isnumber(L, -1)) {
        LOGEP("error: %s", lua_tostring(L, -1));
    }
    ret = (int)lua_tonumber(L, -1);
    lua_pop(L, 1); // pop returned value
    return ret;
}

void script_get_string(void* L, const char* id, char* ret, int ret_max_len) {
    // push functions and arguments
    lua_getglobal(L, "get_string"); // function to be called
    lua_pushstring(L, id); // push 2nd argument
    // do the call (1 arguments, 1 result)
    if (lua_pcall(L, 1, 1, 0) != 0) {
        LOGEP("error: %s", lua_tostring(L, -1));
    }
    /* retrieve result */
    if (!lua_isstring(L, -1)) {
        LOGEP("error: %s", lua_tostring(L, -1));
    }
    const char* ret_v = lua_tostring(L, -1);
    strncpy(ret, ret_v, ret_max_len);
    ret[ret_max_len - 1] = '\0';
    lua_pop(L, 1); // pop returned value
}

void script_on_near_puck_player(void* _script, int dashing) {
    LWSCRIPT* script = (LWSCRIPT*)_script;
    if (script) {
        for (int i = 0; i < LW_MAX_CORO; i++) {
            if (script->coro[i].valid) {
                if (script->coro[i].yield_wait_near_puck_player > 0) {
                    script->coro[i].yield_wait_near_puck_player--;
                }
                if (dashing && script->coro[i].yield_wait_near_puck_player_with_dash > 0) {
                    script->coro[i].yield_wait_near_puck_player_with_dash--;
                }
            }
        }
    }
}

void script_on_player_attack(void* _script) {
    LWSCRIPT* script = (LWSCRIPT*)_script;
    if (script) {
        for (int i = 0; i < LW_MAX_CORO; i++) {
            if (script->coro[i].valid) {
                if (script->coro[i].yield_wait_player_attack > 0) {
                    script->coro[i].yield_wait_player_attack--;
                }
            }
        }
    }
}

void script_on_target_attack(void* _script) {
    LWSCRIPT* script = (LWSCRIPT*)_script;
    if (script) {
        for (int i = 0; i < LW_MAX_CORO; i++) {
            if (script->coro[i].valid) {
                if (script->coro[i].yield_wait_target_attack > 0) {
                    script->coro[i].yield_wait_target_attack--;
                }
            }
        }
    }
}

void script_evaluate_with_name(void* L, const char* code, size_t code_len, const char* name) {
    int top0 = lua_gettop(L);
    int result = (luaL_loadbuffer(L, code, code_len, name) || lua_pcall(L, 0, LUA_MULTRET, 0));
    if (result) {
        LOGE("Failed to run lua: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    } else {
        int top1 = lua_gettop(L);
        int nret = top1 - top0;
        while (nret) {
            LOGIx("Lua result: %lld", lua_tointeger(L, -1));
            nret--;
        }
        lua_pop(L, nret);
    }
}

void script_evaluate_with_name_async(LWCONTEXT* pLwc, const char* code, size_t code_len, const char* name) {
    logic_emit_evalute_with_name_async(pLwc, code, code_len, name);
}

void script_evaluate(void* L, const char* code, size_t code_len) {
    script_evaluate_with_name(L, code, code_len, SCRIPT_UNNAMED_SCRIPT_NAME);
}

void script_evaluate_async(LWCONTEXT* pLwc, const char* code, size_t code_len) {
    logic_emit_evalute_async(pLwc, code, code_len);
}

const char* script_full_asset_path(const char* asset_type, const char* asset_name) {
    static char full_asset_path[1024] = { 0, };
    full_asset_path[0] = 0;
    strcat(full_asset_path, ASSETS_BASE_PATH);
    strcat(full_asset_path, asset_type);
    strcat(full_asset_path, PATH_SEPARATOR);
    strcat(full_asset_path, asset_name);
    return full_asset_path;
}

int script_http_header(void* L, char* header, size_t header_max_len) {
    if (L == 0) {
        return 0;
    }
    int ret = 0;
    // push functions and arguments
    lua_getglobal(L, "http_header"); // function to be called
    // do the call (0 arguments, 1 result)
    if (lua_pcall(L, 0, 1, 0) != 0) {
        LOGEP("error: %s", lua_tostring(L, -1));
    }
    // retrieve result
    if (!lua_isstring(L, -1)) {
        LOGEP("error: %s", lua_tostring(L, -1));
        ret = -1;
    } else {
        strncpy(header, lua_tostring(L, -1), header_max_len - 1);
        header[header_max_len - 1] = 0;
    }
    lua_pop(L, 1); // pop returned value
    return ret;
}

int script_emit_single_string_arg(void* L, const char* func_name, const char* str_arg) {
	int ret;
	// push functions and arguments
	lua_getglobal(L, func_name); // function to be called
	lua_pushstring(L, str_arg); // push 2nd argument
	
								// do the call (1 arguments, 1 result)
	if (lua_pcall(L, 1, 1, 0) != 0) {
		LOGEP("error: %s", lua_tostring(L, -1));
	}
	/* retrieve result */
	if (!lua_isnumber(L, -1)) {
		LOGEP("error: %s", lua_tostring(L, -1));
	}
	ret = (int)lua_tonumber(L, -1);
	lua_pop(L, 1); // pop returned value
	return ret;
}
