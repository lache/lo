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
using namespace ss;

const auto update_interval = boost::posix_time::milliseconds(75);
//const auto update_interval = boost::posix_time::milliseconds(250);

udp_server::udp_server(boost::asio::io_service& io_service,
                       std::shared_ptr<sea> sea,
                       std::shared_ptr<sea_static> sea_static,
                       std::shared_ptr<seaport> seaport,
                       std::shared_ptr<region> region,
                       std::shared_ptr<city> city)
    : socket_(io_service, udp::endpoint(udp::v4(), 3100))
    , timer_(io_service, update_interval)
    , sea_(sea)
    , sea_static_(sea_static)
    , seaport_(seaport)
    , region_(region)
    , city_(city)
    , tick_seq_(0) {
    start_receive();
    timer_.async_wait(boost::bind(&udp_server::update, this));
}

bool udp_server::set_route(int id, int seaport_id1, int seaport_id2) {
    auto route = create_route_id({ seaport_id1, seaport_id2 });
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

void udp_server::send_dynamic_state(float lng, float lat, float ex_lng, float ex_lat, int view_scale) {
    std::vector<sea_object> sop_list;
    sea_->query_near_lng_lat_to_packet(lng, lat, ex_lng * view_scale, ex_lat * view_scale, sop_list);
    std::shared_ptr<LWPTTLDYNAMICSTATE> reply(new LWPTTLDYNAMICSTATE);
    memset(reply.get(), 0, sizeof(LWPTTLDYNAMICSTATE));
    reply->type = 109; // LPGP_LWPTTLDYNAMICSTATE
    size_t reply_obj_index = 0;
    for (sea_object const& v : sop_list) {
        auto& o = reply->obj[reply_obj_index];
        v.fill_packet(o);
        auto it = route_map_.find(v.get_id());
        if (it != route_map_.end() && it->second) {
            o.route_param = it->second->get_param();
            o.route_speed = 1.0f;
            o.route_reversed = it->second->get_reversed() ? 1 : 0;
        } else {
            o.route_param = 0;
            o.route_speed = 0;
            o.route_reversed = 0;
        }
        reply_obj_index++;
        if (reply_obj_index >= boost::size(reply->obj)) {
            break;
        }
    }
    reply->count = static_cast<int>(reply_obj_index);
    LOGIx("Querying (%1%,%2%) extent (%3%,%4%) => %5% hit(s).", lng, lat, ex_lng, ex_lat, reply_obj_index);
    char compressed[1500];
    int compressed_size = LZ4_compress_default((char*)reply.get(), compressed, sizeof(LWPTTLDYNAMICSTATE), static_cast<int>(boost::size(compressed)));
    if (compressed_size > 0) {
        socket_.async_send_to(boost::asio::buffer(compressed, compressed_size),
                              remote_endpoint_,
                              boost::bind(&udp_server::handle_send,
                                          this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    } else {
        LOGE("%1%: LZ4_compress_default() error! - %2%",
             __func__,
             compressed_size);
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
    reply->type = 115; // LPGP_LWPTTLSTATICSTATE2
    reply->ts = 1;
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
        LOGE("%1%: LZ4_compress_default() error! - %2%",
             __func__,
             compressed_size);
    }
    if (reply_obj_dropped_count) {
        LOGE("%1%: %2% cells dropped. (max: %3%) Compressed size is %4% bytes.",
             __func__,
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
    reply->type = 122; // LPGP_LWPTTLSTATICSTATE3
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
        LOGE("%1%: LZ4_compress_default() error! - %2%",
             __func__,
             compressed_size);
    }
}

void udp_server::send_track_object_coords(int track_object_id, int track_object_ship_id) {
    sea_object* obj = nullptr;
    if (track_object_id && track_object_ship_id) {
        LOGE("track_object_id and track_object_ship_id both set. tracking ignored");
        return;
    } else if (track_object_id) {
        obj = sea_->get_object(track_object_id);
    } else if (track_object_ship_id) {
        obj = sea_->get_object_by_type(track_object_ship_id);
    }
    if (!obj) {
        LOGE("Tracking object cannot be found. (track_object_id=%1%, track_object_ship_id=%2%)",
             track_object_id,
             track_object_ship_id);
    }
    std::shared_ptr<LWPTTLTRACKCOORDS> reply(new LWPTTLTRACKCOORDS);
    memset(reply.get(), 0, sizeof(LWPTTLTRACKCOORDS));
    reply->type = 113; // LPGP_LWPTTLTRACKCOORDS
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
        LOGE("%1%: LZ4_compress_default() error! - %2%",
             __func__,
             compressed_size);
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
    reply->type = 117; // LPGP_LWPTTLWAYPOINTS
    reply->ship_id = ship_id;
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
        LOGE("%1%: LZ4_compress_default() error! - %2%",
             __func__,
             compressed_size);
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
    reply->type = 112; // LPGP_LWPTTLSEAPORTSTATE
    reply->ts = seaport_->query_ts(xc0_aligned, yc0_aligned, view_scale);
    reply->xc0 = xc0_aligned;
    reply->yc0 = yc0_aligned;
    reply->view_scale = view_scale;
    size_t reply_obj_index = 0;
    const int view_scale_msb_index = msb_index(view_scale);
    for (seaport_object const& v : sop_list) {
        reply->obj[reply_obj_index].x_scaled_offset_0 = aligned_scaled_offset(v.x0, xc0_aligned, view_scale, view_scale_msb_index, false, 0, 0);
        reply->obj[reply_obj_index].y_scaled_offset_0 = aligned_scaled_offset(v.y0, yc0_aligned, view_scale, view_scale_msb_index, false, 0, 0);
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
        LOGE("%1%: LZ4_compress_default() error! - %2%",
             __func__,
             compressed_size);
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
    reply->type = 123; // LPGP_LWPTTLCITYSTATE
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
        LOGE("%1%: LZ4_compress_default() error! - %2%",
             __func__,
             compressed_size);
    }
}

void udp_server::send_seaarea(float lng, float lat) {
    std::string area_name;
    region_->query_tree(lng, lat, area_name);

    std::shared_ptr<LWPTTLSEAAREA> reply(new LWPTTLSEAAREA);
    memset(reply.get(), 0, sizeof(LWPTTLSEAAREA));
    reply->type = 114; // LPGP_LWPTTLSEAAREA
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
        if (type == 110) {
            // LPGP_LWPTTLPING
            LOGIx("PING received.");
            auto p = reinterpret_cast<LWPTTLPING*>(recv_buffer_.data());
            // ships (vessels)
            send_dynamic_state(p->lng, p->lat, p->ex_lng, p->ex_lat, p->view_scale);
            // area titles
            send_seaarea(p->lng, p->lat);
            if (p->track_object_id || p->track_object_ship_id) {
                // tracking info
                send_track_object_coords(p->track_object_id, p->track_object_ship_id);
            }
            register_client_endpoint();
        } else if (type == 116) {
            // LPGP_LWPTTLREQUESTWAYPOINTS
            LOGIx("REQUESTWAYPOINTS received.");
            auto p = reinterpret_cast<LWPTTLREQUESTWAYPOINTS*>(recv_buffer_.data());
            send_waypoints(p->ship_id);
            LOGIx("REQUESTWAYPOINTS replied with WAYPOINTS.");
        } else if (type == 118) {
            // LPGP_LWPTTLPINGFLUSH
            LOGI("PINGFLUSH received.");
        } else if (type == 119) {
            // LPGP_LWPTTLPINGCHUNK
            LOGIx("PINGCHUNK received.");
            auto p = reinterpret_cast<LWPTTLPINGCHUNK*>(recv_buffer_.data());
            LWTTLCHUNKKEY chunk_key;
            chunk_key.v = p->chunk_key;
            const int clamped_view_scale = boost::algorithm::clamp(1 << chunk_key.bf.view_scale_msb, 1 << 0, 1 << 6);
            const int xc0_aligned = chunk_key.bf.xcc0 << msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * clamped_view_scale);
            const int yc0_aligned = chunk_key.bf.ycc0 << msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * clamped_view_scale);
            const float ex_lng = LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS;
            const float ex_lat = LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS;
            if (p->static_object == 1) {
                // land cells
                send_land_cell_aligned(xc0_aligned, yc0_aligned, ex_lng, ex_lat, clamped_view_scale);
                //send_land_cell_aligned_bitmap(xc0_aligned, yc0_aligned, ex_lng, ex_lat, clamped_view_scale);
                LOGIx("Land cells Chunk key (%1%,%2%,%3%) Sent!",
                      static_cast<int>(chunk_key.bf.xcc0),
                      static_cast<int>(chunk_key.bf.ycc0),
                      static_cast<int>(chunk_key.bf.view_scale_msb));
            } else if (p->static_object == 2) {
                // seaports
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
            } else if (p->static_object == 3) {
                // cities
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
            } else {
                LOGE("Unknown static_object value: %1%", p->static_object);
            }
        } else if (type == 120) {
            // LPGP_LWPTTLPINGSINGLECELL
            LOGIx("PINGSINGLECELL received.");
            auto p = reinterpret_cast<LWPTTLPINGSINGLECELL*>(recv_buffer_.data());
            send_single_cell(p->xc0, p->yc0);
        } else {
            LOGI("%1%: Unknown UDP request of type %2%",
                 __func__,
                 static_cast<int>(type));
        }
    } else {
        LOGE("%1%: error %2%, bytes_transferred %3%", __func__, error, bytes_transferred);
    }
    start_receive();
}

std::shared_ptr<route> udp_server::create_route_id(const std::vector<int>& seaport_id_list) const {
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
        auto wp = sea_static_->calculate_waypoints(point_list[i], point_list[i + 1]);
        if (wp.size() >= 2) {
            std::copy(wp.begin(), wp.end(), std::back_inserter(wp_total));
        } else {
            LOGE("Waypoints of less than 2 detected. Route could not be found.");
            return std::shared_ptr<route>();
        }
    }
    std::shared_ptr<route> r(new route(wp_total, seaport_id_list[0], seaport_id_list[1]));
    r->set_velocity(1);
    return r;
}

