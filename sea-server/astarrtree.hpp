#pragma once

#include "xy.hpp"

namespace astarrtree {
    namespace bi = boost::interprocess;
    namespace bg = boost::geometry;
    namespace bgm = bg::model;
    namespace bgi = bg::index;

    typedef bgm::point<int, 2, bg::cs::cartesian> point;
    typedef bgm::box<point> box;
    typedef std::pair<box, int> value;
    typedef bgi::linear<32, 8> params;
    typedef bgi::indexable<value> indexable;
    typedef bgi::equal_to<value> equal_to;
    typedef bi::allocator<value, bi::managed_mapped_file::segment_manager> allocator;
    typedef bgi::rtree<value, params, indexable, equal_to, allocator> rtree;

    void astar_rtree(const char* water_rtree_filename,
                     size_t water_output_max_size,
                     const char* land_rtree_filename,
                     size_t land_output_max_size,
                     xy32 from,
                     xy32 to);
    std::vector<xy32> astar_rtree_memory(rtree* rtree_water_ptr, rtree* rtree_land_ptr, xy32 from, xy32 to);
    box box_t_from_xy(xy32 v);
    bool find_nearest_point_if_empty(rtree* rtree_ptr, xy32& from, box& from_box, std::vector<value>& from_result_s);
}
