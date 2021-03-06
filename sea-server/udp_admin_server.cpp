#include "precompiled.hpp"
#include "udp_admin_server.hpp"
#include "sea.hpp"
#include "lz4.h"
#include "sea_static.hpp"
#include "xy.hpp"
#include "seaport.hpp"
#include "udp_server.hpp"
#include "packet.h"
#include "shipyard.hpp"
#include "adminmessage.h"
#include "session.hpp"
#include "city.hpp"

using namespace ss;

extern int g_production;

udp_admin_server::udp_admin_server(boost::asio::io_service& io_service,
                                   std::shared_ptr<sea> sea,
                                   std::shared_ptr<sea_static> sea_static,
                                   std::shared_ptr<seaport> seaport,
                                   std::shared_ptr<shipyard> shipyard,
                                   std::shared_ptr<udp_server> udp_server,
                                   std::shared_ptr<session> session,
                                   std::shared_ptr<city> city)
    : socket_(io_service, g_production ? udp::endpoint(udp::v4(), 4000) : udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 4000))
    , sea_(sea)
    , sea_static_(sea_static)
    , seaport_(seaport)
    , shipyard_(shipyard)
    , city_(city)
    , udp_server_(udp_server)
    , session_(session)
    , resolver_(io_service)
    , web_server_query_(udp::v4(), "localhost", "3003")
    , web_server_endpoint_(*resolver_.resolve(web_server_query_))
    , web_server_socket_(io_service) {
    start_receive();
    web_server_socket_.open(udp::v4());
}

void udp_admin_server::start_receive() {
    socket_.async_receive_from(boost::asio::buffer(recv_buffer_),
                               remote_endpoint_,
                               boost::bind(&udp_admin_server::handle_receive,
                                           this,
                                           boost::asio::placeholders::error,
                                           boost::asio::placeholders::bytes_transferred));
}

void udp_admin_server::handle_send(const boost::system::error_code & error, std::size_t bytes_transferred) {
    if (error) {
        LOGE("ERROR: %s", error);
    } else {
        LOGIx("%1% bytes_transferred", bytes_transferred);
    }
}

