#pragma once

namespace ss {
    namespace bi = boost::interprocess;
    namespace bg = boost::geometry;
    namespace bgm = boost::geometry::model;
    namespace bgi = boost::geometry::index;

    struct city_object {
        typedef bgm::point<int, 2, bg::cs::cartesian> point;
        typedef bgm::box<point> box; // only for query
        typedef std::pair<point, int> value;
        typedef bgi::linear<32, 8> params;
        typedef bgi::indexable<value> indexable;
        typedef bgi::equal_to<value> equal_to;
        typedef bi::allocator<value, bi::managed_mapped_file::segment_manager> allocator;
        typedef bgi::rtree<value, params, indexable, equal_to, allocator> rtree;

        int x0, y0;
        int id;
        int population;

        city_object(const value& v, int population)
            : x0(v.first.get<0>())
            , y0(v.first.get<1>())
            , id(v.second)
            , population(population) {
        }
    };
}
