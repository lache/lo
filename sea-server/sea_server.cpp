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
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-fpermissive"
#endif
#include "ss_wrap.inl"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
using namespace ss;

extern "C" int lua_main(int argc, char **argv, void custom_init_func(lua_State* L));
extern "C" int handle_script(lua_State *L, char **argv);

void custom_lua_init(lua_State* L) {
    SWIG_init(L);
}

boost::asio::io_service io_service;
std::shared_ptr<udp_admin_server> udp_admin_server_instance;
int post_admin_message(const unsigned char* b) {
    boost::promise<int> prom;
    io_service.post([b, &prom] {
        prom.set_value(udp_admin_server_instance->process_command(b, false));
    });
    return prom.get_future().get();
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
        std::shared_ptr<session> session_instance(new session());
        std::shared_ptr<contract> contract_instance(new contract(io_service,
                                                                 sea_static_instance));
        if (argc > 1 && strcmp(argv[1], "--prepare") == 0) {
            LOGI("Preparation is completed.");
            return 0;
        }
        std::shared_ptr<udp_server> udp_server_instance(new udp_server(io_service,
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
                                                         session_instance));
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
