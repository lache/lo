#pragma once

#include "seaport_object.hpp"

typedef struct _xy32 xy32;
struct lua_State;
namespace ss {
    class seaport {
    public:
        enum seaport_type {
            SEA,
            LAND,
        };
        seaport(boost::asio::io_service& io_service, std::shared_ptr<lua_State> lua_state_instance);
        ~seaport();
        std::vector<seaport_object> query_near_to_packet(int xc, int yc, float ex_lng, float ex_lat) const;
        std::vector<seaport_object::value> query_nearest(int xc, int yc) const;
        const char* get_seaport_name(int id) const;
        seaport_object::point get_seaport_point(int id) const;
        int get_nearest_two(const xy32& pos, int& id1, std::string& name1, int& id2, std::string& name2) const;
        int lng_to_xc(float lng) const;
        int lat_to_yc(float lat) const;
        int spawn(int expected_db_id, const char* name, int xc0, int yc0, int owner_id, bool& existing_location, seaport_type st);
        int despawn(int id);
        void set_name(int id, const char* name, int owner_id, int port_type);
        long long query_ts(const int xc0, const int yc0, const int view_scale) const;
        long long query_ts(const LWTTLCHUNKKEY& chunk_key) const;
        const char* query_single_cell(int xc0, int yc0, int& id, int& cargo, int& cargo_loaded, int& cargo_unloaded, std::string& lua_data) const;
        void update_single_chunk_key_ts(const LWTTLCHUNKKEY& chunk_key, long long monotonic_uptime);
        int add_cargo(int id, int amount, bool source);
        int remove_cargo(int id, int amount, bool sink);
        int get_owner_id(int id) const;
        int get_type(int id) const;
        void update();
        size_t get_count() const { return rtree_ptr->size(); }
        int dock_ship_no_check(int seaport_id, int ship_id);
        int add_resource(int seaport_id, int resource_id, int amount);
        int buy_ownership(int xc0, int yc0, const char* requester) const;
    private:
        void init();
        std::vector<seaport_object::value> query_tree_ex(int xc, int yc, int half_lng_ex, int half_lat_ex) const;
        void update_chunk_key_ts(int xc0, int yc0);
        void convert_cargo();
		void create_lua_seaport_object(int seaport_id);
        std::shared_ptr<seaport_object::rtree_mem> rtree_ptr;
        const int res_width;
        const int res_height;
        std::unordered_map<int, long long> chunk_key_ts; // chunk key -> timestamp
        std::unordered_map<int, std::string> id_name; // seaport ID -> seaport name
        std::unordered_map<int, seaport_object::point> id_point; // seaport ID -> seaport position
        std::unordered_map<int, int> id_cargo; // seaport ID -> cargo count
        std::unordered_map<int, int> id_cargo_loaded; // seaport ID -> cargo count
        std::unordered_map<int, int> id_cargo_unloaded; // seaport ID -> cargo count
        std::unordered_map<int, int> id_owner_id; // seaport ID -> owner ID
        std::unordered_map<int, int> id_type; // seaport ID -> port type
        boost::asio::deadline_timer timer_;
        long long time0_;
        //int seaport_id_seq_;
        std::shared_ptr<lua_State> lua_state_instance;
        lua_State* L() const { return lua_state_instance.get(); }
    };
}
