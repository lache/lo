#include "precompiled.hpp"
#include "udp_server.hpp"
#include "sea.hpp"
#include "sea_static.hpp"
#include "seaport.hpp"
#include "route.hpp"
#include "xy.hpp"
#include "packet.h"
#include "region.hpp"
#include "city.hpp"
#include "endpoint_aoi_object.hpp"
#include "salvage.hpp"
#include "shipyard.hpp"
#include "mbedtls/aes.h"
#include "session.hpp"
#include "contract.hpp"
#include "srp.hpp"

using namespace ss;

extern int g_production;

const auto update_interval = boost::posix_time::milliseconds(75);
//const auto update_interval = boost::posix_time::milliseconds(250);
const auto salvage_update_interval = boost::posix_time::milliseconds(30 * 60 * 1000);
const auto contract_update_interval = boost::posix_time::milliseconds(30 * 60 * 1000);

udp_server::udp_server(boost::asio::io_service& io_service,
                       std::shared_ptr<sea> sea,
                       std::shared_ptr<sea_static> sea_static,
                       std::shared_ptr<seaport> seaport,
                       std::shared_ptr<region> region,
                       std::shared_ptr<city> city,
                       std::shared_ptr<salvage> salvage,
                       std::shared_ptr<shipyard> shipyard,
                       std::shared_ptr<session> session,
                       std::shared_ptr<contract> contract,
                       std::shared_ptr<lua_State> lua_state_instance)
    : socket_(io_service, g_production ? udp::endpoint(udp::v4(), 3100) : udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 3100))
    , timer_(io_service, update_interval)
    , salvage_timer_(io_service, salvage_update_interval)
    , contract_timer_(io_service, contract_update_interval)
    , sea_(sea)
    , sea_static_(sea_static)
    , seaport_(seaport)
    , region_(region)
    , city_(city)
    , salvage_(salvage)
    , shipyard_(shipyard)
    , session_(session)
    , contract_(contract)
    , tick_seq_(0)
    , client_endpoint_aoi_int_key_(0)
    , gold_(0)
    , lua_state_instance_(lua_state_instance) {
    start_receive();
    timer_.async_wait(boost::bind(&udp_server::update, this));
    salvage_timer_.async_wait(boost::bind(&udp_server::salvage_update, this));
    contract_timer_.async_wait(boost::bind(&udp_server::contract_update, this));
}

bool udp_server::set_route(int id, int seaport_id1, int seaport_id2, int expect_land, std::shared_ptr<astarrtree::coro_context> coro) {
    auto route = create_route_id({ seaport_id1, seaport_id2 }, expect_land, coro);
    if (route) {
        route_map_[id] = route;
        return true;
    }
    return false;
}

void udp_server::update() {
    tick_seq_++;

    timer_.expires_at(timer_.expires_at() + update_interval);
    timer_.async_wait(boost::bind(&udp_server::update, this));

    float delta_time = update_interval.total_milliseconds() / 1000.0f;
    sea_->update(delta_time);
    std::vector<int> remove_keys;
    for (auto v : route_map_) {
        if (sea_->update_route(delta_time,
                               v.first,
                               v.second,
                               seaport_,
                               this) == false) {
            // no more valid 'v'
            remove_keys.push_back(v.first);
        }
    }
    for (auto v : remove_keys) {
        route_map_.erase(v);
    }
    remove_expired_endpoints();

    flush_cargo_notifications();
}

void udp_server::salvage_update() {
    salvage_timer_.expires_at(salvage_timer_.expires_at() + salvage_update_interval);
    salvage_timer_.async_wait(boost::bind(&udp_server::salvage_update, this));

    //float delta_time = salvage_update_interval.total_milliseconds() / 1000.0f;

    for (const auto& e : client_endpoint_aoi_values_) {
        const auto& aoi_box = e.second.first;
        auto salvage_existing = false;
        auto salvage_id = salvage_->spawn_random("Random", aoi_box, salvage_existing);
        LOGIx("Salvage spawned: ID=%1% (existing=%2%)", salvage_id, salvage_existing);
    }
}

void udp_server::contract_update() {
    contract_timer_.expires_at(contract_timer_.expires_at() + contract_update_interval);
    contract_timer_.async_wait(boost::bind(&udp_server::contract_update, this));

    //float delta_time = contract_update_interval.total_milliseconds() / 1000.0f;

    for (const auto& e : client_endpoint_aoi_values_) {
        const auto& aoi_box = e.second.first;
        auto contract_existing = false;
        auto contract_id = contract_->spawn_random("Random", aoi_box, contract_existing);
        LOGIx("Contract spawned: ID=%1% (existing=%2%)", contract_id, contract_existing);
    }
}

void udp_server::start_receive() {
    socket_.async_receive_from(boost::asio::buffer(recv_buffer_),
                               remote_endpoint_,
                               boost::bind(&udp_server::handle_receive,
                                           this,
                                           boost::asio::placeholders::error,
                                           boost::asio::placeholders::bytes_transferred));
}

void udp_server::handle_send(const boost::system::error_code & error, std::size_t bytes_transferred) {
    if (error) {
        LOGE("ERROR: %s", error);
    } else {
        LOGIx("%1% bytes sent.", bytes_transferred);
    }
}

