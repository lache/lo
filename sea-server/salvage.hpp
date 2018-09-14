#pragma once

#include "salvage_object.hpp"

typedef struct _xy32 xy32;
namespace ss {
    class sea_static;
    class salvage {
    public:
        salvage(boost::asio::io_service& io_service, std::shared_ptr<sea_static> sea_static);
        std::vector<salvage_object> query_near_to_packet(int xc, int yc, float ex_lng, float ex_lat) const;
        const char* get_salvage_name(int id) const;
        salvage_object::point get_salvage_point(int id) const;
        int get_nearest_two(const xy32& pos, int& id1, std::string& name1, int& id2, std::string& name2) const;
        int lng_to_xc(float lng) const;
        int lat_to_yc(float lat) const;
        int spawn(const char* name, int xc0, int yc0, int gold_amount, bool& existing);
        int spawn_random(const char* name, salvage_object::box box, bool& existing);
        void despawn(int id);
        void set_name(int id, const char* name, int gold_amount);
        long long query_ts(const int xc0, const int yc0, const int view_scale) const;
        long long query_ts(const LWTTLCHUNKKEY& chunk_key) const;
        const char* query_single_cell(int xc0, int yc0, int& id, int& gold_amount) const;
        void update_single_chunk_key_ts(const LWTTLCHUNKKEY& chunk_key, long long monotonic_uptime);
        void update();
    private:
        std::vector<salvage_object::value> query_tree_ex(int xc, int yc, int half_lng_ex, int half_lat_ex) const;
        void update_chunk_key_ts(int xc0, int yc0);
        salvage_object::rtree rtree;
        const int res_width;
        const int res_height;
        std::unordered_map<int, long long> chunk_key_ts; // chunk key -> timestamp
        std::unordered_map<int, std::string> id_name; // salvage ID -> salvage name
        std::unordered_map<int, salvage_object::point> id_point; // salvage ID -> salvage position
        std::unordered_map<int, int> id_gold_amount; // salvage ID -> gold amount
        boost::asio::deadline_timer timer_;
        std::shared_ptr<sea_static> sea_static_;
        boost::random::mt19937 rng_;
        long long time0_;
        int salvage_id_seq_;
    };
}
