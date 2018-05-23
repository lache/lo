#include "precompiled.hpp"
#include "city.hpp"
#include "xy.hpp"
#include "seaport.hpp"

#define CITY_RTREE_FILENAME "rtree/city.dat"
#define CITY_RTREE_MMAP_MAX_SIZE (4 * 1024 * 1024)

using namespace ss;

const auto update_interval = boost::posix_time::milliseconds(1000);

typedef struct _LWTTLDATA_CITY {
    char name[64]; // max length 35
    char country[4]; // country code; fixed length 2
    int population;
    float lng;
    float lat;
} LWTTLDATA_CITY;

city::city(boost::asio::io_service& io_service,
           std::shared_ptr<seaport> seaport)
    : file(bi::open_or_create, CITY_RTREE_FILENAME, CITY_RTREE_MMAP_MAX_SIZE)
    , alloc(file.get_segment_manager())
    , rtree_ptr(file.find_or_construct<city_object::rtree>("rtree")(city_object::params(), city_object::indexable(), city_object::equal_to(), alloc))
    , res_width(WORLD_MAP_PIXEL_RESOLUTION_WIDTH)
    , res_height(WORLD_MAP_PIXEL_RESOLUTION_HEIGHT)
    , km_per_cell(WORLD_CIRCUMFERENCE_IN_KM / res_width)
    , timer_(io_service, update_interval)
    , seaport_(seaport) {
    boost::interprocess::file_mapping city_file("assets/ttldata/cities.dat", boost::interprocess::read_only);
    boost::interprocess::mapped_region region(city_file, boost::interprocess::read_only);
    LWTTLDATA_CITY* sp = reinterpret_cast<LWTTLDATA_CITY*>(region.get_address());
    int count = static_cast<int>(region.get_size() / sizeof(LWTTLDATA_CITY));
    // dump citys.dat into r-tree data if r-tree is empty.
    if (rtree_ptr->size() == 0) {
        LOGI("Dumping %1% cities...", count);
        for (int i = 0; i < count; i++) {
            city_object::point point(lng_to_xc(sp[i].lng), lat_to_yc(sp[i].lat));
            rtree_ptr->insert(std::make_pair(point, i));
        }
    }
    for (int i = 0; i < count; i++) {
        id_name[i] = sp[i].name;
        id_population[i] = sp[i].population;
        id_country[i] = sp[i].country;
        name_id[sp[i].name] = i;
        //id_point[i] = city_object::point(lng_to_xc(sp[i].lng), lat_to_yc(sp[i].lat));
    }

    const auto monotonic_uptime = get_monotonic_uptime();

    std::set<std::pair<int, int> > point_set;
    std::vector<city_object::value> duplicates;
    const auto bounds = rtree_ptr->bounds();
    for (auto it = rtree_ptr->qbegin(bgi::intersects(bounds)); it != rtree_ptr->qend(); it++) {
        const auto xc0 = it->first.get<0>();
        const auto yc0 = it->first.get<1>();
        const auto point = std::make_pair(xc0, yc0);
        if (point_set.find(point) == point_set.end()) {
            id_point[it->second] = it->first;

            update_chunk_key_ts(xc0, yc0);

            point_set.insert(point);
        } else {
            duplicates.push_back(*it);
        }
    }
    for (const auto it : duplicates) {
        rtree_ptr->remove(it);
    }
    if (point_set.size() != rtree_ptr->size()) {
        LOGE("city rtree integrity check failure. Point set size %1% != R tree size %2%",
             point_set.size(),
             rtree_ptr->size());
        abort();
    }

    timer_.async_wait(boost::bind(&city::update, this));
}

int city::lng_to_xc(float lng) const {
    //return static_cast<int>(roundf(res_width / 2 + lng / 180.0f * res_width / 2)) & (res_width - 1);
    return static_cast<int>(roundf(res_width / 2 + lng / 180.0f * res_width / 2));
}

int city::lat_to_yc(float lat) const {
    //return static_cast<int>(roundf(res_height / 2 - lat / 90.0f * res_height / 2)) & (res_height - 1);
    return static_cast<int>(roundf(res_height / 2 - lat / 90.0f * res_height / 2));
}

//std::vector<city_object> city::query_near_lng_lat_to_packet(float lng, float lat, int half_lng_ex, int half_lat_ex) const {
//    return query_near_to_packet(lng_to_xc(lng),
//                                lat_to_yc(lat),
//                                half_lng_ex,
//                                half_lat_ex);
//}

std::vector<city_object> city::query_near_to_packet(int xc, int yc, float ex_lng, float ex_lat) const {
    const auto half_lng_ex = boost::math::iround(ex_lng / 2);
    const auto half_lat_ex = boost::math::iround(ex_lat / 2);
    auto values = query_tree_ex(xc, yc, half_lng_ex, half_lat_ex);
    std::vector<city_object> sop_list;
    for (std::size_t i = 0; i < values.size(); i++) {
        const auto city_id = values[i].second;
        const auto population_it = id_population.find(city_id);
        sop_list.emplace_back(city_object(values[i], population_it != id_population.cend() ? population_it->second : 0));
    }
    return sop_list;
}

std::vector<city_object::value> city::query_tree_ex(int xc, int yc, int half_lng_ex, int half_lat_ex) const {
    city_object::box query_box(city_object::point(xc - half_lng_ex, yc - half_lat_ex), city_object::point(xc + half_lng_ex, yc + half_lat_ex));
    std::vector<city_object::value> result_s;
    rtree_ptr->query(bgi::intersects(query_box), std::back_inserter(result_s));
    return result_s;
}

