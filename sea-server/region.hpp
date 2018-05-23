#pragma once

#include "seaarea.hpp"

namespace ss {
    namespace bi = boost::interprocess;
    namespace bg = boost::geometry;
    namespace bgm = boost::geometry::model;
    namespace bgi = boost::geometry::index;
    
    class region {
    public:
        region();
        bool query_tree(float lng, float lat, std::string& name) const;
    private:
        seaarea seaarea_;
        seaarea eezland_;
        seaarea intersect_;
    };
}
