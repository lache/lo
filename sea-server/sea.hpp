#pragma once
#include "sea_object.hpp"

namespace ss {
    namespace bg = boost::geometry;
    namespace bgi = boost::geometry::index;
    class route;
    class udp_admin_server;
    class udp_server;
    class seaport;
    class sea {
        typedef bg::model::point<float, 2, bg::cs::cartesian> point;
        typedef bg::model::box<point> box;
        typedef std::pair<box, int> value;

    public:
        sea(boost::asio::io_service& io_service);
        void populate_test();
        int spawn(int type, float x, float y, float w, float h);
        int spawn(const char* guid, int type, float x, float y, float w, float h);
        void despawn(int type);
        void travel_to(const char* guid, float x, float y, float v = 1.0f);
        void teleport_to(int id, float x, float y, float vx = 0, float vy = 0);
        void teleport_to(const char* guid, float x, float y, float vx = 0, float vy = 0);
        void teleport_by(const char* guid, float dx, float dy);
        void query_near_lng_lat_to_packet(float lng, float lat, float ex_lng, float ex_lat, std::vector<sea_object>& sop_list) const;
        void query_near_to_packet(float xc, float yc, float ex_lng, float ex_lat, std::vector<sea_object>& sop_list) const;
        void update(float delta_time);
        void set_object_state(int id, SEA_OBJECT_STATE state);
        SEA_OBJECT_STATE get_object_state(int id) const;
        bool update_route(float delta_time,
                          int id,
                          std::shared_ptr<route> r,
                          std::shared_ptr<seaport> sp,
                          udp_server* us);
        sea_object* get_object(int id);
        sea_object* get_object_by_type(int type);
        void set_udp_admin_server(const std::shared_ptr<udp_admin_server>& uas) { this->uas = uas; }
    private:
        float lng_to_xc(float lng) const;
        float lat_to_yc(float lat) const;
        std::vector<int> query_tree(float xc, float yc, float ex_lng, float ex_lat) const;

        boost::asio::io_service& io_service;
        std::unordered_map<int, sea_object> sea_objects;
        std::unordered_map<int, int> sea_object_id_by_type;
        std::unordered_map<std::string, int> sea_guid_to_id;
        bgi::rtree< value, bgi::quadratic<16> > rtree;
        const int res_width;
        const int res_height;
        const float km_per_cell;
        int tick_seq;
        std::shared_ptr<udp_admin_server> uas;
    };
}
