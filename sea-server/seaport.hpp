#pragma once

#include "seaport_object.hpp"

struct xy32;
namespace ss {
    class seaport {
    public:
        seaport(boost::asio::io_service& io_service);
        std::vector<seaport_object> query_near_to_packet(int xc, int yc, float ex_lng, float ex_lat) const;
        const char* get_seaport_name(int id) const;
        seaport_object::point get_seaport_point(int id) const;
        int get_nearest_two(const xy32& pos, int& id1, std::string& name1, int& id2, std::string& name2) const;
        int lng_to_xc(float lng) const;
        int lat_to_yc(float lat) const;
        int spawn(const char* name, int xc0, int yc0, int owner_id, bool& existing);
        void despawn(int id);
        void set_name(int id, const char* name, int owner_id);
        long long query_ts(const int xc0, const int yc0, const int view_scale) const;
        long long query_ts(const LWTTLCHUNKKEY& chunk_key) const;
        const char* query_single_cell(int xc0, int yc0, int& id, int& cargo, int& cargo_loaded, int& cargo_unloaded) const;
        void update_single_chunk_key_ts(const LWTTLCHUNKKEY& chunk_key, long long monotonic_uptime);
        int add_cargo(int id, int amount, bool source);
        int remove_cargo(int id, int amount, bool sink);
        int get_owner_id(int id) const;
        void update();
    private:
        std::vector<seaport_object::value> query_tree_ex(int xc, int yc, int half_lng_ex, int half_lat_ex) const;
        void update_chunk_key_ts(int xc0, int yc0);
        void convert_cargo();
        bi::managed_mapped_file file;
        seaport_object::allocator alloc;
        seaport_object::rtree* rtree_ptr;
        const int res_width;
        const int res_height;
        const float km_per_cell;
        std::unordered_map<int, long long> chunk_key_ts; // chunk key -> timestamp
        std::unordered_map<int, std::string> id_name; // seaport ID -> seaport name
        std::unordered_map<int, seaport_object::point> id_point; // seaport ID -> seaport position
        std::unordered_map<int, int> id_cargo; // seaport ID -> cargo count
        std::unordered_map<int, int> id_cargo_loaded; // seaport ID -> cargo count
        std::unordered_map<int, int> id_cargo_unloaded; // seaport ID -> cargo count
        std::unordered_map<int, int> id_owner_id; // seaport ID -> owner ID
        boost::asio::deadline_timer timer_;
    };
}
