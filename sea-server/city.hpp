#pragma once

#include "city_object.hpp"
#include "cargo.h"
#include "item.hpp"

typedef struct _xy32 xy32;
struct lua_State;
namespace ss {
    class seaport;
    class city {
    public:
        city(boost::asio::io_service& io_service,
             std::shared_ptr<seaport> seaport,
             std::shared_ptr<lua_State> lua_state_instance);
        ~city();
        std::vector<city_object> query_near_to_packet(int xc, int yc, float ex_lng, float ex_lat) const;
        std::vector<city_object::value> query_nearest(int xc, int yc) const;
        const char* get_city_name(int id) const;
        int get_city_id(const char* name) const;
        city_object::point get_city_point(int id) const;
        city_object::point get_city_point(const char* name) const;
        int get_nearest_two(const xy32& pos, int& id1, std::string& name1, int& id2, std::string& name2) const;
        int lng_to_xc(float lng) const;
        int lat_to_yc(float lat) const;
        int spawn(const char* name, int xc0, int yc0);
        void despawn(int id);
        void set_name(int id, const char* name);
        long long query_ts(const int xc0, const int yc0, const int view_scale) const;
        long long query_ts(const LWTTLCHUNKKEY& chunk_key) const;
        const char* query_single_cell(int xc0, int yc0, int& id, int& population, std::string& lua_data) const;
        void update_single_chunk_key_ts(const LWTTLCHUNKKEY& chunk_key, long long monotonic_uptime);
        void update();
        std::vector<cargo_notification>&& flush_cargo_notifications();
        int set_population(int id, int population);
        int buy_cargo(int xc0, int yc0, int city_id, int item_id, int amount, const char* requester);
    private:
        void init();
        float xc_to_lng(int xc) const;
        float yc_to_lat(int yc) const;
        std::vector<city_object::value> query_tree_ex(int xc, int yc, int half_lng_ex, int half_lat_ex) const;
        void update_chunk_key_ts(int xc0, int yc0);
        void generate_cargo();
        lua_State* L() const { return lua_state_instance.get(); }
        bi::managed_mapped_file file;
        city_object::allocator alloc;
        city_object::rtree* rtree_ptr;
        const int res_width;
        const int res_height;
        //const float km_per_cell;
        std::unordered_map<int, std::string> id_name; // city ID -> city name
        std::unordered_map<int, int> id_population; // city ID -> city population
        std::unordered_map<int, std::string> id_country; // city ID -> city country
        std::unordered_map<int, city_object::point> id_point; // city ID -> city position
        std::unordered_map<std::string, int> name_id; // city name -> city ID (XXX NOT UNIQUE XXX)
        std::unordered_map<int, item> id_wanted_item; // city ID -> wanted item
        std::unordered_map<int, item> id_produced_item; // city ID -> produced item
        std::unordered_map<int, long long> chunk_key_ts; // chunk key -> timestamp
        boost::asio::deadline_timer timer_;
        std::shared_ptr<seaport> seaport_;
        long long time0_;
        int city_id_seq_;
        std::vector<cargo_notification> cargo_notifications;
        std::shared_ptr<lua_State> lua_state_instance;
    };
}
