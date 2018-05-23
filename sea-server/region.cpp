#include "precompiled.hpp"
#include "region.hpp"

#define SEAAREA_SOURCE_FILENAME "assets/ttldata/seaareas.dat"
#define SEAAREA_RTREE_FILENAME "rtree/seaareas.dat"
#define SEAAREA_RTREE_MMAP_MAX_SIZE (4 * 1024 * 1024)

#define EEZLAND_SOURCE_FILENAME "assets/ttldata/eezlands.dat"
#define EEZLAND_RTREE_FILENAME "rtree/eezlands.dat"
#define EEZLAND_RTREE_MMAP_MAX_SIZE (1 * 1024 * 1024)

#define INTERSECT_SOURCE_FILENAME "assets/ttldata/intersects.dat"
#define INTERSECT_RTREE_FILENAME "rtree/intersects.dat"
#define INTERSECT_RTREE_MMAP_MAX_SIZE (1 * 1024 * 1024)

using namespace ss;

bool region::query_tree(float lng, float lat, std::string& name) const {
    if (intersect_.query_tree(lng, lat, name)) {
        return true;
    } else if (eezland_.query_tree(lng, lat, name)) {
        return true;
    } else {
        return seaarea_.query_tree(lng, lat, name);
    }
}

region::region()
    : seaarea_(SEAAREA_RTREE_FILENAME, SEAAREA_RTREE_MMAP_MAX_SIZE, SEAAREA_SOURCE_FILENAME)
    , eezland_(EEZLAND_RTREE_FILENAME, EEZLAND_RTREE_MMAP_MAX_SIZE, EEZLAND_SOURCE_FILENAME)
    , intersect_(INTERSECT_RTREE_FILENAME, INTERSECT_RTREE_MMAP_MAX_SIZE, INTERSECT_SOURCE_FILENAME) {
}
