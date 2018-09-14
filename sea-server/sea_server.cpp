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

extern "C" int lua_main(int argc, char **argv, void custom_init_func(lua_State* L));
extern "C" int handle_script(lua_State *L, char **argv);

static int traceback(lua_State *L) {
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 2);
    lua_call(L, 2, 1);
    return 1;
}

void init_lua(lua_State* L, const char* lua_filename) {
    luaL_openlibs(L);
    char prefixed_filename[256];
    prefixed_filename[0] = '\0';
    strncat(prefixed_filename, "@", sizeof(prefixed_filename) - 1);
    strncat(prefixed_filename, lua_filename, sizeof(prefixed_filename) - 1);
    std::ifstream script_file(lua_filename);
    std::stringstream script_stream;
    script_stream << script_file.rdbuf();
    lua_pushcfunction(L, traceback);
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

void custom_lua_init(lua_State* L) {
    SWIG_init(L);
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

int main(int argc, char* argv[]) {
    try {
        const int major = 0;
        const int minor = 1;
        const int patch = 0;
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



        std::shared_ptr<sea> sea_instance(new sea(io_service));
        std::shared_ptr<sea_static> sea_static_instance(new sea_static());
        std::shared_ptr<seaport> seaport_instance(new seaport(io_service));
        std::shared_ptr<region> region_instance(new region());
        std::shared_ptr<city> city_instance(new city(io_service,
                                                     seaport_instance));
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
                                                 contract_instance));

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
