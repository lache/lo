#include "precompiled.hpp"
#include "seaarea.hpp"

using namespace ss;

int seaarea::contains_point_in_polygon(const seaarea_entry& ent, float testx, float testy) {
    int c = 0;
    const float* vertx = &ent.lng_array[0];
    const float* verty = &ent.lat_array[0];
    for (int pi = 0; pi < ent.parts; pi++) {
        int vert_beg = ent.parts_array[pi];
        int vert_end = pi < ent.parts - 1 ? ent.parts_array[pi + 1] : ent.points;
        for (int vi = vert_beg; vi < vert_end - 1; vi++) {
            const int i = vi;
            const int j = vi + 1;
            if (((verty[i] > testy) != (verty[j] > testy)) &&
                (testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i])) {
                c = !c;
            }
        }
    }
    return c;
}

bool seaarea::query_tree(float lng, float lat, std::string& name) const {
    return query_rtree(seaarea_rtree, seaarea_entries, lng, lat, name);
}

bool seaarea::query_rtree(rtree* rtree, const std::vector<seaarea_entry>& entries, float lng, float lat, std::string& name) {
    point query_point(lng, lat);
    for (auto q = rtree->qbegin(bgi::contains(query_point)); q != rtree->qend(); q++) {
        const auto& ent = entries[q->second];
        if (contains_point_in_polygon(ent, lng, lat) == 1) {
            name = ent.name;
            return true;
        }
    }
    return false;
}

typedef struct _LWTTLDATA_SEAAREA {
    char name[128];
    float bbox[4]; // min_x, min_y, max_x, max_y
    int parts;
    int points;
} LWTTLDATA_SEAAREA;

void seaarea::init_area_data(boost::interprocess::mapped_region& region, std::vector<seaarea_entry>& entries, rtree* rtree) {
    LWTTLDATA_SEAAREA* sa = reinterpret_cast<LWTTLDATA_SEAAREA*>(region.get_address());
    size_t read_accum = 0;
    while (read_accum < region.get_size()) {
        entries.push_back(seaarea_entry{
            sa->name,
            sa->bbox[0],
            sa->bbox[1],
            sa->bbox[2],
            sa->bbox[3],
            sa->parts,
            sa->points,
            reinterpret_cast<int*>(reinterpret_cast<char*>(sa) + sizeof(LWTTLDATA_SEAAREA)),
            reinterpret_cast<float*>(reinterpret_cast<char*>(sa) + sizeof(LWTTLDATA_SEAAREA) + sa->parts * sizeof(int)),
            reinterpret_cast<float*>(reinterpret_cast<char*>(sa) + sizeof(LWTTLDATA_SEAAREA) + sa->parts * sizeof(int) + sa->points * sizeof(float)) });
        size_t read = sizeof(LWTTLDATA_SEAAREA) + sa->parts * sizeof(int) + sa->points * 2 * sizeof(float);
        sa = reinterpret_cast<LWTTLDATA_SEAAREA*>(reinterpret_cast<char*>(sa) + read);
        read_accum += read;
    }

    // dump seaports.dat into r-tree data if r-tree is empty.
    if (rtree->size() == 0) {
        for (int i = 0; i < entries.size(); i++) {
            box box{ 
                { entries[i].xmin, entries[i].ymin },
            { entries[i].xmax, entries[i].ymax } };
            rtree->insert(std::make_pair(box, i));
        }
    }
}

seaarea::seaarea(const std::string& rtree_filename, size_t mmap_max_size, const std::string& source_filename)
    : seaarea_rtree_file(bi::open_or_create, rtree_filename.c_str(), mmap_max_size)
    , seaarea_alloc(seaarea_rtree_file.get_segment_manager())
    , seaarea_rtree(seaarea_rtree_file.find_or_construct<rtree>("rtree")(params(), indexable(), equal_to(), seaarea_alloc))
    , seaarea_file(source_filename.c_str(), boost::interprocess::read_only)
    , seaarea_region(seaarea_file, boost::interprocess::read_only) {
    init_area_data(seaarea_region, seaarea_entries, seaarea_rtree);
}
