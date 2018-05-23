#pragma once
#include "lz4.h"

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

    class udp_server {

    public:
        udp_server(boost::asio::io_service& io_service,
                   std::shared_ptr<sea> sea,
                   std::shared_ptr<sea_static> sea_static,
                   std::shared_ptr<seaport> seaport,
                   std::shared_ptr<region> region,
                   std::shared_ptr<city> city);
        bool set_route(int id, int seaport_id1, int seaport_id2);
        void notify_to_client_gold_earned(int xc, int yc, int amount);
    private:
        void update();
        void start_receive();
        void send_dynamic_state(float lng, float lat, float ex_lng, float ex_lat, int view_scale);
        void send_land_cell(float lng, float lat, float ex_lng, float ex_lat, int view_scale);
        void send_land_cell_aligned(int xc0_aligned, int yc0_aligned, float ex_lng, float ex_lat, int view_scale);
        void send_seaport(float lng, float lat, float ex_lng, float ex_lat, int view_scale);
        void send_seaport_cell_aligned(int xc0_aligned, int yc0_aligned, float ex_lng, float ex_lat, int view_scale);
        void send_city_cell_aligned(int xc0_aligned, int yc0_aligned, float ex_lng, float ex_lat, int view_scale);
        void send_land_cell_aligned_bitmap(int xc0_aligned, int yc0_aligned, float ex_lng, float ex_lat, int view_scale);
        void send_track_object_coords(int track_object_id, int track_object_ship_id);
        void send_seaarea(float lng, float lat);
        void send_waypoints(int ship_id);
        std::shared_ptr<const route> find_route_map_by_ship_id(int ship_id) const;
        void send_single_cell(int xc0, int yc0);
        void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred);
        void handle_send(const boost::system::error_code& error, std::size_t bytes_transferred);
        std::shared_ptr<route> create_route_id(const std::vector<int>& seaport_id_list) const;
        void register_client_endpoint();
        void remove_expired_endpoints();
        template<typename T> void notify_to_client(std::shared_ptr<T> packet) {
            char compressed[1500];
            int compressed_size = LZ4_compress_default((char*)packet.get(), compressed, sizeof(T), static_cast<int>(boost::size(compressed)));
            if (compressed_size > 0) {
                for (auto it : client_endpoints_) {
                    socket_.async_send_to(boost::asio::buffer(compressed, compressed_size),
                                          it.first,
                                          boost::bind(&udp_server::handle_send,
                                                      this,
                                                      boost::asio::placeholders::error,
                                                      boost::asio::placeholders::bytes_transferred));
                }
            } else {
                LOGE("%1%: LZ4_compress_default() error! - %2%",
                     __func__,
                     compressed_size);
            }
        }
        udp::socket socket_;
        udp::endpoint remote_endpoint_;
        std::array<char, 1024> recv_buffer_;
        boost::asio::deadline_timer timer_;
        std::shared_ptr<sea> sea_;
        std::shared_ptr<sea_static> sea_static_;
        std::shared_ptr<seaport> seaport_;
        std::shared_ptr<region> region_;
        std::shared_ptr<city> city_;
        std::unordered_map<int, std::shared_ptr<route> > route_map_; // id -> route
        int tick_seq_;
        std::map<udp::endpoint, std::chrono::steady_clock::duration> client_endpoints_;
    };
}
