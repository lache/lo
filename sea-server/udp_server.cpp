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
using namespace ss;

const auto update_interval = boost::posix_time::milliseconds(75);
//const auto update_interval = boost::posix_time::milliseconds(250);
const auto salvage_update_interval = boost::posix_time::milliseconds(600 * 1000);

udp_server::udp_server(boost::asio::io_service& io_service,
                       std::shared_ptr<sea> sea,
                       std::shared_ptr<sea_static> sea_static,
                       std::shared_ptr<seaport> seaport,
                       std::shared_ptr<region> region,
                       std::shared_ptr<city> city,
                       std::shared_ptr<salvage> salvage)
    : socket_(io_service, udp::endpoint(udp::v4(), 3100))
    , timer_(io_service, update_interval)
    , salvage_timer_(io_service, salvage_update_interval)
    , sea_(sea)
    , sea_static_(sea_static)
    , seaport_(seaport)
    , region_(region)
    , city_(city)
    , salvage_(salvage)
    , tick_seq_(0)
    , client_endpoint_aoi_int_key_(0)
    , gold_(0) {
    start_receive();
    timer_.async_wait(boost::bind(&udp_server::update, this));
    salvage_timer_.async_wait(boost::bind(&udp_server::salvage_update, this));
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

    float delta_time = salvage_update_interval.total_milliseconds() / 1000.0f;

    for (const auto& e : client_endpoint_aoi_values_) {
        const auto& aoi_box = e.second.first;
        auto salvage_existing = false;
        auto salvage_id = salvage_->spawn_random("Random", aoi_box, salvage_existing);
        LOGIx("Salvage spawned: ID=%1% (existing=%2%)", salvage_id, salvage_existing);
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
        v.fill_packet(o);
        auto it = route_map_.find(v.get_db_id());
        if (it != route_map_.end() && it->second) {
            o.route_param = it->second->get_param();
            o.route_speed = 1.0f;
            o.route_flags.reversed = it->second->get_reversed() ? 1 : 0;
        } else {
            o.route_param = 0;
            o.route_speed = 0;
            o.route_flags.reversed = 0;
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
        LOGE("Ship id %1%'s route cannot be found.", ship_id);
        return;
    }
    std::shared_ptr<LWPTTLWAYPOINTS> reply(new LWPTTLWAYPOINTS);
    memset(reply.get(), 0, sizeof(LWPTTLWAYPOINTS));
    reply->type = LPGP_LWPTTLWAYPOINTS;
    reply->ship_id = ship_id;
    reply->flags.land = r->get_land() ? 1 : 0;
    auto waypoints = r->clone_waypoints();
    auto copy_len = std::min(waypoints.size(), boost::size(reply->waypoints));
    reply->count = boost::numeric_cast<int>(copy_len);
    for (size_t i = 0; i < copy_len; i++) {
        reply->waypoints[i].x = waypoints[i].x;
        reply->waypoints[i].y = waypoints[i].y;
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
    const auto half_lng_cell_pixel_extent = boost::math::iround(ex_lng / 2.0f * view_scale);
    const auto half_lat_cell_pixel_extent = boost::math::iround(ex_lat / 2.0f * view_scale);
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
    const auto half_lng_cell_pixel_extent = boost::math::iround(ex_lng / 2.0f * view_scale);
    const auto half_lat_cell_pixel_extent = boost::math::iround(ex_lat / 2.0f * view_scale);
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
    const auto half_lng_cell_pixel_extent = boost::math::iround(ex_lng / 2.0f * view_scale);
    const auto half_lat_cell_pixel_extent = boost::math::iround(ex_lat / 2.0f * view_scale);
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

void udp_server::handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred) {
    if (!error || error == boost::asio::error::message_size) {
        unsigned char type = *reinterpret_cast<unsigned char*>(recv_buffer_.data() + 0x00); // type
        if (type == LPGP_LWPTTLPING) {
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
        } else if (type == LPGP_LWPTTLREQUESTWAYPOINTS) {
            LOGIx("REQUESTWAYPOINTS received.");
            auto p = reinterpret_cast<LWPTTLREQUESTWAYPOINTS*>(recv_buffer_.data());
            send_waypoints(p->ship_id);
            LOGIx("REQUESTWAYPOINTS replied with WAYPOINTS.");
        } else if (type == LPGP_LWPTTLPINGFLUSH) {
            LOGI("PINGFLUSH received.");
        } else if (type == LPGP_LWPTTLPINGCHUNK) {
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
            } else {
                LOGE("Unknown static_object value: %1%", p->static_object);
            }
        } else if (type == LPGP_LWPTTLPINGSINGLECELL) {
            LOGIx("PINGSINGLECELL received.");
            auto p = reinterpret_cast<LWPTTLPINGSINGLECELL*>(recv_buffer_.data());
            send_single_cell(p->xc0, p->yc0);
        } else if (type == LPGP_LWPTTLCHAT) {
            LOGIx("CHAT received.");
            auto p = reinterpret_cast<LWPTTLCHAT*>(recv_buffer_.data());
            std::shared_ptr<LWPTTLCHAT> reply(new LWPTTLCHAT);
            memset(reply.get(), 0, sizeof(LWPTTLCHAT));
            reply->type = LPGP_LWPTTLCHAT;
            strncpy(reply->line, p->line, boost::size(reply->line) - 1);
            reply->line[boost::size(reply->line) - 1] = 0;
            notify_to_all_clients(reply);
        } else if (type == LPGP_LWPTTLTRANSFORMSINGLECELL) {
            LOGIx("TRANSFORMSINGLECELL received.");
            auto p = reinterpret_cast<LWPTTLTRANSFORMSINGLECELL*>(recv_buffer_.data());
            if (p->to == 0) {
                sea_static_->transform_single_cell_water_to_land(p->xc0, p->yc0);
            } else if (p->to == 1) {
                sea_static_->transform_single_cell_land_to_water(p->xc0, p->yc0);
            } else {
                LOGEP("Invalid message parameter p->to=%||", p);
            }
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
    r->set_velocity(1);
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
            LOGEP("Sea object %1% has no route info.", ship_id);
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
    auto seaport_name = seaport_->query_single_cell(xc0,
                                                    yc0,
                                                    seaport_id,
                                                    seaport_cargo,
                                                    seaport_cargo_loaded,
                                                    seaport_cargo_unloaded);
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
    auto city_name = city_->query_single_cell(xc0, yc0, city_id, city_population);
    reply->city_id = city_id;
    if (city_id >= 0 && city_name) {
        strncpy(reply->city_name, city_name, boost::size(reply->city_name));
        reply->city_name[boost::size(reply->city_name) - 1] = 0;
        reply->population = city_population;
    }
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
