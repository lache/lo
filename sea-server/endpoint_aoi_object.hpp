#pragma once

namespace ss {
    namespace bi = boost::interprocess;
    namespace bg = boost::geometry;
    namespace bgm = boost::geometry::model;
    namespace bgi = boost::geometry::index;

    struct endpoint_aoi_object {
        typedef bgm::point<int, 2, bg::cs::cartesian> point;
        typedef bgm::box<point> box;
        typedef std::pair<box, unsigned long long> value;
        typedef bgi::linear<32, 8> params;
        typedef bgi::rtree<value, params> rtree;

        int x0, y0;
        int x1, y1;

        endpoint_aoi_object(const value& v) :
            x0(v.first.min_corner().get<0>()),
            y0(v.first.min_corner().get<1>()),
            x1(v.first.max_corner().get<0>()),
            y1(v.first.max_corner().get<1>())
        {}
    };
}