std::shared_ptr<const route> udp_server::find_route_map_by_ship_id(int ship_id) const {
    auto obj = sea_->get_object_by_type(ship_id);
    if (!obj) {
        LOGE("%1%: Sea object %2% not found.",
             __func__,
             ship_id);
    } else {
        auto it = route_map_.find(obj->get_id());
        if (it != route_map_.end()) {
            return it->second;
        } else {
            LOGE("%1%: Sea object %2% has no route info.",
                 __func__,
                 ship_id);
        }
    }
    return std::shared_ptr<const route>();
}

void udp_server::send_single_cell(int xc0, int yc0) {
    std::shared_ptr<LWPTTLSINGLECELL> reply(new LWPTTLSINGLECELL);
    memset(reply.get(), 0, sizeof(LWPTTLSINGLECELL));
    reply->type = 121; // LPGP_LWPTTLSINGLECELL
    reply->xc0 = xc0;
    reply->yc0 = yc0;
    reply->attr = sea_static_->query_single_cell(xc0, yc0);
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
    auto city_name = city_->query_single_cell(xc0, yc0, city_id);
    reply->city_id = city_id;
    if (city_id >= 0 && city_name) {
        strncpy(reply->city_name, city_name, boost::size(reply->city_name));
        reply->city_name[boost::size(reply->city_name) - 1] = 0;
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
        LOGE("%1%: LZ4_compress_default() error! - %2%",
             __func__,
             compressed_size);
    }
}

void udp_server::register_client_endpoint() {
    if (client_endpoints_.find(remote_endpoint_) == client_endpoints_.end()) {
        LOGI("Registering new client endpoint %1%...", remote_endpoint_);
    }
    client_endpoints_[remote_endpoint_] = get_monotonic_uptime_duration();
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
    }
}

void udp_server::notify_to_client_gold_earned(int xc, int yc, int amount) {
    std::shared_ptr<LWPTTLGOLDEARNED> reply(new LWPTTLGOLDEARNED);
    memset(reply.get(), 0, sizeof(LWPTTLGOLDEARNED));
    reply->type = 124; // LPGP_LWPTTLGOLDEARNED
    reply->xc0 = xc;
    reply->yc0 = yc;
    reply->amount = amount;
    notify_to_client(reply);
}