const char* city::get_city_name(int id) const {
    auto it = id_name.find(id);
    if (it != id_name.cend()) {
        return it->second.c_str();
    }
    return "";
}

int city::get_city_id(const char* name) const {
    auto it = name_id.find(name);
    if (it != name_id.cend()) {
        return it->second;
    }
    return -1;
}

city_object::point city::get_city_point(int id) const {
    if (id >= 0) {
        auto cit = id_point.find(id);
        if (cit != id_point.end()) {
            return cit->second;
        }
    }
    return city_object::point(-1, -1);
}

city_object::point city::get_city_point(const char* name) const {
    auto id = get_city_id(name);
    return get_city_point(id);
}

int city::get_nearest_two(const xy32& pos, int& id1, std::string& name1, int& id2, std::string& name2) const {
    id1 = -1;
    id2 = -1;
    city_object::point p = { boost::numeric_cast<int>(pos.x), boost::numeric_cast<int>(pos.y) };
    int count = 0;
    for (auto it = rtree_ptr->qbegin(bgi::nearest(p, 2)); it != rtree_ptr->qend(); it++) {
        if (count == 0) {
            id1 = it->second;
            name1 = get_city_name(it->second);
            LOGI("Nearest 1: %1% (%2%,%3%)", name1, it->first.get<0>(), it->first.get<1>());
            count++;
        } else if (count == 1) {
            id2 = it->second;
            name2 = get_city_name(it->second);
            LOGI("Nearest 2: %1% (%2%,%3%)", name2, it->first.get<0>(), it->first.get<1>());
            count++;
            return count;
        }
    }
    return 0;
}

void city::update_chunk_key_ts(int xc0, int yc0) {
    int view_scale = LNGLAT_VIEW_SCALE_PING_MAX;
    const auto monotonic_uptime = get_monotonic_uptime();
    while (view_scale) {
        const auto xc0_aligned = aligned_chunk_index(xc0, view_scale, LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS);
        const auto yc0_aligned = aligned_chunk_index(yc0, view_scale, LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS);
        const auto chunk_key = make_chunk_key(xc0_aligned, yc0_aligned, view_scale);
        auto it = chunk_key_ts.find(chunk_key.v);
        if (it != chunk_key_ts.end()) {
            it->second++;
        } else {
            chunk_key_ts[chunk_key.v] = monotonic_uptime;
        }
        view_scale >>= 1;
    }
}

int city::spawn(const char* name, int xc0, int yc0) {
    city_object::point new_port_point{ xc0, yc0 };
    if (rtree_ptr->qbegin(bgi::intersects(new_port_point)) != rtree_ptr->qend()) {
        // already exists
        return -1;
    }

    const auto id = static_cast<int>(rtree_ptr->size());
    rtree_ptr->insert(std::make_pair(new_port_point, id));
    id_point[id] = new_port_point;
    if (name[0] != 0) {
        set_name(id, name);
    } else {
        LOGE("%1%: city spawned, but name empty. (city id = %2%)",
             __func__,
             id);
    }

    update_chunk_key_ts(xc0, yc0);
    return id;
}

void city::despawn(int id) {
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

    rtree_ptr->remove(std::make_pair(it->second, id));
    id_point.erase(it);
    id_name.erase(id);
    //name_id
}

void city::set_name(int id, const char* name) {
    if (id_point.find(id) != id_point.end()) {
        id_name[id] = name;
        name_id[name] = id;
    } else {
        LOGE("%1%: cannot find city id %2%. city set name to '%3%' failed.",
             __func__,
             id,
             name);
    }
}

long long city::query_ts(const int xc0, const int yc0, const int view_scale) const {
    return query_ts(make_chunk_key(xc0, yc0, view_scale));
}

long long city::query_ts(const LWTTLCHUNKKEY& chunk_key) const {
    const auto cit = chunk_key_ts.find(chunk_key.v);
    if (cit != chunk_key_ts.cend()) {
        return cit->second;
    }
    return 0;
}

const char* city::query_single_cell(int xc0, int yc0, int& id) const {
    const auto city_it = rtree_ptr->qbegin(bgi::intersects(city_object::point{ xc0, yc0 }));
    if (city_it != rtree_ptr->qend()) {
        id = city_it->second;
        return get_city_name(city_it->second);
    }
    id = -1;
    return nullptr;
}

void city::update() {
    float delta_time = update_interval.total_milliseconds() / 1000.0f;

    timer_.expires_at(timer_.expires_at() + update_interval);
    timer_.async_wait(boost::bind(&city::update, this));

    generate_cargo();
}

void city::generate_cargo() {
    // iterate all cities
    const auto bounds = rtree_ptr->bounds();
    for (auto it = rtree_ptr->qbegin(bgi::intersects(bounds)); it != rtree_ptr->qend(); it++) {
        const auto xc = it->first.get<0>();
        const auto yc = it->first.get<1>();
        // query near seaports
        const auto near_extent = 10.0f;
        const auto seaports = seaport_->query_near_to_packet(xc, yc, near_extent, near_extent);
        if (seaports.size() > 0) {
            const auto total_cargo = 10.0f;
            const auto cargo = std::max(1, boost::math::iround(total_cargo / seaports.size()));
            for (const auto sop : seaports) {
                seaport_->add_cargo(sop.id, cargo, true);
            }
        }
    }
}
