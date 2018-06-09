#pragma once

#include "sea_static_object.hpp"

typedef struct _xy32 xy32;
namespace astarrtree {
    struct coro_context;
}
namespace ss {
    class sea_static {
    public:
        sea_static();
        std::vector<sea_static_object> query_near_lng_lat_to_packet(float lng, float lat, float ex) const;
        std::vector<sea_static_object> query_near_to_packet(int xc, int yc, float ex_lng, float ex_lat) const;
        std::vector<sea_static_object> query_near_to_packet(int xc0, int yc0, int xc1, int yc1) const;
        std::vector<sea_static_object> query_near_to_packet_water(int xc0, int yc0, int xc1, int yc1) const;
        std::vector<xy32> calculate_waypoints(const xy32& from, const xy32& to, int expect_land, std::shared_ptr<astarrtree::coro_context> coro) const;
        std::vector<xy32> calculate_waypoints(const sea_static_object::point& from, const sea_static_object::point& to, int expect_land, std::shared_ptr<astarrtree::coro_context> coro) const;
        bool is_water(const xy32& cell) const;
        bool is_sea_water(const xy32& cell) const;
        bool is_land(const xy32& cell) const;
        int lng_to_xc(float lng) const;
        int lat_to_yc(float lat) const;
        long long query_ts(const int xc0, const int yc0, const int view_scale) const;
        long long query_ts(const LWTTLCHUNKKEY& chunk_key) const;
        unsigned int sea_static::query_single_cell(int xc0,
                                                   int yc0,
                                                   bool& land_box_valid,
                                                   sea_static_object::box& land_box,
                                                   bool& water_box_valid,
                                                   sea_static_object::box& water_box) const;
        void update_single_chunk_key_ts(const LWTTLCHUNKKEY& chunk_key, long long monotonic_uptime);
        void transform_single_cell_water_to_land(int xc0, int yc0);
        void transform_single_cell_land_to_water(int xc0, int yc0);
    private:
        std::vector<sea_static_object::value> query_tree_ex(int xc, int yc, int half_lng_ex, int half_lat_ex) const;
        std::vector<sea_static_object::value> query_tree(int xc0, int yc0, int xc1, int yc1) const;
        std::vector<sea_static_object::value> query_tree_water(int xc0, int yc0, int xc1, int yc1) const;
        void mark_sea_water(sea_static_object::rtree* rtree);
        void update_chunk_key_ts(int xc0, int yc0);
        void transform_single_cell(int xc0, int yc0, int to);
        bi::managed_mapped_file land_file;
        sea_static_object::allocator land_alloc;
        sea_static_object::rtree* land_rtree_ptr;
        bi::managed_mapped_file water_file;
        sea_static_object::allocator water_alloc;
        sea_static_object::rtree* water_rtree_ptr;

        //bi::managed_mapped_file sea_water_set_file;
        //sea_static_object::allocator sea_water_set_alloc;

        const int res_width;
        const int res_height;
        const float km_per_cell;
        //typedef std::unordered_set<int, std::hash<int>, std::equal_to<int>, sea_static_object::allocator> sea_water_set_t;
        //sea_water_set_t* sea_water_set;
        std::vector<int> sea_water_vector;
        std::unordered_map<int, long long> chunk_key_ts; // chunk key -> timestamp
        long long time0_;
    };
}
