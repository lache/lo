#include "precompiled.hpp"
#include "udp_server.hpp"
#include "tcp_server.hpp"
#include "sea.hpp"
#include "udp_admin_server.hpp"
#include "sea_static.hpp"
#include "seaport.hpp"
#include "region.hpp"
#include "city.hpp"
#include "salvage.hpp"
#include "shipyard.hpp"
#include "session.hpp"
#include "contract.hpp"
#include "ss_wrap.inl"

using namespace ss;

boost::asio::io_service io_service;
std::shared_ptr<udp_server> udp_server_instance;
std::shared_ptr<udp_admin_server> udp_admin_server_instance;
std::shared_ptr<sea_static> sea_static_instance;
std::shared_ptr<sea> sea_instance;
std::shared_ptr<seaport> seaport_instance;

extern "C" int lua_main(int argc, char **argv, void custom_init_func(lua_State* L));
extern "C" int handle_script(lua_State *L, char **argv);

int lua_attach_traceback(lua_State *L) {
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 2);
    lua_call(L, 2, 1);
    return 1;
}

void eval_lua_script_file(lua_State* L, const char* lua_filename) {
    char prefixed_filename[256];
    prefixed_filename[0] = '\0';
    strncat(prefixed_filename, "@", sizeof(prefixed_filename) - 1);
    strncat(prefixed_filename, lua_filename, sizeof(prefixed_filename) - 1);
    std::ifstream script_file(lua_filename);
    std::stringstream script_stream;
    script_stream << script_file.rdbuf();
    lua_pushcfunction(L, lua_attach_traceback);
    int lua_load_result = (luaL_loadbuffer(L,
                                           script_stream.str().c_str(),
                                           strlen(script_stream.str().c_str()), prefixed_filename)
                           || lua_pcall(L, 0, LUA_MULTRET, lua_gettop(L) - 1));
    if (lua_load_result) {
        LOGE("LUA ERROR: %1%", lua_tostring(L, -1));
    } else {
        LOGI("Lua result: %1%", lua_tointeger(L, -1));
    }
    // pop return value
    lua_pop(L, 1);
}

static int l_sea_static_calculate_waypoints(lua_State* L) {
    if (lua_gettop(L) >= 5) {
        int from_x = static_cast<int>(lua_tonumber(L, 1));
        int from_y = static_cast<int>(lua_tonumber(L, 2));
        int to_x = static_cast<int>(lua_tonumber(L, 3));
        int to_y = static_cast<int>(lua_tonumber(L, 4));
        int expect_land = static_cast<int>(lua_tonumber(L, 5));
        std::promise<std::vector<xy32>> prom;
        io_service.post([from_x, from_y, to_x, to_y, expect_land, &prom] {
            auto wp = sea_static_instance->calculate_waypoints(from_x, from_y, to_x, to_y, expect_land, std::shared_ptr<astarrtree::coro_context>());
            prom.set_value(wp);
        });
        auto wp = prom.get_future().get();
        lua_newtable(L);
        for (size_t i = 0; i < wp.size(); i++) {
            lua_newtable(L);
            lua_pushnumber(L, wp[i].x);
            lua_rawseti(L, -2, 1);
            lua_pushnumber(L, wp[i].y);
            lua_rawseti(L, -2, 2);

            lua_rawseti(L, -2, i + 1);
        }
        return 1;
    }
    return 0;
}

static int l_sea_spawn(lua_State* L) {
    if (lua_gettop(L) >= 3) {
        int sea_object_id = static_cast<int>(lua_tonumber(L, 1));
        float x = static_cast<float>(lua_tonumber(L, 2));
        float y = static_cast<float>(lua_tonumber(L, 3));
        std::promise<int> prom;
        io_service.post([sea_object_id, x, y, &prom] {
            prom.set_value(sea_instance->spawn(sea_object_id, x, y, 1, 1, 0, 1));
        });
        lua_pushnumber(L, prom.get_future().get());
        return 1;
    } else {
        LOGI("USAGE: sea_spawn(sea_object_id, x, y)");
    }
    return 0;
}

static int l_sea_teleport_to(lua_State* L) {
    if (lua_gettop(L) >= 3) {
        int sea_object_id = static_cast<int>(lua_tonumber(L, 1));
        float x = static_cast<float>(lua_tonumber(L, 2));
        float y = static_cast<float>(lua_tonumber(L, 3));
        std::promise<int> prom;
        io_service.post([sea_object_id, x, y, &prom] {
            prom.set_value(sea_instance->teleport_to(sea_object_id, x, y));
        });
        lua_pushnumber(L, prom.get_future().get());
        return 1;
    }
    return 0;
}

static int l_sea_dock(lua_State* L) {
	if (lua_gettop(L) >= 1) {
		int sea_object_id = static_cast<int>(lua_tonumber(L, 1));
		std::promise<int> prom;
		io_service.post([sea_object_id, &prom] {
			prom.set_value(sea_instance->dock(sea_object_id));
		});
		lua_pushnumber(L, prom.get_future().get());
		return 1;
	}
	return 0;
}

static int l_sea_undock(lua_State* L) {
    if (lua_gettop(L) >= 1) {
        int sea_object_id = static_cast<int>(lua_tonumber(L, 1));
        std::promise<int> prom;
        io_service.post([sea_object_id, &prom] {
            prom.set_value(sea_instance->undock(sea_object_id));
        });
        lua_pushnumber(L, prom.get_future().get());
        return 1;
    }
    return 0;
}

