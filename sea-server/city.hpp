#pragma once

#include "city_object.hpp"

struct xy32;
namespace ss {
    class seaport;
    class city {
    public:
        city(boost::asio::io_service& io_service,
             std::shared_ptr<seaport> seaport);
        std::vector<city_object> query_near_to_packet(int xc, int yc, float ex_lng, float ex_lat) const;
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
        const char* query_single_cell(int xc0, int yc0, int& id) const;
        void update();
    private:
        std::vector<city_object::value> query_tree_ex(int xc, int yc, int half_lng_ex, int half_lat_ex) const;
        void update_chunk_key_ts(int xc0, int yc0);
        void generate_cargo();
        bi::managed_mapped_file file;
        city_object::allocator alloc;
        city_object::rtree* rtree_ptr;
        const int res_width;
        const int res_height;
        const float km_per_cell;
        std::unordered_map<int, std::string> id_name; // city ID -> city name
        std::unordered_map<int, int> id_population; // city ID -> city population
        std::unordered_map<int, std::string> id_country; // city ID -> city country
        std::unordered_map<int, city_object::point> id_point; // city ID -> city position
        std::unordered_map<std::string, int> name_id; // city name -> city ID (XXX NOT UNIQUE XXX)
        std::unordered_map<int, long long> chunk_key_ts; // chunk key -> timestamp
        boost::asio::deadline_timer timer_;
        std::shared_ptr<seaport> seaport_;
    };
}