void udp_server::send_route_state(float lng, float lat, float ex_lng, float ex_lat, int view_scale) {
    std::vector<sea_object> sop_list;
    sea_->query_near_lng_lat_to_packet(lng, lat, ex_lng * view_scale, ex_lat * view_scale, sop_list);
    std::shared_ptr<LWPTTLROUTESTATE> reply(new LWPTTLROUTESTATE);
    memset(reply.get(), 0, sizeof(LWPTTLROUTESTATE));
    reply->type = LPGP_LWPTTLROUTESTATE;
    size_t reply_obj_index = 0;
    for (sea_object const& v : sop_list) {
        auto& o = reply->obj[reply_obj_index];
        // ship info
        v.fill_packet(o);
        // route info
        auto it = route_map_.find(v.get_db_id());
        if (it != route_map_.end() && it->second) {
            o.route_param = it->second->get_param();
            o.route_speed = it->second->get_velocity();
            o.route_flags.reversed = it->second->get_reversed() ? 1 : 0;
            o.route_flags.no_route = 0;
            o.x = 0;
            o.y = 0;
        } else {
            o.route_param = 0;
            o.route_speed = 0;
            o.route_flags.reversed = 0;
            o.route_flags.no_route = 1;
            v.get_xy(o.x, o.y);
        }
        reply_obj_index++;
        if (reply_obj_index >= boost::size(reply->obj)) {
            break;
        }
    }
    reply->count = static_cast<int>(reply_obj_index);
    LOGIx("Querying (%1%,%2%) extent (%3%,%4%) => %5% hit(s).", lng, lat, ex_lng, ex_lat, reply_obj_index);
    char compressed[1500];
    int compressed_size = LZ4_compress_default((char*)reply.get(), compressed, sizeof(LWPTTLROUTESTATE), static_cast<int>(boost::size(compressed)));
    if (compressed_size > 0) {
        socket_.async_send_to(boost::asio::buffer(compressed, compressed_size),
                              remote_endpoint_,
                              boost::bind(&udp_server::handle_send,
                                          this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    } else {
        LOGEP("LZ4_compress_default() error! - %1%", compressed_size);
    }
}

static signed char aligned_scaled_offset(const int cell_index, const int aligned_cell_index, const int view_scale, const int view_scale_msb_index, const bool clamp, int lo, int hi) {
    const auto cell_index_scaled = cell_index & ~(view_scale - 1);
    const auto offset = cell_index_scaled - aligned_cell_index;
    try {
        return boost::numeric_cast<signed char>((clamp ? boost::algorithm::clamp(offset, lo, hi) : offset) >> view_scale_msb_index);
    } catch (const boost::numeric::negative_overflow& o) {
        LOGE(o.what());
    }
    return 0;
}

void udp_server::send_land_cell(float lng, float lat, float ex_lng, float ex_lat, int view_scale) {
    const auto xc0 = sea_static_->lng_to_xc(lng);
    const auto yc0 = sea_static_->lat_to_yc(lat);
    const auto xc0_aligned = aligned_chunk_index(xc0, view_scale, static_cast<int>(ex_lng));
    const auto yc0_aligned = aligned_chunk_index(yc0, view_scale, static_cast<int>(ex_lat));
    send_land_cell_aligned(xc0_aligned,
                           yc0_aligned,
                           ex_lng,
                           ex_lat,
                           view_scale);
}

void udp_server::send_land_cell_aligned(int xc0_aligned, int yc0_aligned, float ex_lng, float ex_lat, int view_scale) {
    const auto half_lng_cell_pixel_extent = boost::math::iround(ex_lng / 2.0f * view_scale);
    const auto half_lat_cell_pixel_extent = boost::math::iround(ex_lat / 2.0f * view_scale);
    auto sop_list = sea_static_->query_near_to_packet(xc0_aligned,
                                                      yc0_aligned,
                                                      ex_lng * view_scale,
                                                      ex_lat * view_scale);
    //auto sop_list = sea_static_->query_near_to_packet(xc0_aligned, yc0_aligned, xc1, yc1);


    const auto xclo = -half_lng_cell_pixel_extent;
    const auto xchi = +half_lng_cell_pixel_extent;
    const auto yclo = -half_lat_cell_pixel_extent;
    const auto ychi = +half_lat_cell_pixel_extent;
    std::shared_ptr<LWPTTLSTATICSTATE2> reply(new LWPTTLSTATICSTATE2);
    memset(reply.get(), 0, sizeof(LWPTTLSTATICSTATE2));
    reply->type = LPGP_LWPTTLSTATICSTATE2;
    reply->ts = sea_static_->query_ts(xc0_aligned, yc0_aligned, view_scale);
    reply->xc0 = xc0_aligned;
    reply->yc0 = yc0_aligned;
    reply->view_scale = view_scale;
    size_t reply_obj_index = 0;
    size_t reply_obj_dropped_count = 0;
    const int view_scale_msb_index = msb_index(view_scale);
    for (const auto& v : sop_list) {
        // x-axis

        //const auto vx0 = v.x0 &~(view_scale - 1);
        //const auto vx1 = v.x1 &~(view_scale - 1);
        const auto x_scaled_offset_0 = aligned_scaled_offset(v.x0, xc0_aligned, view_scale, view_scale_msb_index, true, xclo, xchi);// boost::numeric_cast<signed char>(boost::algorithm::clamp(vx0 - xc0_aligned, xclo, xchi) >> view_scale_msb_index);
        const auto x_scaled_offset_1 = aligned_scaled_offset(v.x1, xc0_aligned, view_scale, view_scale_msb_index, true, xclo, xchi);// boost::numeric_cast<signed char>(boost::algorithm::clamp(vx1 - xc0_aligned, xclo, xchi) >> view_scale_msb_index);
        // skip degenerated one
        if (x_scaled_offset_0 >= x_scaled_offset_1) {
            continue;
        }
        // y-axis
        //const auto vy0 = v.y0 &~(view_scale - 1);
        //const auto vy1 = v.y1 &~(view_scale - 1);
        const auto y_scaled_offset_0 = aligned_scaled_offset(v.y0, yc0_aligned, view_scale, view_scale_msb_index, true, yclo, ychi); //boost::numeric_cast<signed char>(boost::algorithm::clamp(vy0 - yc0_aligned, yclo, ychi) >> view_scale_msb_index);
        const auto y_scaled_offset_1 = aligned_scaled_offset(v.y1, yc0_aligned, view_scale, view_scale_msb_index, true, yclo, ychi); //boost::numeric_cast<signed char>(boost::algorithm::clamp(vy1 - yc0_aligned, yclo, ychi) >> view_scale_msb_index);
        // skip degenerated one
        if (y_scaled_offset_0 >= y_scaled_offset_1) {
            continue;
        }
        if (reply_obj_index < boost::size(reply->obj)) {
            reply->obj[reply_obj_index].x_scaled_offset_0 = x_scaled_offset_0;
            reply->obj[reply_obj_index].y_scaled_offset_0 = y_scaled_offset_0;
            reply->obj[reply_obj_index].x_scaled_offset_1 = x_scaled_offset_1;
            reply->obj[reply_obj_index].y_scaled_offset_1 = y_scaled_offset_1;
            reply_obj_index++;
        } else {
            reply_obj_dropped_count++;
        }
    }
    reply->count = static_cast<int>(reply_obj_index);
    LOGIx("Querying (%1%,%2%) extent (%3%,%4%) => %5% hit(s).", xc0_aligned, yc0_aligned, ex_lng * view_scale, ex_lng * view_scale, reply_obj_index);
    char compressed[1500];
    int compressed_size = LZ4_compress_default((char*)reply.get(), compressed, sizeof(LWPTTLSTATICSTATE2), static_cast<int>(boost::size(compressed)));
    if (compressed_size > 0) {
        boost::crc_32_type result;
        result.process_bytes(compressed, compressed_size);
        auto crc = result.checksum();
        LOGIx("CRC: %1%", crc);

        socket_.async_send_to(boost::asio::buffer(compressed, compressed_size),
                              remote_endpoint_,
                              boost::bind(&udp_server::handle_send,
                                          this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    } else {
        LOGEP("LZ4_compress_default() error! - %1%", compressed_size);
    }
    if (reply_obj_dropped_count) {
        LOGEP("%1% cells dropped. (max: %2%) Compressed size is %3% bytes.",
              reply_obj_dropped_count,
              boost::size(reply->obj),
              compressed_size);
    }
}

void udp_server::send_land_cell_aligned_bitmap(int xc0_aligned, int yc0_aligned, float ex_lng, float ex_lat, int view_scale) {

    ex_lng *= 8;
    ex_lat *= 8;

    const auto half_lng_cell_pixel_extent = boost::math::iround(ex_lng / 2.0f * view_scale);
    const auto half_lat_cell_pixel_extent = boost::math::iround(ex_lat / 2.0f * view_scale);
    auto sop_list = sea_static_->query_near_to_packet(xc0_aligned,
                                                      yc0_aligned,
                                                      ex_lng * view_scale,
                                                      ex_lat * view_scale);
    const auto xclo = -half_lng_cell_pixel_extent;
    const auto xchi = +half_lng_cell_pixel_extent;
    const auto yclo = -half_lat_cell_pixel_extent;
    const auto ychi = +half_lat_cell_pixel_extent;
    std::shared_ptr<LWPTTLSTATICSTATE3> reply(new LWPTTLSTATICSTATE3);
    memset(reply.get(), 0, sizeof(LWPTTLSTATICSTATE3));
    reply->type = LPGP_LWPTTLSTATICSTATE3;
    reply->ts = 1;
    reply->xc0 = xc0_aligned;
    reply->yc0 = yc0_aligned;
    reply->view_scale = view_scale;
    size_t reply_obj_index = 0;
    const int view_scale_msb_index = msb_index(view_scale);
    const auto half_lng_cell_pixel_extent_scaled = half_lng_cell_pixel_extent >> view_scale_msb_index;
    for (const auto& v : sop_list) {
        // x-axis
        const auto x_scaled_offset_0 = aligned_scaled_offset(v.x0, xc0_aligned, view_scale, view_scale_msb_index, true, xclo, xchi);
        const auto x_scaled_offset_1 = aligned_scaled_offset(v.x1, xc0_aligned, view_scale, view_scale_msb_index, true, xclo, xchi);
        // skip degenerated one
        if (x_scaled_offset_0 >= x_scaled_offset_1) {
            continue;
        }
        // y-axis
        const auto y_scaled_offset_0 = aligned_scaled_offset(v.y0, yc0_aligned, view_scale, view_scale_msb_index, true, yclo, ychi);
        const auto y_scaled_offset_1 = aligned_scaled_offset(v.y1, yc0_aligned, view_scale, view_scale_msb_index, true, yclo, ychi);
        // skip degenerated one
        if (y_scaled_offset_0 >= y_scaled_offset_1) {
            continue;
        }
        const auto x0 = half_lng_cell_pixel_extent_scaled + x_scaled_offset_0;
        const auto y0 = half_lng_cell_pixel_extent_scaled + y_scaled_offset_0;
        const auto x1 = half_lng_cell_pixel_extent_scaled + x_scaled_offset_1;
        const auto y1 = half_lng_cell_pixel_extent_scaled + y_scaled_offset_1;
        for (auto y = y0; y < y1; y++) {
            for (auto x = x0; x < x1; x++) {
                const auto xq = x / (sizeof(int) * 8);
                const auto xr = x % (sizeof(int) * 8);
                assert(y >= 0 && y < boost::size(reply->bitmap));
                assert(xq >= 0 && xq < boost::size(reply->bitmap[0]));
                reply->bitmap[y][xq] |= 1 << xr;
            }
        }
        reply_obj_index++;
    }
    LOGIx("Querying (%1%,%2%) extent (%3%,%4%) => %5% hit(s).", xc0_aligned, yc0_aligned, ex_lng * view_scale, ex_lng * view_scale, reply_obj_index);
    char compressed[1500];
    int compressed_size = LZ4_compress_default((char*)reply.get(), compressed, sizeof(LWPTTLSTATICSTATE3), static_cast<int>(boost::size(compressed)));
    if (compressed_size > 0) {
        boost::crc_32_type result;
        result.process_bytes(compressed, compressed_size);
        auto crc = result.checksum();
        LOGIx("CRC: %1%", crc);

        socket_.async_send_to(boost::asio::buffer(compressed, compressed_size),
                              remote_endpoint_,
                              boost::bind(&udp_server::handle_send,
                                          this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    } else {
        LOGEP("LZ4_compress_default() error! - %1%", compressed_size);
    }
}

void udp_server::send_track_object_coords(int track_object_id, int track_object_ship_id) {
    std::shared_ptr<sea_object> obj;
    if (track_object_id) {
        LOGEP("track_object_id not supported");
        return;
    }
    if (track_object_ship_id) {
        obj = sea_->get_by_db_id(track_object_ship_id);
    }
    if (!obj) {
        LOGEP("Tracking object cannot be found. (track_object_id=%1%, track_object_ship_id=%2%)",
              track_object_id,
              track_object_ship_id);
    }
    std::shared_ptr<LWPTTLTRACKCOORDS> reply(new LWPTTLTRACKCOORDS);
    memset(reply.get(), 0, sizeof(LWPTTLTRACKCOORDS));
    reply->type = LPGP_LWPTTLTRACKCOORDS;
    reply->id = obj ? (track_object_id ? track_object_id : track_object_ship_id ? track_object_ship_id : 0) : 0;
    if (obj) {
        obj->get_xy(reply->x, reply->y);
    }
    char compressed[1500];
    int compressed_size = LZ4_compress_default((char*)reply.get(), compressed, sizeof(LWPTTLTRACKCOORDS), static_cast<int>(boost::size(compressed)));
    if (compressed_size > 0) {
        socket_.async_send_to(boost::asio::buffer(compressed, compressed_size),
                              remote_endpoint_,
                              boost::bind(&udp_server::handle_send,
                                          this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    } else {
        LOGEP("LZ4_compress_default() error! - %1%", compressed_size);
    }
}

void udp_server::send_waypoints(int ship_id) {
    auto r = find_route_map_by_ship_id(ship_id);
    if (!r) {
        //LOGE("Ship id %1%'s route cannot be found.", ship_id);
        //return;
    }
    std::shared_ptr<LWPTTLWAYPOINTS> reply(new LWPTTLWAYPOINTS);
    memset(reply.get(), 0, sizeof(LWPTTLWAYPOINTS));
    reply->type = LPGP_LWPTTLWAYPOINTS;
    reply->ship_id = ship_id;
    if (r) {
        reply->flags.land = r->get_land() ? 1 : 0;
        auto waypoints = r->clone_waypoints();
        auto copy_len = std::min(waypoints.size(), boost::size(reply->waypoints));
        reply->count = boost::numeric_cast<int>(copy_len);
        for (size_t i = 0; i < copy_len; i++) {
            reply->waypoints[i].x = waypoints[i].x;
            reply->waypoints[i].y = waypoints[i].y;
        }
    } else {
        reply->count = 0; // 1
        /*auto ship = sea_->get_by_db_id(ship_id);
        float x, y;
        ship->get_xy(x, y);
        reply->waypoints[0].x = static_cast<int>(x);
        reply->waypoints[0].y = static_cast<int>(y);*/
    }
    // send
    char compressed[1500 * 4];
    int compressed_size = LZ4_compress_default((char*)reply.get(), compressed, sizeof(LWPTTLWAYPOINTS), static_cast<int>(boost::size(compressed)));
    if (compressed_size > 0) {
        socket_.async_send_to(boost::asio::buffer(compressed, compressed_size),
                              remote_endpoint_,
                              boost::bind(&udp_server::handle_send,
                                          this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    } else {
        LOGEP("LZ4_compress_default() error! - %1%", compressed_size);
    }
}

void udp_server::send_seaport(float lng, float lat, float ex_lng, float ex_lat, int view_scale) {
    const auto xc0 = sea_static_->lng_to_xc(lng);
    const auto yc0 = sea_static_->lat_to_yc(lat);
    const auto xc0_aligned = aligned_chunk_index(xc0, view_scale, static_cast<int>(ex_lng));
    const auto yc0_aligned = aligned_chunk_index(yc0, view_scale, static_cast<int>(ex_lat));
    send_seaport_cell_aligned(xc0_aligned,
                              yc0_aligned,
                              ex_lng,
                              ex_lat,
                              view_scale);
}

void udp_server::send_seaport_cell_aligned(int xc0_aligned, int yc0_aligned, float ex_lng, float ex_lat, int view_scale) {
    //const auto half_lng_cell_pixel_extent = boost::math::iround(ex_lng / 2.0f * view_scale);
    //const auto half_lat_cell_pixel_extent = boost::math::iround(ex_lat / 2.0f * view_scale);
    auto sop_list = seaport_->query_near_to_packet(xc0_aligned,
                                                   yc0_aligned,
                                                   ex_lng * view_scale,
                                                   ex_lat * view_scale);
    std::shared_ptr<LWPTTLSEAPORTSTATE> reply(new LWPTTLSEAPORTSTATE);
    memset(reply.get(), 0, sizeof(LWPTTLSEAPORTSTATE));
    reply->type = LPGP_LWPTTLSEAPORTSTATE;
    reply->ts = seaport_->query_ts(xc0_aligned, yc0_aligned, view_scale);
    reply->xc0 = xc0_aligned;
    reply->yc0 = yc0_aligned;
    reply->view_scale = view_scale;
    size_t reply_obj_index = 0;
    const int view_scale_msb_index = msb_index(view_scale);
    for (seaport_object const& v : sop_list) {
        reply->obj[reply_obj_index].x_scaled_offset_0 = aligned_scaled_offset(v.x0, xc0_aligned, view_scale, view_scale_msb_index, false, 0, 0);
        reply->obj[reply_obj_index].y_scaled_offset_0 = aligned_scaled_offset(v.y0, yc0_aligned, view_scale, view_scale_msb_index, false, 0, 0);
        reply->obj[reply_obj_index].flags.land = v.port_type;
        if (view_scale < 16) {
            //strcpy(reply->obj[reply_obj_index].name, seaport_->get_seaport_name(v.id));
        }
        reply_obj_index++;
        if (reply_obj_index >= boost::size(reply->obj)) {
            break;
        }
    }
    reply->count = static_cast<int>(reply_obj_index);
    char compressed[1500];
    int compressed_size = LZ4_compress_default((char*)reply.get(), compressed, sizeof(LWPTTLSEAPORTSTATE), static_cast<int>(boost::size(compressed)));
    if (compressed_size > 0) {
        socket_.async_send_to(boost::asio::buffer(compressed, compressed_size),
                              remote_endpoint_,
                              boost::bind(&udp_server::handle_send,
                                          this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    } else {
        LOGEP("LZ4_compress_default() error! - %1%", compressed_size);
    }
}

void udp_server::send_city_cell_aligned(int xc0_aligned, int yc0_aligned, float ex_lng, float ex_lat, int view_scale) {
    //const auto half_lng_cell_pixel_extent = boost::math::iround(ex_lng / 2.0f * view_scale);
    //const auto half_lat_cell_pixel_extent = boost::math::iround(ex_lat / 2.0f * view_scale);
    auto sop_list = city_->query_near_to_packet(xc0_aligned,
                                                yc0_aligned,
                                                ex_lng * view_scale,
                                                ex_lat * view_scale);
    std::shared_ptr<LWPTTLCITYSTATE> reply(new LWPTTLCITYSTATE);
    memset(reply.get(), 0, sizeof(LWPTTLCITYSTATE));
    reply->type = LPGP_LWPTTLCITYSTATE;
    reply->ts = city_->query_ts(xc0_aligned, yc0_aligned, view_scale);
    reply->xc0 = xc0_aligned;
    reply->yc0 = yc0_aligned;
    reply->view_scale = view_scale;
    size_t reply_obj_index = 0;
    const int view_scale_msb_index = msb_index(view_scale);
    for (city_object const& v : sop_list) {
        reply->obj[reply_obj_index].x_scaled_offset_0 = aligned_scaled_offset(v.x0, xc0_aligned, view_scale, view_scale_msb_index, false, 0, 0);
        reply->obj[reply_obj_index].y_scaled_offset_0 = aligned_scaled_offset(v.y0, yc0_aligned, view_scale, view_scale_msb_index, false, 0, 0);
        reply->obj[reply_obj_index].population_level = boost::numeric_cast<unsigned char>(v.population >> 17);
        reply_obj_index++;
        if (reply_obj_index >= boost::size(reply->obj)) {
            break;
        }
    }
    reply->count = static_cast<int>(reply_obj_index);
    char compressed[1500];
    int compressed_size = LZ4_compress_default((char*)reply.get(), compressed, sizeof(LWPTTLCITYSTATE), static_cast<int>(boost::size(compressed)));
    if (compressed_size > 0) {
        socket_.async_send_to(boost::asio::buffer(compressed, compressed_size),
                              remote_endpoint_,
                              boost::bind(&udp_server::handle_send,
                                          this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    } else {
        LOGEP("LZ4_compress_default() error! - %1%", compressed_size);
    }
}

void udp_server::send_salvage_cell_aligned(int xc0_aligned, int yc0_aligned, float ex_lng, float ex_lat, int view_scale) {
    //const auto half_lng_cell_pixel_extent = boost::math::iround(ex_lng / 2.0f * view_scale);
    //const auto half_lat_cell_pixel_extent = boost::math::iround(ex_lat / 2.0f * view_scale);
    auto sop_list = salvage_->query_near_to_packet(xc0_aligned,
                                                   yc0_aligned,
                                                   ex_lng * view_scale,
                                                   ex_lat * view_scale);
    std::shared_ptr<LWPTTLSALVAGESTATE> reply(new LWPTTLSALVAGESTATE);
    memset(reply.get(), 0, sizeof(LWPTTLSALVAGESTATE));
    reply->type = LPGP_LWPTTLSALVAGESTATE;
    reply->ts = salvage_->query_ts(xc0_aligned, yc0_aligned, view_scale);
    reply->xc0 = xc0_aligned;
    reply->yc0 = yc0_aligned;
    reply->view_scale = view_scale;
    size_t reply_obj_index = 0;
    const int view_scale_msb_index = msb_index(view_scale);
    for (const auto& v : sop_list) {
        reply->obj[reply_obj_index].x_scaled_offset_0 = aligned_scaled_offset(v.x0, xc0_aligned, view_scale, view_scale_msb_index, false, 0, 0);
        reply->obj[reply_obj_index].y_scaled_offset_0 = aligned_scaled_offset(v.y0, yc0_aligned, view_scale, view_scale_msb_index, false, 0, 0);
        reply_obj_index++;
        if (reply_obj_index >= boost::size(reply->obj)) {
            break;
        }
    }
    reply->count = static_cast<int>(reply_obj_index);
    if (reply->count < sop_list.size()) {
        LOGEP("packet truncated; capacity %1%, actual %2%", reply->count, sop_list.size());
    }
    char compressed[1500];
    int compressed_size = LZ4_compress_default((char*)reply.get(), compressed, sizeof(LWPTTLSALVAGESTATE), static_cast<int>(boost::size(compressed)));
    if (compressed_size > 0) {
        socket_.async_send_to(boost::asio::buffer(compressed, compressed_size),
                              remote_endpoint_,
                              boost::bind(&udp_server::handle_send,
                                          this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    } else {
        LOGEP("LZ4_compress_default() error! - %1%", compressed_size);
    }
}

void udp_server::send_contract_cell_aligned(int xc0_aligned, int yc0_aligned, float ex_lng, float ex_lat, int view_scale) {
    //const auto half_lng_cell_pixel_extent = boost::math::iround(ex_lng / 2.0f * view_scale);
    //const auto half_lat_cell_pixel_extent = boost::math::iround(ex_lat / 2.0f * view_scale);
    auto sop_list = contract_->query_near_to_packet(xc0_aligned,
                                                    yc0_aligned,
                                                    ex_lng * view_scale,
                                                    ex_lat * view_scale);
    std::shared_ptr<LWPTTLCONTRACTSTATE> reply(new LWPTTLCONTRACTSTATE);
    memset(reply.get(), 0, sizeof(LWPTTLCONTRACTSTATE));
    reply->type = LPGP_LWPTTLCONTRACTSTATE;
    reply->ts = contract_->query_ts(xc0_aligned, yc0_aligned, view_scale);
    reply->xc0 = xc0_aligned;
    reply->yc0 = yc0_aligned;
    reply->view_scale = view_scale;
    size_t reply_obj_index = 0;
    const int view_scale_msb_index = msb_index(view_scale);
    for (const auto& v : sop_list) {
        reply->obj[reply_obj_index].x_scaled_offset_0 = aligned_scaled_offset(v.x0, xc0_aligned, view_scale, view_scale_msb_index, false, 0, 0);
        reply->obj[reply_obj_index].y_scaled_offset_0 = aligned_scaled_offset(v.y0, yc0_aligned, view_scale, view_scale_msb_index, false, 0, 0);
        reply_obj_index++;
        if (reply_obj_index >= boost::size(reply->obj)) {
            break;
        }
    }
    reply->count = static_cast<int>(reply_obj_index);
    if (reply->count < sop_list.size()) {
        LOGEP("packet truncated; capacity %1%, actual %2%", reply->count, sop_list.size());
    }
    char compressed[1500];
    int compressed_size = LZ4_compress_default((char*)reply.get(), compressed, sizeof(LWPTTLCONTRACTSTATE), static_cast<int>(boost::size(compressed)));
    if (compressed_size > 0) {
        socket_.async_send_to(boost::asio::buffer(compressed, compressed_size),
                              remote_endpoint_,
                              boost::bind(&udp_server::handle_send,
                                          this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    } else {
        LOGEP("LZ4_compress_default() error! - %1%", compressed_size);
    }
}

void udp_server::send_shipyard_cell_aligned(int xc0_aligned, int yc0_aligned, float ex_lng, float ex_lat, int view_scale) {
    //const auto half_lng_cell_pixel_extent = boost::math::iround(ex_lng / 2.0f * view_scale);
    //const auto half_lat_cell_pixel_extent = boost::math::iround(ex_lat / 2.0f * view_scale);
    auto sop_list = shipyard_->query_near_to_packet(xc0_aligned,
                                                    yc0_aligned,
                                                    ex_lng * view_scale,
                                                    ex_lat * view_scale);
    std::shared_ptr<LWPTTLSHIPYARDSTATE> reply(new LWPTTLSHIPYARDSTATE);
    memset(reply.get(), 0, sizeof(LWPTTLSHIPYARDSTATE));
    reply->type = LPGP_LWPTTLSHIPYARDSTATE;
    reply->ts = shipyard_->query_ts(xc0_aligned, yc0_aligned, view_scale);
    reply->xc0 = xc0_aligned;
    reply->yc0 = yc0_aligned;
    reply->view_scale = view_scale;
    size_t reply_obj_index = 0;
    const int view_scale_msb_index = msb_index(view_scale);
    for (const auto& v : sop_list) {
        reply->obj[reply_obj_index].x_scaled_offset_0 = aligned_scaled_offset(v.x0, xc0_aligned, view_scale, view_scale_msb_index, false, 0, 0);
        reply->obj[reply_obj_index].y_scaled_offset_0 = aligned_scaled_offset(v.y0, yc0_aligned, view_scale, view_scale_msb_index, false, 0, 0);
        reply_obj_index++;
        if (reply_obj_index >= boost::size(reply->obj)) {
            break;
        }
    }
    reply->count = static_cast<int>(reply_obj_index);
    if (reply->count < sop_list.size()) {
        LOGEP("packet truncated; capacity %1%, actual %2%", reply->count, sop_list.size());
    }
    char compressed[1500];
    int compressed_size = LZ4_compress_default((char*)reply.get(), compressed, sizeof(LWPTTLSHIPYARDSTATE), static_cast<int>(boost::size(compressed)));
    if (compressed_size > 0) {
        socket_.async_send_to(boost::asio::buffer(compressed, compressed_size),
                              remote_endpoint_,
                              boost::bind(&udp_server::handle_send,
                                          this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    } else {
        LOGEP("LZ4_compress_default() error! - %1%", compressed_size);
    }
}

void udp_server::send_seaarea(float lng, float lat) {
    std::string area_name;
    region_->query_tree(lng, lat, area_name);

    std::shared_ptr<LWPTTLSEAAREA> reply(new LWPTTLSEAAREA);
    memset(reply.get(), 0, sizeof(LWPTTLSEAAREA));
    reply->type = LPGP_LWPTTLSEAAREA;
    strcpy(reply->name, area_name.c_str());
    char compressed[1500];
    int compressed_size = LZ4_compress_default((char*)reply.get(), compressed, sizeof(LWPTTLSEAAREA), static_cast<int>(boost::size(compressed)));
    if (compressed_size > 0) {
        socket_.async_send_to(boost::asio::buffer(compressed, compressed_size),
                              remote_endpoint_,
                              boost::bind(&udp_server::handle_send,
                                          this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    } else {
        LOGE("send_seaarea: LZ4_compress_default() error! - %1%", compressed_size);
    }
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

static int round_up(int num_to_round, int multiple) {
    if (multiple == 0)
        return num_to_round;

    int remainder = num_to_round % multiple;
    if (remainder == 0)
        return num_to_round;

    return num_to_round + multiple - remainder;
}

void udp_server::handle_ping() {
    LOGIx("PING received.");
    auto p = reinterpret_cast<LWPTTLPING*>(recv_buffer_.data());
    // routed ships
    send_route_state(p->lng, p->lat, p->ex_lng, p->ex_lat, p->view_scale);
    // area titles
    send_seaarea(p->lng, p->lat);
    // tracking info
    if (p->track_object_id || p->track_object_ship_id) {
        send_track_object_coords(p->track_object_id, p->track_object_ship_id);
    }
    // stat
    send_stat();
    // register(or refresh) endpoint and AOI
    auto xc = static_cast<int>(sea_->lng_to_xc(p->lng));
    auto yc = static_cast<int>(sea_->lat_to_yc(p->lat));
    auto ex_lng_scaled = static_cast<int>(p->ex_lng * p->view_scale);
    auto ex_lat_scaled = static_cast<int>(p->ex_lat * p->view_scale);
    endpoint_aoi_object::box aoi_box{
        {
            xc - ex_lng_scaled / 2,
            yc - ex_lat_scaled / 2
        }, {
            xc + ex_lng_scaled / 2 - 1,
            yc + ex_lat_scaled / 2 - 1
        }
    };
    register_client_endpoint(remote_endpoint_, aoi_box);
}

void udp_server::handle_request_waypoints() {
    LOGIx("REQUESTWAYPOINTS received.");
    auto p = reinterpret_cast<LWPTTLREQUESTWAYPOINTS*>(recv_buffer_.data());
    send_waypoints(p->ship_id);
    LOGIx("REQUESTWAYPOINTS replied with WAYPOINTS.");
}

void udp_server::handle_ping_flush() {
    LOGI("PINGFLUSH received.");
}

void udp_server::handle_ping_chunk() {
    LOGIx("PINGCHUNK received.");
    auto p = reinterpret_cast<LWPTTLPINGCHUNK*>(recv_buffer_.data());
    LWTTLCHUNKKEY chunk_key;
    chunk_key.v = p->chunk_key;
    const int clamped_view_scale = boost::algorithm::clamp(1 << chunk_key.bf.view_scale_msb, 1 << 0, 1 << 6);
    const int xc0_aligned = chunk_key.bf.xcc0 << msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * clamped_view_scale);
    const int yc0_aligned = chunk_key.bf.ycc0 << msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * clamped_view_scale);
    const float ex_lng = LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS;
    const float ex_lat = LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS;
    if (p->static_object == LTSOT_LAND_CELL) {
        auto ts = sea_static_->query_ts(chunk_key);
        if (ts == 0) {
            // In this case, client requested 'first' query on 'empty' chunk
            // since server startup. We should update this chunk timestamp properly
            // to invalidate client cache data if needed.
            const auto monotonic_uptime = get_monotonic_uptime();
            sea_static_->update_single_chunk_key_ts(chunk_key, monotonic_uptime);
            ts = monotonic_uptime;
        }
        if (ts > p->ts) {
            send_land_cell_aligned(xc0_aligned, yc0_aligned, ex_lng, ex_lat, clamped_view_scale);
            LOGIx("Land cells chunk key (%1%,%2%,%3%) Sent! (latest ts %4%)",
                  static_cast<int>(chunk_key.bf.xcc0),
                  static_cast<int>(chunk_key.bf.ycc0),
                  static_cast<int>(chunk_key.bf.view_scale_msb),
                  ts);
        } else {
            LOGIx("Land cells chunk key (%1%,%2%,%3%) Not Sent! (latest ts %4%)",
                  static_cast<int>(chunk_key.bf.xcc0),
                  static_cast<int>(chunk_key.bf.ycc0),
                  static_cast<int>(chunk_key.bf.view_scale_msb),
                  ts);
        }
    } else if (p->static_object == LTSOT_SEAPORT) {
        auto ts = seaport_->query_ts(chunk_key);
        if (ts == 0) {
            // In this case, client requested 'first' query on 'empty' chunk
            // since server startup. We should update this chunk timestamp properly
            // to invalidate client cache data if needed.
            const auto monotonic_uptime = get_monotonic_uptime();
            seaport_->update_single_chunk_key_ts(chunk_key, monotonic_uptime);
            ts = monotonic_uptime;
        }
        if (ts > p->ts) {
            send_seaport_cell_aligned(xc0_aligned, yc0_aligned, ex_lng, ex_lat, clamped_view_scale);
            LOGIx("Seaports chunk key (%1%,%2%,%3%) Sent! (latest ts %4%)",
                  static_cast<int>(chunk_key.bf.xcc0),
                  static_cast<int>(chunk_key.bf.ycc0),
                  static_cast<int>(chunk_key.bf.view_scale_msb),
                  ts);
        } else {
            LOGIx("Seaports chunk key (%1%,%2%,%3%) Not Sent! (latest ts %4%)",
                  static_cast<int>(chunk_key.bf.xcc0),
                  static_cast<int>(chunk_key.bf.ycc0),
                  static_cast<int>(chunk_key.bf.view_scale_msb),
                  ts);
        }
    } else if (p->static_object == LTSOT_CITY) {
        const auto ts = city_->query_ts(chunk_key);
        if (ts > p->ts) {
            send_city_cell_aligned(xc0_aligned, yc0_aligned, ex_lng, ex_lat, clamped_view_scale);
            LOGIx("Cities chunk key (%1%,%2%,%3%) Sent! (latest ts %4%)",
                  static_cast<int>(chunk_key.bf.xcc0),
                  static_cast<int>(chunk_key.bf.ycc0),
                  static_cast<int>(chunk_key.bf.view_scale_msb),
                  ts);
        } else {
            LOGIx("Cities chunk key (%1%,%2%,%3%) Not Sent! (latest ts %4%)",
                  static_cast<int>(chunk_key.bf.xcc0),
                  static_cast<int>(chunk_key.bf.ycc0),
                  static_cast<int>(chunk_key.bf.view_scale_msb),
                  ts);
        }
    } else if (p->static_object == LTSOT_SALVAGE) {
        const auto ts = salvage_->query_ts(chunk_key);
        if (ts > p->ts) {
            send_salvage_cell_aligned(xc0_aligned, yc0_aligned, ex_lng, ex_lat, clamped_view_scale);
            LOGIx("Salvages chunk key (%1%,%2%,%3%) Sent! (server ts %4%, client ts %5%)",
                  static_cast<int>(chunk_key.bf.xcc0),
                  static_cast<int>(chunk_key.bf.ycc0),
                  static_cast<int>(chunk_key.bf.view_scale_msb),
                  ts,
                  p->ts);
        } else {
            LOGIx("Salvages chunk key (%1%,%2%,%3%) Not Sent! (server ts %4%, client ts %5%)",
                  static_cast<int>(chunk_key.bf.xcc0),
                  static_cast<int>(chunk_key.bf.ycc0),
                  static_cast<int>(chunk_key.bf.view_scale_msb),
                  ts,
                  p->ts);
        }
    } else if (p->static_object == LTSOT_CONTRACT) {
        const auto ts = contract_->query_ts(chunk_key);
        if (ts > p->ts) {
            send_contract_cell_aligned(xc0_aligned, yc0_aligned, ex_lng, ex_lat, clamped_view_scale);
            LOGIx("Contracts chunk key (%1%,%2%,%3%) Sent! (server ts %4%, client ts %5%)",
                  static_cast<int>(chunk_key.bf.xcc0),
                  static_cast<int>(chunk_key.bf.ycc0),
                  static_cast<int>(chunk_key.bf.view_scale_msb),
                  ts,
                  p->ts);
        } else {
            LOGIx("Contracts chunk key (%1%,%2%,%3%) Not Sent! (server ts %4%, client ts %5%)",
                  static_cast<int>(chunk_key.bf.xcc0),
                  static_cast<int>(chunk_key.bf.ycc0),
                  static_cast<int>(chunk_key.bf.view_scale_msb),
                  ts,
                  p->ts);
        }
    } else if (p->static_object == LTSOT_SHIPYARD) {
        const auto ts = shipyard_->query_ts(chunk_key);
        if (ts > p->ts) {
            send_shipyard_cell_aligned(xc0_aligned, yc0_aligned, ex_lng, ex_lat, clamped_view_scale);
            LOGIx("Shipyards chunk key (%1%,%2%,%3%) Sent! (server ts %4%, client ts %5%)",
                  static_cast<int>(chunk_key.bf.xcc0),
                  static_cast<int>(chunk_key.bf.ycc0),
                  static_cast<int>(chunk_key.bf.view_scale_msb),
                  ts,
                  p->ts);
        } else {
            LOGIx("Shipyards chunk key (%1%,%2%,%3%) Not Sent! (server ts %4%, client ts %5%)",
                  static_cast<int>(chunk_key.bf.xcc0),
                  static_cast<int>(chunk_key.bf.ycc0),
                  static_cast<int>(chunk_key.bf.view_scale_msb),
                  ts,
                  p->ts);
        }
    } else {
        LOGE("Unknown static_object value: %1%", p->static_object);
    }
}

void udp_server::handle_ping_single_cell() {
    LOGIx("PINGSINGLECELL received.");
    auto p = reinterpret_cast<LWPTTLPINGSINGLECELL*>(recv_buffer_.data());
    send_single_cell(p->xc0, p->yc0);
}

void udp_server::handle_chat() {
    LOGIx("CHAT received.");
    auto p = reinterpret_cast<LWPTTLCHAT*>(recv_buffer_.data());
    std::shared_ptr<LWPTTLCHAT> reply(new LWPTTLCHAT);
    memset(reply.get(), 0, sizeof(LWPTTLCHAT));
    reply->type = LPGP_LWPTTLCHAT;
    strncpy(reply->line, p->line, boost::size(reply->line) - 1);
    reply->line[boost::size(reply->line) - 1] = 0;
    notify_to_all_clients(reply);
}

void udp_server::transform_single_cell() {
    LOGIx("TRANSFORMSINGLECELL received.");
    auto p = reinterpret_cast<LWPTTLTRANSFORMSINGLECELL*>(recv_buffer_.data());
    if (p->to == 0) {
        sea_static_->transform_single_cell_water_to_land(p->xc0, p->yc0);
    } else if (p->to == 1) {
        sea_static_->transform_single_cell_land_to_water(p->xc0, p->yc0);
    } else {
        LOGEP("Invalid message parameter p->to=%||", p);
    }
}

void udp_server::add_padding_bytes_inplace(uchar_vec& bytes_plaintext) {
    auto CIPHER_BLOCK_PADDING_SENTINEL = 0x80;
    auto CIPHER_BLOCK_BYTES = 16;
    bytes_plaintext.push_back(CIPHER_BLOCK_PADDING_SENTINEL);
    auto remainder = bytes_plaintext.size() % CIPHER_BLOCK_BYTES;
    if (remainder > 0) {
        bytes_plaintext.insert(bytes_plaintext.end(), CIPHER_BLOCK_BYTES - remainder, 0);
    }
}

int udp_server::encode_message(uchar_vec& bytes_iv_first,
                               uchar_vec& bytes_ciphertext,
                               const uchar_vec& bytes_plaintext,
                               unsigned char* bytes_key,
                               int len_key) {
    mbedtls_aes_context aes_context;
    mbedtls_aes_init(&aes_context);
    if (mbedtls_aes_setkey_enc(&aes_context, bytes_key, len_key * 8)) {
        LOGE("mbedtls_aes_setkey_enc failed.");
        return -1;
    }

    const auto len_plaintext = bytes_plaintext.size();

    if (len_plaintext % 16 != 0) {
        LOGE("input plaintext length should be multiple of 16-byte");
        return -2;
    }
    const auto len_iv = 0x10;
    unsigned char* bytes_iv;
    const auto result_iv = srp_alloc_random_bytes(&bytes_iv, len_iv);
    if (result_iv != 0) {
        LOGE("cannot seed iv");
        return -3;
    }

    bytes_iv_first.insert(bytes_iv_first.end(), bytes_iv, bytes_iv + 16);

    bytes_ciphertext.resize(len_plaintext);

    if (mbedtls_aes_crypt_cbc(&aes_context,
                              MBEDTLS_AES_ENCRYPT,
                              len_plaintext,
                              bytes_iv,
                              bytes_plaintext.data(),
                              bytes_ciphertext.data())) {
        LOGE("mbedtls_aes_crypt_cbc failed.");
        free(bytes_iv);
        return -4;
    }
    mbedtls_aes_free(&aes_context);
    free(bytes_iv);
    return 0;
}

int udp_server::decode_message(uchar_vec& bytes_plaintext,
                               unsigned char* bytes_iv,
                               unsigned char* bytes_ciphertext,
                               unsigned char* bytes_key,
                               int len_key) {
    mbedtls_aes_context aes_context;
    mbedtls_aes_init(&aes_context);
    if (mbedtls_aes_setkey_dec(&aes_context, bytes_key, len_key * 8)) {
        LOGE("mbedtls_aes_setkey_dec failed.");
        return -3;
    }
    if (mbedtls_aes_crypt_cbc(&aes_context,
                              MBEDTLS_AES_DECRYPT,
                              bytes_plaintext.size(),
                              bytes_iv,
                              bytes_ciphertext,
                              bytes_plaintext.data())) {
        LOGE("mbedtls_aes_crypt_cbc failed.");
        return -2;
    }
    mbedtls_aes_free(&aes_context);
    size_t sentinel_index = 0;
    for (size_t i = bytes_plaintext.size() - 1; i != static_cast<size_t>(-1); i--) {
        if (bytes_plaintext[i] == 0x80) {
            sentinel_index = i;
            break;
        }
    }
    if (sentinel_index == 0) {
        LOGE("Sentinel at zero index");
        return -1;
    } else {
        bytes_plaintext.resize(sentinel_index);
        return 0;
    }
}

void udp_server::handle_encrypted_json(std::size_t bytes_transferred) {
    LOGI("JSON received (%zu bytes).", bytes_transferred);
    // cipher JSON payload
    const char* bytes_account_id = reinterpret_cast<const char*>(recv_buffer_.data() + 0x04);
    int bytes_account_id_len = -1;
    for (int i = 0; i < 64; i++) {
        if (bytes_account_id[i] == 0) {
            bytes_account_id_len = i;
            break;
        }
    }
    if (bytes_account_id_len <= 0) {
        LOGE("Invalid account ID");
    } else {
        const int account_id_block_len = round_up(bytes_account_id_len + 1, 4); // plus 1 for a null-terminated character
        const char* key = session_->get_key(bytes_account_id);
        if (key) {
            LOGI("From Account ID: %1%", bytes_account_id);
            unsigned char* bytes_key;
            int len_key;
            srp_unhexify(key, &bytes_key, &len_key);
            const auto message_type_len = 0x04;
            const auto iv_len = 0x10;
            const auto bytes_iv = reinterpret_cast<unsigned char*>(recv_buffer_.data() + message_type_len + account_id_block_len);
            const auto bytes_ciphertext = reinterpret_cast<unsigned char*>(recv_buffer_.data() + message_type_len + account_id_block_len + iv_len);
            const auto ciphertext_len = bytes_transferred - message_type_len - account_id_block_len - iv_len;
            const auto plaintext_len = ciphertext_len;
            uchar_vec bytes_plaintext(plaintext_len);

            auto decode_result = decode_message(bytes_plaintext, bytes_iv, bytes_ciphertext, bytes_key, len_key);
            if (decode_result) {
                LOGE("error occured during decode_message: %||", decode_result);
            }

            jsmn_parser json_parser;
            jsmn_init(&json_parser);
            const int LW_MAX_JSON_TOKEN = 1024;
            std::vector<jsmntok_t> json_token(LW_MAX_JSON_TOKEN);
            const char* json_str = reinterpret_cast<const char*>(bytes_plaintext.data());
            int token_count = jsmn_parse(&json_parser,
                                         json_str,
                                         bytes_plaintext.size(),
                                         json_token.data(),
                                         LW_MAX_JSON_TOKEN);
            if (token_count == JSMN_ERROR_NOMEM) {
                LOGE("JSON parser NOMEM error...");
            } else if (token_count == JSMN_ERROR_INVAL) {
                LOGE("JSON parser INVALID CHARACTER error... : check for '\\c' kind of thing in json file");
            } else if (token_count == JSMN_ERROR_PART) {
                LOGE("JSON parser NOT COMPLETE error...");
            } else {
                if (token_count < 1 || json_token[0].type != JSMN_OBJECT) {
                    LOGE("JSON data broken...");
                } else {
                    // At last, valid JSON message from valid endpoint with valid credential
                    LOGI("VALID JSON MESSAGE FROM %1%", bytes_account_id);
                    int message_counter = 0;
                    std::string m, a1, a2, a3, a4, a5;
                    parse_json_message(token_count, json_str, json_token, message_counter, m, a1, a2, a3, a4, a5);
                    if (m == "sea_spawn_without_id") {
                        sea_->spawn(std::stof(a1), std::stof(a2), 1, 1, 0, 1);
                    } else if (m == "buy_seaport_ownership") {
                        int buy_ownership_result = seaport_->buy_ownership(std::stoi(a1), std::stoi(a2), bytes_account_id);
                        auto json_reply = make_reply_json(message_counter, buy_ownership_result, "");

                        // encrypt plaintext json reply
                        uchar_vec bytes_reply_ciphertext;
                        uchar_vec bytes_iv;
                        auto encrypt_result = encrypt_message(bytes_iv, bytes_reply_ciphertext, json_reply, bytes_key, len_key);
                        if (encrypt_result) {
                            LOGE("error occured during encrypt_message: %||", encrypt_result);
                        }

                        // send encrypted json reply
                        auto json_reply_message = create_encrypted_json_message(bytes_iv, bytes_reply_ciphertext);
                        send_compressed((const char*)&json_reply_message[0], static_cast<int>(json_reply_message.size()));

                    } else if (m == "buy_cargo_from_city") {
                        city_->buy_cargo(std::stoi(a1),
                                         std::stoi(a2),
                                         std::stoi(a3),
                                         std::stoi(a4),
                                         std::stoi(a5),
                                         bytes_account_id);
                    }
                }
            }
            free(bytes_key);
        } else {
            LOGEP("Key not found on sea-server session cache for account ID %1%", bytes_account_id);

            // encrypt plaintext json reply
            std::string reply = "{\"rc\":-1,\"note\":\"keynotfound\"}";

            // send encrypted json reply

            uchar_vec json_reply;
            json_reply.insert(json_reply.end(), { LPGP_LWPTTLJSONREPLY, 0, 0, 0 });
            json_reply.insert(json_reply.end(), reply.begin(), reply.end());
            send_compressed((const char*)&json_reply[0], static_cast<int>(json_reply.size()));
        }
    }
}

void udp_server::parse_json_message(int token_count,
                                    const char* json_str,
                                    std::vector<jsmntok_t>& json_token,
                                    int& message_counter,
                                    std::string& m,
                                    std::string& a1,
                                    std::string& a2,
                                    std::string& a3,
                                    std::string& a4,
                                    std::string& a5) {
    for (int i = 1; i < token_count; i++) {
        if (jsoneq(json_str, &json_token[i], "c") == 0) {
            message_counter = std::stoi(std::string(json_str + json_token[i + 1].start, json_token[i + 1].end - json_token[i + 1].start));
            LOGI("MESSAGE c: %1%", message_counter);
        }
        if (jsoneq(json_str, &json_token[i], "m") == 0) {
            int token_null_pos = std::min(json_token[i + 1].end - json_token[i + 1].start, static_cast<int>(sizeof(m)) - 1);
            m.assign(json_str + json_token[i + 1].start, token_null_pos);
            LOGI("MESSAGE m: %1%", m);
        }
        if (jsoneq(json_str, &json_token[i], "a1") == 0) {
            int token_null_pos = std::min(json_token[i + 1].end - json_token[i + 1].start, static_cast<int>(sizeof(a1)) - 1);
            a1.assign(json_str + json_token[i + 1].start, token_null_pos);
            LOGI("MESSAGE a1: %1%", a1);
        }
        if (jsoneq(json_str, &json_token[i], "a2") == 0) {
            int token_null_pos = std::min(json_token[i + 1].end - json_token[i + 1].start, static_cast<int>(sizeof(a2)) - 1);
            a2.assign(json_str + json_token[i + 1].start, token_null_pos);
            LOGI("MESSAGE a2: %1%", a2);
        }
        if (jsoneq(json_str, &json_token[i], "a3") == 0) {
            int token_null_pos = std::min(json_token[i + 1].end - json_token[i + 1].start, static_cast<int>(sizeof(a3)) - 1);
            a3.assign(json_str + json_token[i + 1].start, token_null_pos);
            LOGI("MESSAGE a3: %1%", a3);
        }
        if (jsoneq(json_str, &json_token[i], "a4") == 0) {
            int token_null_pos = std::min(json_token[i + 1].end - json_token[i + 1].start, static_cast<int>(sizeof(a4)) - 1);
            a4.assign(json_str + json_token[i + 1].start, token_null_pos);
            LOGI("MESSAGE a4: %1%", a4);
        }
        if (jsoneq(json_str, &json_token[i], "a5") == 0) {
            int token_null_pos = std::min(json_token[i + 1].end - json_token[i + 1].start, static_cast<int>(sizeof(a5)) - 1);
            a5.assign(json_str + json_token[i + 1].start, token_null_pos);
            LOGI("MESSAGE a5: %1%", a5);
        }
    }
}

std::string ss::udp_server::make_reply_json(int message_counter, int result_code, const std::string& note) {
    std::string reply;
    lua_getglobal(L(), "make_reply_json");
    lua_pushinteger(L(), message_counter);
    lua_pushinteger(L(), result_code);
    lua_pushstring(L(), note.c_str());
    if (lua_pcall(L(), 3/*arguments*/, 1/*result*/, 0)) {
        LOGEP("error: %1%", lua_tostring(L(), -1));
    } else {
        reply = lua_tostring(L(), -1);
    }
    lua_settop(L(), 0);
    LOGI("buy_seaport_ownership reply json: %1%", reply);
    return reply;
}

uchar_vec ss::udp_server::create_encrypted_json_message(const uchar_vec& bytes_iv, const uchar_vec& bytes_reply_ciphertext) {
    uchar_vec json_reply;
    json_reply.insert(json_reply.end(), { LPGP_LWPTTLJSON, 0, 0, 0 });
    json_reply.insert(json_reply.end(), bytes_iv.begin(), bytes_iv.end());
    json_reply.insert(json_reply.end(), bytes_reply_ciphertext.begin(), bytes_reply_ciphertext.end());
    return json_reply;
}

int udp_server::encrypt_message(uchar_vec& bytes_iv,
                                uchar_vec& bytes_ciphertext,
                                const std::string& message,
                                unsigned char* bytes_key,
                                int len_key) {
    uchar_vec bytes_plaintext(message.begin(), message.end());
    add_padding_bytes_inplace(bytes_plaintext);
    auto encode_result = encode_message(bytes_iv, bytes_ciphertext, bytes_plaintext, bytes_key, len_key);
    if (encode_result) {
        LOGE("error occured during encode_message: %||", encode_result);
        return encode_result;
    }
    return 0;
}

void udp_server::send_compressed(const char* bytes, int bytes_len) {
    char compressed[1500];
    int compressed_size = LZ4_compress_default((char*)bytes, compressed, bytes_len, static_cast<int>(boost::size(compressed)));
    if (compressed_size > 0) {
        socket_.async_send_to(boost::asio::buffer(compressed, compressed_size),
                              remote_endpoint_,
                              boost::bind(&udp_server::handle_send,
                                          this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    } else {
        LOGEP("send_compressed: LZ4_compress_default() error! - %1%", compressed_size);
    }
}

void udp_server::handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred) {
    if (!error || error == boost::asio::error::message_size) {
        unsigned char type = *reinterpret_cast<unsigned char*>(recv_buffer_.data() + 0x00); // type
        if (type == LPGP_LWPTTLPING) {
            handle_ping();
        } else if (type == LPGP_LWPTTLREQUESTWAYPOINTS) {
            handle_request_waypoints();
        } else if (type == LPGP_LWPTTLPINGFLUSH) {
            handle_ping_flush();
        } else if (type == LPGP_LWPTTLPINGCHUNK) {
            handle_ping_chunk();
        } else if (type == LPGP_LWPTTLPINGSINGLECELL) {
            handle_ping_single_cell();
        } else if (type == LPGP_LWPTTLCHAT) {
            handle_chat();
        } else if (type == LPGP_LWPTTLTRANSFORMSINGLECELL) {
            transform_single_cell();
        } else if (type == LPGP_LWPTTLJSON) {
            handle_encrypted_json(bytes_transferred);
        } else {
            LOGIP("Unknown UDP request of type %1%", static_cast<int>(type));
        }
    } else {
        LOGEP("error %1%, bytes_transferred %2%", error, bytes_transferred);
    }
    start_receive();
}

std::shared_ptr<route> udp_server::create_route_id(const std::vector<int>& seaport_id_list, int expect_land, std::shared_ptr<astarrtree::coro_context> coro) const {
    if (seaport_id_list.size() == 0) {
        return std::shared_ptr<route>();
    }
    std::vector<seaport_object::point> point_list;
    for (auto v : seaport_id_list) {
        point_list.emplace_back(seaport_->get_seaport_point(v));
        LOGI("Seaport ID: %1% (%2%)", v, seaport_->get_seaport_name(v));
    }
    std::vector<xy32> wp_total;
    for (size_t i = 0; i < point_list.size() - 1; i++) {
        auto wp = sea_static_->calculate_waypoints(point_list[i], point_list[i + 1], expect_land, coro);
        if (wp.size() >= 2) {
            std::copy(wp.begin(), wp.end(), std::back_inserter(wp_total));
        } else {
            LOGE("Waypoints of less than 2 detected. Route could not be found.");
            return std::shared_ptr<route>();
        }
    }
    std::shared_ptr<route> r(new route(wp_total, seaport_id_list[0], seaport_id_list[1], expect_land));
    r->set_velocity(0.1f);
    return r;
}

std::shared_ptr<const route> udp_server::find_route_map_by_ship_id(int ship_id) const {
    auto obj = sea_->get_by_db_id(ship_id);
    if (!obj) {
        LOGEP("Sea object %1% not found.", ship_id);
    } else {
        auto it = route_map_.find(obj->get_db_id());
        if (it != route_map_.cend()) {
            return it->second;
        } else {
            //LOGEP("Sea object %1% has no route info.", ship_id);
        }
    }
    return std::shared_ptr<const route>();
}

void udp_server::send_single_cell(int xc0, int yc0) {
    std::shared_ptr<LWPTTLSINGLECELL> reply(new LWPTTLSINGLECELL);
    memset(reply.get(), 0, sizeof(LWPTTLSINGLECELL));
    reply->type = LPGP_LWPTTLSINGLECELL;
    reply->xc0 = xc0;
    reply->yc0 = yc0;
    bool land_box_valid = false;
    bool water_box_valid = false;
    sea_static_object::box land_box;
    sea_static_object::box water_box;
    reply->attr = sea_static_->query_single_cell(xc0,
                                                 yc0,
                                                 land_box_valid,
                                                 land_box,
                                                 water_box_valid,
                                                 water_box);
    if (land_box_valid) {
        reply->land_box_valid = 1;
        reply->land_box[0] = land_box.min_corner().get<0>();
        reply->land_box[1] = land_box.min_corner().get<1>();
        reply->land_box[2] = land_box.max_corner().get<0>();
        reply->land_box[3] = land_box.max_corner().get<1>();
    }
    if (water_box_valid) {
        reply->water_box_valid = 1;
        reply->water_box[0] = water_box.min_corner().get<0>();
        reply->water_box[1] = water_box.min_corner().get<1>();
        reply->water_box[2] = water_box.max_corner().get<0>();
        reply->water_box[3] = water_box.max_corner().get<1>();
    }
    // seaport details
    int seaport_id = -1;
    int seaport_cargo = 0;
    int seaport_cargo_loaded = 0;
    int seaport_cargo_unloaded = 0;
    std::string seaport_lua_data;
    auto seaport_name = seaport_->query_single_cell(xc0,
                                                    yc0,
                                                    seaport_id,
                                                    seaport_cargo,
                                                    seaport_cargo_loaded,
                                                    seaport_cargo_unloaded,
                                                    seaport_lua_data);
    strncpy(reply->seaport_lua_data, seaport_lua_data.c_str(), sizeof(reply->seaport_lua_data));
    reply->seaport_lua_data[sizeof(reply->seaport_lua_data) - 1] = 0;
    reply->port_id = seaport_id;
    reply->cargo = seaport_cargo;
    reply->cargo_loaded = seaport_cargo_loaded;
    reply->cargo_unloaded = seaport_cargo_unloaded;
    if (seaport_id >= 0 && seaport_name) {
        strncpy(reply->port_name, seaport_name, boost::size(reply->port_name));
        reply->port_name[boost::size(reply->port_name) - 1] = 0;
    }
    // city details
    int city_id = -1;
    int city_population = 0;
    std::string city_lua_data;
    auto city_name = city_->query_single_cell(xc0, yc0, city_id, city_population, city_lua_data);
    reply->city_id = city_id;
    if (city_id >= 0 && city_name) {
        strncpy(reply->city_name, city_name, boost::size(reply->city_name));
        reply->city_name[boost::size(reply->city_name) - 1] = 0;
        reply->population = city_population;
    }
    strncpy(reply->city_lua_data, city_lua_data.c_str(), sizeof(reply->city_lua_data));
    reply->city_lua_data[sizeof(reply->city_lua_data) - 1] = 0;
    // take salvage
    int salvage_id = -1;
    int salvage_gold_amount = 0;
    salvage_->query_single_cell(xc0, yc0, salvage_id, salvage_gold_amount);
    if (salvage_id >= 0) {
        salvage_->despawn(salvage_id);
        if (salvage_gold_amount > 0) {
            gold_earned(xc0, yc0, salvage_gold_amount);
        } else {
            LOGEP("salvage_gold_amount is %1% at salvage ID %2%", salvage_gold_amount, salvage_id);
        }
    }
    // shipyard details
    int shipyard_id = -1;
    int shipyard_gold_amount = 0;
    auto shipyard_name = shipyard_->query_single_cell(xc0, yc0, shipyard_id, shipyard_gold_amount);
    reply->shipyard_id = shipyard_id;
    if (shipyard_id >= 0 && shipyard_name) {
        strncpy(reply->shipyard_name, shipyard_name, boost::size(reply->shipyard_name));
        reply->shipyard_name[boost::size(reply->shipyard_name) - 1] = 0;
    }
    // contract details
    int contract_id = -1;
    int contract_gold_amount = 0;
    contract_->query_single_cell(xc0, yc0, contract_id, contract_gold_amount);
    reply->contract_id = contract_id;
    // ship details
    std::vector<sea_object> sop_list;
    sea_->query_near_to_packet(static_cast<float>(xc0), static_cast<float>(yc0), 1, 1, sop_list);
    for (const auto& s : sop_list) {
        std::string ship_lua_data = sea_->query_lua_data(s.get_db_id());
        strncpy(reply->ship_lua_data, ship_lua_data.c_str(), sizeof(reply->ship_lua_data));
        reply->ship_lua_data[sizeof(reply->ship_lua_data) - 1] = 0;
    }
    // SEND!
    char compressed[1500];
    int compressed_size = LZ4_compress_default((char*)reply.get(), compressed, sizeof(LWPTTLSINGLECELL), static_cast<int>(boost::size(compressed)));
    if (compressed_size > 0) {
        socket_.async_send_to(boost::asio::buffer(compressed, compressed_size),
                              remote_endpoint_,
                              boost::bind(&udp_server::handle_send,
                                          this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    } else {
        LOGEP("LZ4_compress_default() error! - %1%", compressed_size);
    }
}

void udp_server::register_client_endpoint(const udp::endpoint& endpoint, const endpoint_aoi_object::box& aoi_box) {
    if (client_endpoints_.find(endpoint) == client_endpoints_.end()) {
        LOGI("Registering new client endpoint %1%...", endpoint);
    }
    client_endpoints_[endpoint] = get_monotonic_uptime_duration();
    auto aoi_value_it = client_endpoint_aoi_values_.find(endpoint);
    if (aoi_value_it != client_endpoint_aoi_values_.end()) {
        client_endpoint_aoi_rtree_.remove(aoi_value_it->second);
    }
    client_endpoint_aoi_int_key_++;
    client_endpoint_aoi_rtree_.insert(std::make_pair(aoi_box, client_endpoint_aoi_int_key_));
    client_endpoint_aoi_values_[endpoint] = std::make_pair(aoi_box, client_endpoint_aoi_int_key_);
    aoi_int_keys_[client_endpoint_aoi_int_key_] = endpoint;
}

void udp_server::remove_expired_endpoints() {
    const auto now = get_monotonic_uptime_duration();
    std::vector<udp::endpoint> to_be_removed;
    for (const auto& e : client_endpoints_) {
        const auto& last_update = e.second;
        if (now - last_update > std::chrono::seconds(3)) {
            to_be_removed.emplace_back(e.first);
        }
    }
    for (const auto& tbr : to_be_removed) {
        LOGI("Removing client endpoint %1%... (timeout)", tbr);
        client_endpoints_.erase(tbr);
        auto int_key = client_endpoint_aoi_values_[tbr].second;
        client_endpoint_aoi_rtree_.remove(client_endpoint_aoi_values_[tbr]);
        client_endpoint_aoi_values_.erase(tbr);
        aoi_int_keys_.erase(int_key);
    }
}

void udp_server::notify_to_client_gold_earned(int xc, int yc, int amount) {
    std::shared_ptr<LWPTTLGOLDEARNED> reply(new LWPTTLGOLDEARNED);
    memset(reply.get(), 0, sizeof(LWPTTLGOLDEARNED));
    reply->type = LPGP_LWPTTLGOLDEARNED;
    reply->xc0 = xc;
    reply->yc0 = yc;
    reply->amount = amount;
    notify_to_aoi_clients(reply, xc, yc);
}

void udp_server::notify_to_client_gold_used(int xc, int yc, int amount) {
    std::shared_ptr<LWPTTLGOLDUSED> reply(new LWPTTLGOLDUSED);
    memset(reply.get(), 0, sizeof(LWPTTLGOLDUSED));
    reply->type = LPGP_LWPTTLGOLDUSED;
    reply->xc0 = xc;
    reply->yc0 = yc;
    reply->amount = amount;
    notify_to_aoi_clients(reply, xc, yc);
}

void udp_server::notify_to_client_cargo_notification(const cargo_notification& cn) {
    std::shared_ptr<LWPTTLCARGONOTIFICATION> reply(new LWPTTLCARGONOTIFICATION);
    memset(reply.get(), 0, sizeof(LWPTTLCARGONOTIFICATION));
    reply->type = LPGP_LWPTTLCARGONOTIFICATION;
    reply->xc0 = cn.xc0;
    reply->yc0 = cn.yc0;
    reply->xc1 = cn.xc1;
    reply->yc1 = cn.yc1;
    reply->amount = cn.amount;
    reply->cargo_notification_type = cn.cargo_notification_type;
    notify_to_aoi_clients(reply, cn.xc0, cn.yc0);
}

std::vector<endpoint_aoi_object::value> udp_server::query_aoi_endpoints(int xc, int yc) const {
    std::vector<endpoint_aoi_object::value> endpoints;
    auto p = endpoint_aoi_object::point(xc, yc);
    client_endpoint_aoi_rtree_.query(bgi::intersects(p), std::back_inserter(endpoints));
    return endpoints;
}

template<> udp::endpoint udp_server::extract_endpoint(std::vector<udp::endpoint>::const_iterator v) {
    return *v;
}

template<> udp::endpoint udp_server::extract_endpoint(std::vector<endpoint_aoi_object::value>::const_iterator v) {
    return aoi_int_keys_[v->second];
}

void udp_server::flush_cargo_notifications() {
    auto city_cns = city_->flush_cargo_notifications();
    for (const auto& cn : city_cns) {
        notify_to_client_cargo_notification(cn);
    }
    auto sea_cns = sea_->flush_cargo_notifications();
    for (const auto& cn : sea_cns) {
        notify_to_client_cargo_notification(cn);
    }
}

void udp_server::send_stat() {
    std::shared_ptr<LWPTTLSTAT> reply(new LWPTTLSTAT);
    memset(reply.get(), 0, sizeof(LWPTTLSTAT));
    reply->type = LPGP_LWPTTLSTAT;
    reply->gold = gold_;
    reply->ports = static_cast<int>(seaport_->get_count());
    reply->ships = static_cast<int>(sea_->get_count());
    char compressed[1500];
    int compressed_size = LZ4_compress_default((char*)reply.get(), compressed, sizeof(LWPTTLSTAT), static_cast<int>(boost::size(compressed)));
    if (compressed_size > 0) {
        socket_.async_send_to(boost::asio::buffer(compressed, compressed_size),
                              remote_endpoint_,
                              boost::bind(&udp_server::handle_send,
                                          this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    } else {
        LOGEP("LZ4_compress_default() error! - %1%", compressed_size);
    }
}

std::vector<udp::endpoint> udp_server::endpoints() const {
    std::vector<udp::endpoint> endpoints;
    endpoints.reserve(client_endpoints_.size());
    for (const auto& e : client_endpoints_) {
        endpoints.push_back(e.first);
    }
    return endpoints;
}
