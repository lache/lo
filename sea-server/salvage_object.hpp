#pragma once

namespace ss {
    namespace bi = boost::interprocess;
    namespace bg = boost::geometry;
    namespace bgm = boost::geometry::model;
    namespace bgi = boost::geometry::index;

    struct salvage_object {
        typedef bgm::point<int, 2, bg::cs::cartesian> point;
        typedef bgm::box<point> box; // only for query
        typedef std::pair<point, int> value;
        typedef bgi::linear<32, 8> params;
        typedef bgi::rtree<value, params> rtree;

        int x0, y0;
        int id;

        salvage_object(const value& v)
            : x0(v.first.get<0>())
            , y0(v.first.get<1>())
            , id(v.second) {
        }
    };
}
