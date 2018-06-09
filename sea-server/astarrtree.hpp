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

    struct coro_context {
        //boost::asio::io_service& io_service;
        //boost::asio::yield_context yield;
    };

    std::vector<xy32> astar_rtree_memory(const rtree* rtree_ptr, const xy32& from, const xy32& to, std::shared_ptr<coro_context> coro);
    box box_t_from_xy(xy32 v);
    bool find_nearest_point_if_empty(const rtree* rtree_ptr, xy32& from, box& from_box, std::vector<value>& from_result_s);
    xy32xy32 xyxy_from_box_t(const box& v);
    box box_t_from_xyxy(const xy32xy32& v);
    double xy32_distance(const xy32& a, const xy32& b);
}
