#pragma once

namespace ss {
    namespace bi = boost::interprocess;
    namespace bg = boost::geometry;
    namespace bgm = boost::geometry::model;
    namespace bgi = boost::geometry::index;

    class seaarea {
    public:
        seaarea(const std::string& rtree_filename, size_t mmap_max_size, const std::string& source_filename);
        bool query_tree(float lng, float lat, std::string& name) const;
        struct seaarea_entry {
            std::string name;
            float xmin;
            float ymin;
            float xmax;
            float ymax;
            int parts;
            int points;
            const int* parts_array;
            const float* lng_array;
            const float* lat_array;
        };
    private:
        typedef bgm::point<float, 2, bg::cs::cartesian> point;
        typedef bgm::box<point> box;
        typedef std::pair<box, int> value;
        typedef bgi::linear<32, 8> params;
        typedef bgi::indexable<value> indexable;
        typedef bgi::equal_to<value> equal_to;
        typedef bi::allocator<value, bi::managed_mapped_file::segment_manager> allocator;
        typedef bgi::rtree<value, params, indexable, equal_to, allocator> rtree;

        seaarea();
        seaarea(seaarea&);
        seaarea(seaarea&&);

        static bool query_rtree(rtree* rtree, const std::vector<seaarea_entry>& entries, float lng, float lat, std::string& name);
        static int contains_point_in_polygon(const seaarea_entry& ent, float testx, float testy);
        static void init_area_data(boost::interprocess::mapped_region& region, std::vector<seaarea_entry>& entries, rtree* rtree);

        bi::managed_mapped_file seaarea_rtree_file;
        allocator seaarea_alloc;
        rtree* seaarea_rtree;
        boost::interprocess::file_mapping seaarea_file;
        boost::interprocess::mapped_region seaarea_region;
        std::vector<seaarea_entry> seaarea_entries;
    };
}
