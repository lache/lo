#include "precompiled.hpp"
#include "salvage.hpp"
#include "xy.hpp"

using namespace ss;

const auto update_interval = boost::posix_time::milliseconds(1000);

salvage::salvage(boost::asio::io_service& io_service)
    : res_width(WORLD_MAP_PIXEL_RESOLUTION_WIDTH)
    , res_height(WORLD_MAP_PIXEL_RESOLUTION_HEIGHT)
    , km_per_cell(WORLD_CIRCUMFERENCE_IN_KM / res_width)
    , timer_(io_service, update_interval) {
    timer_.async_wait(boost::bind(&salvage::update, this));
}

int salvage::lng_to_xc(float lng) const {
    //return static_cast<int>(roundf(res_width / 2 + lng / 180.0f * res_width / 2)) & (res_width - 1);
    return static_cast<int>(roundf(res_width / 2 + lng / 180.0f * res_width / 2));
}

int salvage::lat_to_yc(float lat) const {
    //return static_cast<int>(roundf(res_height / 2 - lat / 90.0f * res_height / 2)) & (res_height - 1);
    return static_cast<int>(roundf(res_height / 2 - lat / 90.0f * res_height / 2));
}

//std::vector<salvage_object> salvage::query_near_lng_lat_to_packet(float lng, float lat, int half_lng_ex, int half_lat_ex) const {
//    return query_near_to_packet(lng_to_xc(lng),
//                                lat_to_yc(lat),
//                                half_lng_ex,
//                                half_lat_ex);
//}

std::vector<salvage_object> salvage::query_near_to_packet(int xc, int yc, float ex_lng, float ex_lat) const {
    const auto half_lng_ex = boost::math::iround(ex_lng / 2);
    const auto half_lat_ex = boost::math::iround(ex_lat / 2);
    auto values = query_tree_ex(xc, yc, half_lng_ex, half_lat_ex);
    std::vector<salvage_object> sop_list;
    for (std::size_t i = 0; i < values.size(); i++) {
        sop_list.emplace_back(salvage_object(values[i]));
    }
    return sop_list;
}

std::vector<salvage_object::value> salvage::query_tree_ex(int xc, int yc, int half_lng_ex, int half_lat_ex) const {
    salvage_object::box query_box(salvage_object::point(xc - half_lng_ex, yc - half_lat_ex), salvage_object::point(xc + half_lng_ex, yc + half_lat_ex));
    std::vector<salvage_object::value> result_s;
    rtree.query(bgi::intersects(query_box), std::back_inserter(result_s));
    return result_s;
}

const char* salvage::get_salvage_name(int id) const {
    auto it = id_name.find(id);
    if (it != id_name.cend()) {
        return it->second.c_str();
    }
    return "";
}

salvage_object::point salvage::get_salvage_point(int id) const {
    if (id >= 0) {
        auto cit = id_point.find(id);
        if (cit != id_point.end()) {
            return cit->second;
        }
    }
    return salvage_object::point(-1, -1);
}

int salvage::get_nearest_two(const xy32& pos, int& id1, std::string& name1, int& id2, std::string& name2) const {
    id1 = -1;
    id2 = -1;
    salvage_object::point p = { boost::numeric_cast<int>(pos.x), boost::numeric_cast<int>(pos.y) };
    int count = 0;
    for (auto it = rtree.qbegin(bgi::nearest(p, 2)); it != rtree.qend(); it++) {
        if (count == 0) {
            id1 = it->second;
            name1 = get_salvage_name(it->second);
            LOGI("Nearest 1: %1% (%2%,%3%)", name1, it->first.get<0>(), it->first.get<1>());
            count++;
        } else if (count == 1) {
            id2 = it->second;
            name2 = get_salvage_name(it->second);
            LOGI("Nearest 2: %1% (%2%,%3%)", name2, it->first.get<0>(), it->first.get<1>());
            count++;
            return count;
        }
    }
    return 0;
}

