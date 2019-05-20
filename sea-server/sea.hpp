#pragma once
#include "sea_object.hpp"
#include "cargo.h"

struct spawn_ship_command;
struct lua_State;
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
        sea(boost::asio::io_service& io_service,
            std::shared_ptr<seaport> seaport,
            std::shared_ptr<lua_State> lua_state_instance);
        ~sea();
        int spawn(int db_id, float x, float y, float w, float h, int expect_land, int template_id);
        int spawn(const spawn_ship_command& spawn_ship_cmd);
        int spawn(float x, float y, float w, float h, int expect_land, int template_id);
        void despawn(int type);
        int teleport_to(int id, float x, float y, float vx = 0, float vy = 0);
		int dock(int id);
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
        std::shared_ptr<sea_object> get_by_db_id(int db_id);
        void set_udp_admin_server(const std::shared_ptr<udp_admin_server>& uas) { this->uas = uas; }
        float lng_to_xc(float lng) const;
        float lat_to_yc(float lat) const;
        std::vector<cargo_notification>&& flush_cargo_notifications();
        size_t get_count() const { return rtree.size(); }
        int undock(int ship_id);
        std::string query_lua_data(int id);
    private:
        void init();
        std::vector<int> query_tree(float xc, float yc, float ex_lng, float ex_lat) const;
        boost::asio::io_service& io_service;
        std::unordered_map<int, std::shared_ptr<sea_object> > sea_objects;
        bgi::rtree< value, bgi::quadratic<16> > rtree;
        const int res_width;
        const int res_height;
        //const float km_per_cell;
        //int tick_seq;
        std::shared_ptr<udp_admin_server> uas;
        std::vector<cargo_notification> cargo_notifications;
        std::shared_ptr<lua_State> lua_state_instance;
        std::shared_ptr<seaport> seaport_;
        lua_State* L() const { return lua_state_instance.get(); }
        int undock_ship_no_check(int ship_id);
    };
}