int udp_admin_server::process_command(const unsigned char* data, bool send_reply) {
    const command* cp = reinterpret_cast<const command*>(data);
    int ret = -1;
    switch (cp->type) {
    case 1: // Spawn
    {
        LOGEP("Not supported admin command!");
        abort();
    }
    case 2: // Travel To
    {
        LOGEP("Not supported admin command!");
        abort();
    }
    case 3: // Telport To
    {
        LOGEP("Not supported admin command!");
        abort();
    }
    case 4: // Spawn Ship
    {
        LOGIx("Spawn Ship type: %1%", static_cast<int>(cp->type));
        auto spawn = reinterpret_cast<const spawn_ship_command*>(data);
        int reply_id = spawn->reply_id;
        int expect_land = spawn->expect_land;
        int id = sea_->spawn(*spawn);
        udp_server_->gold_used(static_cast<int>(spawn->x), static_cast<int>(spawn->y), 1000);
        int id1 = spawn->port1_id;
        int id2 = spawn->port2_id;
        if (send_reply) {
            spawn_ship_command_reply reply;
            memset(&reply, 0, sizeof(spawn_ship_command_reply));
            reply._.type = 1;
            reply.reply_id = reply_id;
            reply.db_id = id;
            reply.port1_id = id1;
            reply.port2_id = id2;
            if (id1 > 0 && id2 > 0) {
                reply.routed = udp_server_->set_route(id, id1, id2, expect_land, std::shared_ptr<astarrtree::coro_context>());
            } else {
                LOGEP("Route endpoints not specified! port1_id=%1%, port2_id=%2%", id1, id2);
            }
            socket_.async_send_to(boost::asio::buffer(&reply, sizeof(spawn_ship_command_reply)), remote_endpoint_,
                                  boost::bind(&udp_admin_server::handle_send, this,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
        }
        break;
    }
    case 5: // Delete Ship
    {
        LOGIx("Delete Ship type: %1%", static_cast<int>(cp->type));
        auto spawn = reinterpret_cast<const delete_ship_command*>(data);
        sea_->despawn(spawn->ship_id);
        break;
    }
    case 6: // Spawn Port
    {
        LOGI("Spawn Port type: %1%", static_cast<int>(cp->type));
        auto spawn = reinterpret_cast<const spawn_port_command*>(data);
        xy32 spawn_pos = { spawn->xc, spawn->yc };
        spawn_port_command_reply reply;
        memset(&reply, 0, sizeof(spawn_port_command_reply));
        reply._.type = 4;
        reply.reply_id = spawn->reply_id;
        bool check_type = false;
        if (spawn->expect_land == 0) {
            // Seaport
            if (sea_static_->is_water(spawn_pos)) {
                check_type = true;
            }
        } else if (spawn->expect_land == 1) {
            // (Land?)port
            if (sea_static_->is_land(spawn_pos)) {
                check_type = true;
            }
        } else {
            LOGEP("Unknown spawn type: %||", spawn->expect_land);
        }
        if (check_type == false) {
            LOGEP("[1] check_type failure - spawn->expect_land=%||, but it is not.", spawn->expect_land);
        }
        bool check_distance = false;
        if (check_type) {
            const auto& nid = seaport_->query_nearest(spawn_pos.x, spawn_pos.y);
            if (nid.size() > 0) {
                const auto nearest_x = nid[0].first.get<0>();
                const auto nearest_y = nid[0].first.get<1>();
                if (nearest_x == spawn_pos.x && nearest_y == spawn_pos.y) {
                    // existing seaport should be returned as success
                    check_distance = true;
                } else {
                    check_distance = abs(nearest_x - spawn_pos.x) >= 3 || abs(nearest_y - spawn_pos.y) >= 3;
                }
            } else {
                check_distance = true;
            }
        }
        if (check_distance == false) {
            LOGEP("[2] check_distance failure - minimum distance between existing seaports not satisfied.");
        }
        if (check_type && check_distance) {
            bool existing = false;
            int id = seaport_->spawn(spawn->expected_db_id, spawn->name, spawn->xc, spawn->yc, spawn->owner_id, existing, spawn->expect_land ? seaport::seaport_type::LAND : seaport::seaport_type::SEA);
            if (existing == false && id > 0) {
                udp_server_->gold_used(spawn->xc, spawn->yc, 10000);
            }
            reply.db_id = id;
            if (id > 0) {
                if (existing) {
                    LOGI("Seaport SP %1% {%2%,%3%} already exists. owner_id=%4%",
                         id,
                         spawn_pos.x,
                         spawn_pos.y,
                         spawn->owner_id);
                } else {
                    LOGI("New seaport SP %1% {%2%,%3%} spawned. owner_id=%4%",
                         id,
                         spawn_pos.x,
                         spawn_pos.y,
                         spawn->owner_id);
                    ret = 0;
                }
                reply.existing = existing;
            } else {
                LOGEP("New port cannot be spawned at (x=%1%, y=%2%)",
                      spawn_pos.x,
                      spawn_pos.y);
            }
        }
        reply.too_close = check_distance ? 0 : 1;
        if (send_reply) {
            socket_.async_send_to(boost::asio::buffer(&reply,
                                                      sizeof(spawn_port_command_reply)),
                                  remote_endpoint_,
                                  boost::bind(&udp_admin_server::handle_send, this,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
        }
        break;
    }
    case 7: // Name Port
    {
        LOGEP("Not supported admin command!");
        break;
    }
    case 8: // Delete Port
    {
        LOGIx("Delete Port type: %1%", static_cast<int>(cp->type));
        auto spawn = reinterpret_cast<const delete_port_command*>(data);
        ret = seaport_->despawn(spawn->port_id);
        break;
    }
    case 9: // Spawn Shipyard
    {
        LOGI("Spawn Shipyard type: %1%", static_cast<int>(cp->type));
        auto spawn = reinterpret_cast<const spawn_shipyard_command*>(data);
        xy32 spawn_pos = { spawn->xc, spawn->yc };
        spawn_shipyard_command_reply reply;
        memset(&reply, 0, sizeof(spawn_shipyard_command_reply));
        reply._.type = 5;
        reply.reply_id = spawn->reply_id;
        bool check_type = false;
        if (sea_static_->is_water(spawn_pos)) {
            check_type = true;
        }
        if (check_type == false) {
            LOGEP("[1] spawn shipyard check_type failure");
        }
        bool check_distance = false;
        if (check_type) {
            const auto& nid = seaport_->query_nearest(spawn_pos.x, spawn_pos.y);
            if (nid.size() > 0) {
                const auto nearest_x = nid[0].first.get<0>();
                const auto nearest_y = nid[0].first.get<1>();
                if (nearest_x == spawn_pos.x && nearest_y == spawn_pos.y) {
                    // existing seaport should be returned as success
                    check_distance = true;
                } else {
                    check_distance = abs(nearest_x - spawn_pos.x) >= 3 || abs(nearest_y - spawn_pos.y) >= 3;
                }
            } else {
                check_distance = true;
            }
        }
        if (check_distance == false) {
            LOGEP("[2] spawn shipyard check_distance failure - minimum distance between existing seaports not satisfied.");
        }
        if (check_type && check_distance) {
            bool existing = false;
            int id = shipyard_->spawn(spawn->expected_db_id, spawn->name, spawn->xc, spawn->yc, spawn->owner_id, 0, existing);
            if (existing == false) {
                udp_server_->gold_used(spawn->xc, spawn->yc, 50000);
            }
            reply.db_id = id;
            if (id > 0) {
                if (existing) {
                    LOGI("Shipyard SP %1% {%2%,%3%} already exists. owner_id=%4%",
                         id,
                         spawn_pos.x,
                         spawn_pos.y,
                         spawn->owner_id);
                } else {
                    LOGI("New shipyard SP %1% {%2%,%3%} spawned. owner_id=%4%",
                         id,
                         spawn_pos.x,
                         spawn_pos.y,
                         spawn->owner_id);
                    ret = 0;
                }
                reply.existing = existing;
            } else {
                LOGEP("New shipyard cannot be spawned at (x=%1%, y=%2%)",
                      spawn_pos.x,
                      spawn_pos.y);
            }
        }
        if (send_reply) {
            socket_.async_send_to(boost::asio::buffer(&reply,
                                                      sizeof(spawn_shipyard_command_reply)),
                                  remote_endpoint_,
                                  boost::bind(&udp_admin_server::handle_send, this,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
        }
        break;
    }
    case 10: // Delete Shipyard
    {
        LOGI("Delete Shipyard type: %1%", static_cast<int>(cp->type));
        auto spawn = reinterpret_cast<const delete_shipyard_command*>(data);
        shipyard_->despawn(spawn->shipyard_id);
        break;
    }
    case 11: // Query Nearest Shipyard For Ship
    {
        LOGI("Query Nearest Shipyard For Ship type: %1%", static_cast<int>(cp->type));
        auto cmd = reinterpret_cast<const query_nearest_shipyard_for_ship_command*>(data);
        const auto& ship = sea_->get_by_db_id(cmd->ship_id);
        int nearest_shipyard_id = -1;
        if (ship) {
            float fx, fy;
            ship->get_xy(fx, fy);
            nearest_shipyard_id = shipyard_->get_nearest(xy32{ static_cast<int>(fx), static_cast<int>(fy) });
        }
        if (send_reply) {
            query_nearest_shipyard_for_ship_command_reply reply;
            memset(&reply, 0, sizeof(query_nearest_shipyard_for_ship_command_reply));
            reply._.type = 6;
            reply.reply_id = cmd->reply_id;
            reply.ship_id = cmd->ship_id;
            reply.shipyard_id = nearest_shipyard_id;
            socket_.async_send_to(boost::asio::buffer(&reply,
                                                      sizeof(spawn_shipyard_command_reply)),
                                  remote_endpoint_,
                                  boost::bind(&udp_admin_server::handle_send, this,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
        }
        break;
    }
    case 12: // Register Shared Secret Session Key
    {
        LOGI("Registering Shared Secret Session Key type: %1%", static_cast<int>(cp->type));
        auto cmd = reinterpret_cast<const register_shared_secret_session_key_command*>(data);
        LOGI("  Account ID: %1%", cmd->account_id);
        session_->register_key(cmd->account_id, cmd->key_str, cmd->key_str_len);
        if (send_reply) {
            register_shared_secret_session_key_command_reply reply;
            memset(&reply, 0, sizeof(register_shared_secret_session_key_command_reply));
            reply._.type = 7;
            reply.reply_id = cmd->reply_id;
            socket_.async_send_to(boost::asio::buffer(&reply,
                                                      sizeof(register_shared_secret_session_key_command_reply)),
                                  remote_endpoint_,
                                  boost::bind(&udp_admin_server::handle_send, this,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
        }
        break;
    }
    case 13: // Spawn City
    {
        LOGI("Spawn City type: %1%", static_cast<int>(cp->type));
        auto spawn = reinterpret_cast<const spawn_city_command*>(data);
        xy32 spawn_pos = { spawn->xc, spawn->yc };
        spawn_city_command_reply reply;
        memset(&reply, 0, sizeof(spawn_city_command_reply));
        reply._.type = 8;
        reply.reply_id = spawn->reply_id;
        bool check_type = false;
        if (sea_static_->is_land(spawn_pos)) {
            check_type = true;
        }
        if (check_type == false) {
            LOGEP("[1] spawn city check_type failure: city can be spawned only at land cell");
        }
        bool check_distance = false;
        if (check_type) {
            const auto& nid = city_->query_nearest(spawn_pos.x, spawn_pos.y);
            if (nid.size() > 0) {
                const auto nearest_x = nid[0].first.get<0>();
                const auto nearest_y = nid[0].first.get<1>();
                if (nearest_x == spawn_pos.x && nearest_y == spawn_pos.y) {
                    // existing city should be returned as success
                    check_distance = true;
                } else {
                    check_distance = abs(nearest_x - spawn_pos.x) >= 3 || abs(nearest_y - spawn_pos.y) >= 3;
                }
            } else {
                check_distance = true;
            }
        }
        if (check_distance == false) {
            LOGEP("[2] spawn city check_distance failure - minimum distance between existing citys not satisfied.");
        }
        if (check_type && check_distance) {
            bool existing = false;
            int id = city_->spawn(spawn->name, spawn->xc, spawn->yc);
            if (existing == false && id > 0) {
                udp_server_->gold_used(spawn->xc, spawn->yc, 50000);
            }
            ret = id;
            reply.db_id = id;
            if (id > 0) {
                if (existing) {
                    LOGI("city SP %1% {%2%,%3%} already exists. owner_id=%4%",
                         id,
                         spawn_pos.x,
                         spawn_pos.y,
                         spawn->owner_id);
                } else {
                    LOGI("New city SP %1% {%2%,%3%} spawned. owner_id=%4%",
                         id,
                         spawn_pos.x,
                         spawn_pos.y,
                         spawn->owner_id);
                }
                reply.existing = existing;
            } else {
                LOGEP("New city cannot be spawned at (x=%1%, y=%2%)",
                      spawn_pos.x,
                      spawn_pos.y);
            }
        }
        if (send_reply) {
            socket_.async_send_to(boost::asio::buffer(&reply,
                                                      sizeof(spawn_city_command_reply)),
                                  remote_endpoint_,
                                  boost::bind(&udp_admin_server::handle_send, this,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
        }
        break;
    }
    case 14: // Set City Population
    {
        LOGI("Set City Population type: %1%", static_cast<int>(cp->type));
        auto spawn = reinterpret_cast<const set_city_population_command*>(data);
        ret = city_->set_population(spawn->id, spawn->population);
        break;
    }
    default:
    {
        LOGE("Unknown command packet type: %1%", static_cast<int>(cp->type));
        break;
    }
    }
    return ret;
}

void udp_admin_server::handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred) {
    if (!error || error == boost::asio::error::message_size) {
        process_command(reinterpret_cast<unsigned char*>(recv_buffer_.data()), true);
        start_receive();
    }
}

void udp_admin_server::send_recover_all_ships() {
    std::array<char, 1> send_buf = { {2} }; // RecoverAllShips
    web_server_socket_.send_to(boost::asio::buffer(send_buf), web_server_endpoint_);
}

void udp_admin_server::send_arrival(int ship_id) {
    struct {
        char type; char padding0; char padding1; char padding2;
        int ship_id;
    } p;
    p.type = 3;
    p.ship_id = ship_id;
    web_server_socket_.send_to(boost::asio::buffer(&p, sizeof(p)), web_server_endpoint_);
}