void salvage::update_chunk_key_ts(int xc0, int yc0) {
    int view_scale = LNGLAT_VIEW_SCALE_PING_MAX;
    const auto monotonic_uptime = get_monotonic_uptime();
    while (view_scale) {
        const auto xc0_aligned = aligned_chunk_index(xc0, view_scale, LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS);
        const auto yc0_aligned = aligned_chunk_index(yc0, view_scale, LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS);
        const auto chunk_key = make_chunk_key(xc0_aligned, yc0_aligned, view_scale);
        update_single_chunk_key_ts(chunk_key, monotonic_uptime);
        view_scale >>= 1;
    }
}

void salvage::update_single_chunk_key_ts(const LWTTLCHUNKKEY& chunk_key, long long monotonic_uptime) {
    auto it = chunk_key_ts.find(chunk_key.v);
    if (it != chunk_key_ts.end()) {
        it->second++;
    } else {
        chunk_key_ts[chunk_key.v] = monotonic_uptime;
    }
}

int salvage::spawn(const char* name, int xc0, int yc0, int gold_amount, bool& existing) {
    existing = false;
    salvage_object::point new_port_point{ xc0, yc0 };
    const auto existing_it = rtree.qbegin(bgi::intersects(new_port_point));
    if (existing_it != rtree.qend()) {
        // already exists; return the existing one
        existing = true;
        return existing_it->second;
    }

    const auto id = static_cast<int>(rtree.size());
    rtree.insert(std::make_pair(new_port_point, id));
    id_point[id] = new_port_point;
    if (name[0] != 0) {
        set_name(id, name, gold_amount);
    } else {
        LOGE("%1%: salvage spawned, but name empty. (salvage id = %2%)",
             __func__,
             id);
    }

    update_chunk_key_ts(xc0, yc0);
    return id;
}

void salvage::despawn(int id) {
    auto it = id_point.find(id);
    if (it == id_point.end()) {
        LOGE("%1%: id not found (%2%)",
             __func__,
             id);
        return;
    }
    
    const auto xc0 = it->second.get<0>();
    const auto yc0 = it->second.get<1>();
    update_chunk_key_ts(xc0, yc0);

    rtree.remove(std::make_pair(it->second, id));
    id_name.erase(id);
    id_point.erase(it);
    id_gold_amount.erase(id);
}

void salvage::set_name(int id, const char* name, int gold_amount) {
    if (id_point.find(id) != id_point.end()) {
        id_name[id] = name;
    } else {
        LOGE("%1%: cannot find salvage id %2%. salvage set name to '%3%' failed.",
             __func__,
             id,
             name);
    }
}

long long salvage::query_ts(const int xc0, const int yc0, const int view_scale) const {
    return query_ts(make_chunk_key(xc0, yc0, view_scale));
}

long long salvage::query_ts(const LWTTLCHUNKKEY& chunk_key) const {
    const auto cit = chunk_key_ts.find(chunk_key.v);
    if (cit != chunk_key_ts.cend()) {
        return cit->second;
    }
    return 0;
}

const char* salvage::query_single_cell(int xc0, int yc0, int& id, int& gold_amount) const {
    const auto salvage_it = rtree.qbegin(bgi::intersects(salvage_object::point{ xc0, yc0 }));
    if (salvage_it != rtree.qend()) {
        id = salvage_it->second;
        const auto gold_amount_it = id_gold_amount.find(id);
        if (gold_amount_it != id_gold_amount.end()) {
            gold_amount = gold_amount_it->second;
        } else {
            gold_amount = 0;
        }
        return get_salvage_name(salvage_it->second);
    }
    id = -1;
    gold_amount = 0;
    return nullptr;
}

void salvage::update() {
    float delta_time = update_interval.total_milliseconds() / 1000.0f;

    timer_.expires_at(timer_.expires_at() + update_interval);
    timer_.async_wait(boost::bind(&salvage::update, this));
}