static int l_seaport_new_resource(lua_State* L) {
    if (lua_gettop(L) >= 3) {
        int seaport_id = static_cast<int>(lua_tonumber(L, 1));
        int resource_id = static_cast<int>(lua_tonumber(L, 2));
        int amount = static_cast<int>(lua_tonumber(L, 3));
        std::promise<int> prom;
        io_service.post([seaport_id, resource_id, amount, &prom] {
            prom.set_value(seaport_instance->add_resource(seaport_id, resource_id, amount));
        });
        lua_pushnumber(L, prom.get_future().get());
        return 1;
    }
    return 0;
}


void custom_lua_init(lua_State* L) {
    SWIG_init(L);
    lua_pushcclosure(L, l_sea_static_calculate_waypoints, 0);
    lua_setglobal(L, "sea_static_calculate_waypoints");
    lua_pushcclosure(L, l_sea_spawn, 0);
    lua_setglobal(L, "sea_spawn");
    lua_pushcclosure(L, l_sea_teleport_to, 0);
    lua_setglobal(L, "sea_teleport_to");
	lua_pushcclosure(L, l_sea_dock, 0);
	lua_setglobal(L, "sea_dock");
    lua_pushcclosure(L, l_sea_undock, 0);
    lua_setglobal(L, "sea_undock");
    lua_pushcclosure(L, l_seaport_new_resource, 0);
    lua_setglobal(L, "seaport_add_resource");
}

int post_admin_message(const unsigned char* b) {
    std::promise<int> prom;
    io_service.post([b, &prom] {
        prom.set_value(udp_admin_server_instance->process_command(b, false));
    });
    return prom.get_future().get();
}

int endpoints(char*** o, int* n) {
    auto eps = udp_server_instance->endpoints();
    *n = static_cast<int>(eps.size());
    if (*n <= 0) {
        *o = nullptr;
        return 0;
    }
    char** o_arr = (char**)calloc(*n, sizeof(char*));
    *o = o_arr;
    if (o_arr == nullptr) {
        *o = nullptr;
        return -1;
    }
    for (int i = 0; i < *n; i++) {
        auto s = eps[i].address().to_string() + ":" + std::to_string(eps[i].port());
        o_arr[i] = strdup(s.c_str());
        if (o_arr[i] == nullptr) {
            return -1;
        }
    }
    return 0;
}

int g_production = 0;

int main(int argc, char* argv[]) {
    try {
        const int major = 0;
        const int minor = 1;
        const int patch = 0;
        if (argc > 1 && strcmp(argv[1], "production") == 0) {
            g_production = 1;
        }
        LOGI("sea-server v%d.%d.%d", major, minor, patch);
        auto cwd = boost::filesystem::current_path();
        do {
            auto assets = cwd;
            assets.append("assets");
            if (boost::filesystem::is_directory(assets)) {
                boost::filesystem::current_path(cwd);
                break;
            }
            cwd.remove_leaf();
        } while (!cwd.empty());

        if (cwd.empty()) {
            abort();
        }

        LOGI("Current path: %s", boost::filesystem::current_path());

        //std::shared_ptr<lua_State> lua_state_instance(luaL_newstate(), [](lua_State* L) {lua_close(L); });
        std::shared_ptr<lua_State> lua_state_instance(luaL_newstate(), lua_close);
        luaL_openlibs(lua_state_instance.get());
        eval_lua_script_file(lua_state_instance.get(), "assets/l/contract.lua");
        eval_lua_script_file(lua_state_instance.get(), "assets/l/loadunload.lua");
        eval_lua_script_file(lua_state_instance.get(), "assets/l/collection.lua");
        eval_lua_script_file(lua_state_instance.get(), "assets/l/sea_server.lua");
        
        sea_static_instance.reset(new sea_static());
        seaport_instance.reset(new seaport(io_service, lua_state_instance));
        sea_instance.reset(new sea(io_service, seaport_instance, lua_state_instance));
        std::shared_ptr<region> region_instance(new region());
        std::shared_ptr<city> city_instance(new city(io_service,
                                                     seaport_instance,
                                                     lua_state_instance));
        std::shared_ptr<salvage> salvage_instance(new salvage(io_service,
                                                              sea_static_instance));
        std::shared_ptr<shipyard> shipyard_instance(new shipyard(io_service,
                                                                 sea_static_instance));
        std::shared_ptr<ss::session> session_instance(new ss::session());
        std::shared_ptr<contract> contract_instance(new contract(io_service,
                                                                 sea_static_instance));
        if (argc > 1 && strcmp(argv[1], "--prepare") == 0) {
            LOGI("Preparation is completed.");
            return 0;
        }
        udp_server_instance.reset(new udp_server(io_service,
                                                 sea_instance,
                                                 sea_static_instance,
                                                 seaport_instance,
                                                 region_instance,
                                                 city_instance,
                                                 salvage_instance,
                                                 shipyard_instance,
                                                 session_instance,
                                                 contract_instance,
                                                 lua_state_instance));

        tcp_server tcp_server_instance(io_service);
        udp_admin_server_instance.reset(new udp_admin_server(io_service,
                                                             sea_instance,
                                                             sea_static_instance,
                                                             seaport_instance,
                                                             shipyard_instance,
                                                             udp_server_instance,
                                                             session_instance,
                                                             city_instance));
        sea_instance->set_udp_admin_server(udp_admin_server_instance);
        udp_admin_server_instance->send_recover_all_ships();

        eval_lua_script_file(lua_state_instance.get(), "assets/l/run_tests.lua");

        LOGI("Starting to server IO service thread....");
        boost::thread thread(boost::bind(&boost::asio::io_service::run, &io_service));

        const char* argv[] = { "ss", "-i", "--", "assets/l/ss.lua" };
        lua_main(4, (char**)argv, custom_lua_init);

        thread.join();

    } catch (std::exception& e) {
        LOGE(e.what());
    }

    return 0;
}
