#include "precompiled.hpp"
#include "shipyard.hpp"
#include "xy.hpp"
#include "sea_static.hpp"

using namespace ss;

const auto update_interval = boost::posix_time::milliseconds(1000);

shipyard::shipyard(boost::asio::io_service& io_service, std::shared_ptr<sea_static> sea_static)
    : res_width(WORLD_MAP_PIXEL_RESOLUTION_WIDTH)
    , res_height(WORLD_MAP_PIXEL_RESOLUTION_HEIGHT)
    , timer_(io_service, update_interval)
    , sea_static_(sea_static) {
    time0_ = get_monotonic_uptime();
    timer_.async_wait(boost::bind(&shipyard::update, this));
}

int shipyard::lng_to_xc(float lng) const {
    return static_cast<int>(roundf(res_width / 2 + lng / 180.0f * res_width / 2));
}

int shipyard::lat_to_yc(float lat) const {
    return static_cast<int>(roundf(res_height / 2 - lat / 90.0f * res_height / 2));
}

std::vector<shipyard_object> shipyard::query_near_to_packet(int xc, int yc, float ex_lng, float ex_lat) const {
    const auto half_lng_ex = boost::math::iround(ex_lng / 2);
    const auto half_lat_ex = boost::math::iround(ex_lat / 2);
    auto values = query_tree_ex(xc, yc, half_lng_ex, half_lat_ex);
    std::vector<shipyard_object> sop_list;
    for (std::size_t i = 0; i < values.size(); i++) {
        sop_list.emplace_back(shipyard_object(values[i]));
    }
    return sop_list;
}

std::vector<shipyard_object::value> shipyard::query_tree_ex(int xc, int yc, int half_lng_ex, int half_lat_ex) const {
    // min-max range should be inclusive-exclusive.
    shipyard_object::box query_box(shipyard_object::point(xc - half_lng_ex, yc - half_lat_ex),
                                   shipyard_object::point(xc + half_lng_ex - 1, yc + half_lat_ex - 1));
    std::vector<shipyard_object::value> result_s;
    rtree.query(bgi::intersects(query_box), std::back_inserter(result_s));
    return result_s;
}

const char* shipyard::get_shipyard_name(int id) const {
    auto it = id_name.find(id);
    if (it != id_name.cend()) {
        return it->second.c_str();
    }
    return "";
}

shipyard_object::point shipyard::get_shipyard_point(int id) const {
    if (id >= 0) {
        auto cit = id_point.find(id);
        if (cit != id_point.end()) {
            return cit->second;
        }
    }
    return shipyard_object::point(-1, -1);
}

int shipyard::get_nearest(const xy32& pos) const {
    shipyard_object::point p = { boost::numeric_cast<int>(pos.x), boost::numeric_cast<int>(pos.y) };
    //int count = 0;
    for (auto it = rtree.qbegin(bgi::nearest(p, 1)); it != rtree.qend(); it++) {
        return it->second;
    }
    return -1;
}

void shipyard::update_chunk_key_ts(int xc0, int yc0) {
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

void shipyard::update_single_chunk_key_ts(const LWTTLCHUNKKEY& chunk_key, long long monotonic_uptime) {
    chunk_key_ts[chunk_key.v] = monotonic_uptime;
}

int shipyard::spawn(int expected_db_id, const char* name, int xc0, int yc0, int owner_id, int gold_amount, bool& existing) {
    existing = false;
    shipyard_object::point new_shipyard_point{ xc0, yc0 };
    const auto existing_it = rtree.qbegin(bgi::intersects(new_shipyard_point));
    if (existing_it != rtree.qend()) {
        // already exists; return the existing one
        existing = true;
        return existing_it->second;
    }

    rtree.insert(std::make_pair(new_shipyard_point, expected_db_id));
    id_point[expected_db_id] = new_shipyard_point;
    if (name[0] != 0) {
        set_name(expected_db_id, name, gold_amount);
    } else {
        LOGEP("%1%: shipyard spawned, but name empty. (shipyard id = %2%)",
              __func__,
              expected_db_id);
    }
    id_owner_id[expected_db_id] = owner_id;

    update_chunk_key_ts(xc0, yc0);
    return expected_db_id;
}

void shipyard::despawn(int id) {
    auto it = id_point.find(id);
    if (it == id_point.end()) {
        LOGEP("id not found (%1%)", id);
        return;
    }

    const auto xc0 = it->second.get<0>();
    const auto yc0 = it->second.get<1>();

    rtree.remove(std::make_pair(it->second, id));
    id_name.erase(id);
    id_point.erase(it);
    id_gold_amount.erase(id);
    id_owner_id.erase(id);

    update_chunk_key_ts(xc0, yc0);
}

void shipyard::set_name(int id, const char* name, int gold_amount) {
    if (id_point.find(id) != id_point.end()) {
        id_name[id] = name;
        id_gold_amount[id] = gold_amount;
    } else {
        LOGEP("cannot find shipyard id %1%. shipyard set name to '%2%' failed.", id, name);
    }
}

long long shipyard::query_ts(const int xc0, const int yc0, const int view_scale) const {
    return query_ts(make_chunk_key(xc0, yc0, view_scale));
}

long long shipyard::query_ts(const LWTTLCHUNKKEY& chunk_key) const {
    const auto cit = chunk_key_ts.find(chunk_key.v);
    if (cit != chunk_key_ts.cend()) {
        return cit->second;
    }
    return time0_;
}

const char* shipyard::query_single_cell(int xc0, int yc0, int& id, int& gold_amount) const {
    const auto shipyard_it = rtree.qbegin(bgi::intersects(shipyard_object::point{ xc0, yc0 }));
    if (shipyard_it != rtree.qend()) {
        id = shipyard_it->second;
        const auto gold_amount_it = id_gold_amount.find(id);
        if (gold_amount_it != id_gold_amount.end()) {
            gold_amount = gold_amount_it->second;
        } else {
            gold_amount = 0;
        }
        return get_shipyard_name(shipyard_it->second);
    }
    id = -1;
    gold_amount = 0;
    return nullptr;
}

void shipyard::update() {
    //float delta_time = update_interval.total_milliseconds() / 1000.0f;

    timer_.expires_at(timer_.expires_at() + update_interval);
    timer_.async_wait(boost::bind(&shipyard::update, this));
}
