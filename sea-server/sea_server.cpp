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
using namespace ss;

int main() {
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

        for (int i = 0; i <= 16; i++) {
            LOGI("C=%1% -> Aligned C=%2%",
                 i,
                 aligned_chunk_index(i, 1, 16));
        }

        boost::asio::io_service io_service;
        std::shared_ptr<sea> sea_instance(new sea(io_service));
        sea_instance->populate_test();
        std::shared_ptr<sea_static> sea_static_instance(new sea_static());
        std::shared_ptr<seaport> seaport_instance(new seaport(io_service));
        std::shared_ptr<region> region_instance(new region());
        std::shared_ptr<city> city_instance(new city(io_service,
                                                     seaport_instance));
        std::shared_ptr<salvage> salvage_instance(new salvage(io_service,
                                                              sea_static_instance));

        auto existing = false;
        for (int i = 0; i < 20; i++) {
            for (int j = 0; j < 20; j++) {
                salvage_instance->spawn("TEST", i, j, 1, existing);
            }
        }

        std::shared_ptr<udp_server> udp_server_instance(new udp_server(io_service,
                                                                       sea_instance,
                                                                       sea_static_instance,
                                                                       seaport_instance,
                                                                       region_instance,
                                                                       city_instance,
                                                                       salvage_instance));
        tcp_server tcp_server_instance(io_service);
        std::shared_ptr<udp_admin_server> udp_admin_server_instance(new udp_admin_server(io_service,
                                                                                         sea_instance,
                                                                                         sea_static_instance,
                                                                                         seaport_instance,
                                                                                         udp_server_instance));
        sea_instance->set_udp_admin_server(udp_admin_server_instance);
        udp_admin_server_instance->send_recover_all_ships();
        io_service.run();
    } catch (std::exception& e) {
        LOGE(e.what());
    }

    return 0;
}
