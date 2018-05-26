#pragma once
#include "lz4.h"
#include "endpoint_aoi_object.hpp"

namespace ss {
    namespace bg = boost::geometry;
    namespace bgi = boost::geometry::index;
    using boost::asio::ip::udp;
    class sea;
    class sea_static;
    class seaport;
    class route;
    class region;
    class city;
    class salvage;
    class udp_server {

    public:
        udp_server(boost::asio::io_service& io_service,
                   std::shared_ptr<sea> sea,
                   std::shared_ptr<sea_static> sea_static,
                   std::shared_ptr<seaport> seaport,
                   std::shared_ptr<region> region,
                   std::shared_ptr<city> city,
                   std::shared_ptr<salvage> salvage);
        bool set_route(int id, int seaport_id1, int seaport_id2);
        void notify_to_client_gold_earned(int xc, int yc, int amount);
    private:
        void update();
        void salvage_update();
        void start_receive();
        void send_route_state(float lng, float lat, float ex_lng, float ex_lat, int view_scale);
        void send_land_cell(float lng, float lat, float ex_lng, float ex_lat, int view_scale);
        void send_land_cell_aligned(int xc0_aligned, int yc0_aligned, float ex_lng, float ex_lat, int view_scale);
        void send_seaport(float lng, float lat, float ex_lng, float ex_lat, int view_scale);
        void send_seaport_cell_aligned(int xc0_aligned, int yc0_aligned, float ex_lng, float ex_lat, int view_scale);
        void send_city_cell_aligned(int xc0_aligned, int yc0_aligned, float ex_lng, float ex_lat, int view_scale);
        void send_salvage_cell_aligned(int xc0_aligned, int yc0_aligned, float ex_lng, float ex_lat, int view_scale);
        void send_land_cell_aligned_bitmap(int xc0_aligned, int yc0_aligned, float ex_lng, float ex_lat, int view_scale);
        void send_track_object_coords(int track_object_id, int track_object_ship_id);
        void send_seaarea(float lng, float lat);
        void send_waypoints(int ship_id);
        std::shared_ptr<const route> find_route_map_by_ship_id(int ship_id) const;
        void send_single_cell(int xc0, int yc0);
        void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred);
        void handle_send(const boost::system::error_code& error, std::size_t bytes_transferred);
        std::shared_ptr<route> create_route_id(const std::vector<int>& seaport_id_list) const;
        void register_client_endpoint(const udp::endpoint& endpoint, const endpoint_aoi_object::box& aoi_box);
        void remove_expired_endpoints();
        template<typename T> void notify_to_aoi_clients(std::shared_ptr<T> packet, int xc, int yc) {
            notify_to_clients(packet, query_aoi_endpoints(xc, yc));
        }
        template<typename T> void notify_to_all_clients(std::shared_ptr<T> packet) {
            notify_to_clients(packet, client_endpoints_);
        }
        template<typename T, typename T2> void notify_to_clients(std::shared_ptr<T> packet, const T2& endpoints) {
            char compressed[1500];
            int compressed_size = LZ4_compress_default((char*)packet.get(), compressed, sizeof(T), static_cast<int>(boost::size(compressed)));
            if (compressed_size > 0) {
                for (auto it = endpoints.cbegin(); it != endpoints.cend(); it++) {
                    socket_.async_send_to(boost::asio::buffer(compressed, compressed_size),
                                          extract_endpoint(it),
                                          boost::bind(&udp_server::handle_send,
                                                      this,
                                                      boost::asio::placeholders::error,
                                                      boost::asio::placeholders::bytes_transferred));
                }
            } else {
                LOGEP("LZ4_compress_default() error! - %1%", compressed_size);
            }
        }
        template<typename T> udp::endpoint extract_endpoint(T v) {
            return v->first;
        }
        std::vector<endpoint_aoi_object::value> query_aoi_endpoints(int xc, int yc) const;
        udp::socket socket_;
        udp::endpoint remote_endpoint_;
        std::array<char, 1024> recv_buffer_;
        boost::asio::deadline_timer timer_;
        boost::asio::deadline_timer salvage_timer_;
        std::shared_ptr<sea> sea_;
        std::shared_ptr<sea_static> sea_static_;
        std::shared_ptr<seaport> seaport_;
        std::shared_ptr<region> region_;
        std::shared_ptr<city> city_;
        std::shared_ptr<salvage> salvage_;
        std::unordered_map<int, std::shared_ptr<route> > route_map_; // id -> route
        int tick_seq_;
        std::map<udp::endpoint, std::chrono::steady_clock::duration> client_endpoints_;
        std::map<udp::endpoint, endpoint_aoi_object::value> client_endpoint_aoi_values_;
        std::unordered_map<unsigned long long, udp::endpoint> aoi_int_keys_;
        endpoint_aoi_object::rtree client_endpoint_aoi_rtree_;
        unsigned long long client_endpoint_aoi_int_key_;
    };
    template<> udp::endpoint udp_server::extract_endpoint(std::vector<udp::endpoint>::const_iterator v);
    template<> udp::endpoint udp_server::extract_endpoint(std::vector<endpoint_aoi_object::value>::const_iterator v);
}
