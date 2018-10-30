#include "precompiled.hpp"
#include "seaport.hpp"
#include "xy.hpp"

#define SEAPORT_RTREE_FILENAME "rtree/seaport.dat"
#define SEAPORT_RTREE_MMAP_MAX_SIZE (4 * 1024 * 1024)

using namespace ss;

const auto update_interval = boost::posix_time::milliseconds(1000);

void eval_lua_script_file(lua_State*, const char* lua_filename);

typedef struct _LWTTLDATA_SEAPORT {
    char name[80]; // maximum length from crawling data: 65
    char locode[8]; // fixed length of 5
    float lat;
    float lng;
} LWTTLDATA_SEAPORT;

seaport::seaport(boost::asio::io_service& io_service, std::shared_ptr<lua_State> lua_state_instance)
    : rtree_ptr(new seaport_object::rtree_mem())
    , res_width(WORLD_MAP_PIXEL_RESOLUTION_WIDTH)
    , res_height(WORLD_MAP_PIXEL_RESOLUTION_HEIGHT)
    , timer_(io_service, update_interval)
    , lua_state_instance(lua_state_instance) {
    init();
}

seaport::~seaport() {
}

void seaport::init() {
	eval_lua_script_file(L(), "assets/l/seaport.lua");

    time0_ = get_monotonic_uptime();
    
    timer_.async_wait(boost::bind(&seaport::update, this));
}

int seaport::lng_to_xc(float lng) const {
    return static_cast<int>(roundf(res_width / 2 + lng / 180.0f * res_width / 2));
}

int seaport::lat_to_yc(float lat) const {
    return static_cast<int>(roundf(res_height / 2 - lat / 90.0f * res_height / 2));
}

std::vector<seaport_object> seaport::query_near_to_packet(int xc, int yc, float ex_lng, float ex_lat) const {
    const auto half_lng_ex = boost::math::iround(ex_lng / 2);
    const auto half_lat_ex = boost::math::iround(ex_lat / 2);
    auto values = query_tree_ex(xc, yc, half_lng_ex, half_lat_ex);
    std::vector<seaport_object> sop_list;
    for (std::size_t i = 0; i < values.size(); i++) {
        sop_list.emplace_back(seaport_object(values[i], get_owner_id(values[i].second), get_type(values[i].second)));
    }
    return sop_list;
}

std::vector<seaport_object::value> seaport::query_tree_ex(int xc, int yc, int half_lng_ex, int half_lat_ex) const {
    // min-max range should be inclusive-exclusive.
    seaport_object::box query_box(seaport_object::point(xc - half_lng_ex, yc - half_lat_ex),
                                  seaport_object::point(xc + half_lng_ex - 1, yc + half_lat_ex - 1));
    std::vector<seaport_object::value> result_s;
    rtree_ptr->query(bgi::intersects(query_box), std::back_inserter(result_s));
    return result_s;
}

const char* seaport::get_seaport_name(int id) const {
    auto it = id_name.find(id);
    if (it != id_name.cend()) {
        return it->second.c_str();
    }
    return "";
}

seaport_object::point seaport::get_seaport_point(int id) const {
    if (id >= 0) {
        auto cit = id_point.find(id);
        if (cit != id_point.end()) {
            return cit->second;
        }
    }
    return seaport_object::point(-1, -1);
}

int seaport::get_nearest_two(const xy32& pos, int& id1, std::string& name1, int& id2, std::string& name2) const {
    id1 = -1;
    id2 = -1;
    seaport_object::point p = { boost::numeric_cast<int>(pos.x), boost::numeric_cast<int>(pos.y) };
    int count = 0;
    for (auto it = rtree_ptr->qbegin(bgi::nearest(p, 2)); it != rtree_ptr->qend(); it++) {
        if (count == 0) {
            id1 = it->second;
            name1 = get_seaport_name(it->second);
            LOGI("Nearest 1: %1% (%2%,%3%)", name1, it->first.get<0>(), it->first.get<1>());
            count++;
        } else if (count == 1) {
            id2 = it->second;
            name2 = get_seaport_name(it->second);
            LOGI("Nearest 2: %1% (%2%,%3%)", name2, it->first.get<0>(), it->first.get<1>());
            count++;
            return count;
        }
    }
    return 0;
}

void seaport::update_chunk_key_ts(int xc0, int yc0) {
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

void seaport::update_single_chunk_key_ts(const LWTTLCHUNKKEY& chunk_key, long long monotonic_uptime) {
    chunk_key_ts[chunk_key.v] = monotonic_uptime;
    /*auto it = chunk_key_ts.find(chunk_key.v);
    if (it != chunk_key_ts.end()) {
        it->second++;
    } else {
        chunk_key_ts[chunk_key.v] = monotonic_uptime;
    }*/
}

int seaport::spawn(int expected_db_id, const char* name, int xc0, int yc0, int owner_id, bool& existing_location, seaport::seaport_type st) {
    existing_location = false;

    if (expected_db_id <= 0) {
        LOGEP("Seaport with requested ID <= 0 cannot be spawned. (seaport id = %1%)", expected_db_id);
        return -1;
    }

    auto existing_point = id_point.find(expected_db_id);
    if (id_point.find(expected_db_id) != id_point.end()) {
        LOGEP("Seaport cannot be spawned with duplicated ID. (seaport id = %1%) Existing location is {%2%,%3%}",
              expected_db_id,
              existing_point->second.get<0>(),
              existing_point->second.get<1>());
        return -1;
    }

    seaport_object::point new_port_point{ xc0, yc0 };
    const auto existing_it = rtree_ptr->qbegin(bgi::intersects(new_port_point));
    if (existing_it != rtree_ptr->qend()) {
        // already exists; return the existing_location one
        existing_location = true;
        LOGEP("Seaport [%1%] already exists at {%2%,%3%}",
              existing_it->second,
              existing_it->first.get<0>(),
              existing_it->first.get<1>());
        return existing_it->second;
    }

    rtree_ptr->insert(std::make_pair(new_port_point, expected_db_id));
    id_point[expected_db_id] = new_port_point;
	create_lua_seaport_object(expected_db_id);
    if (name[0] != 0) {
        set_name(expected_db_id, name, owner_id, st);
    } else {
        LOGEP("seaport spawned, but name empty. (seaport id = %1%)", expected_db_id);
    }
    id_owner_id[expected_db_id] = owner_id;
    id_cargo[expected_db_id] = 0;

    update_chunk_key_ts(xc0, yc0);
    return expected_db_id;
}

int seaport::despawn(int id) {
    auto it = id_point.find(id);
    if (it == id_point.end()) {
        LOGEP("id not found (%1%)", id);
        return -1;
    }

    const auto xc0 = it->second.get<0>();
    const auto yc0 = it->second.get<1>();
    update_chunk_key_ts(xc0, yc0);

    rtree_ptr->remove(std::make_pair(it->second, id));
    id_name.erase(id);
    id_point.erase(it);
    id_cargo.erase(id);
    id_cargo_loaded.erase(id);
    id_cargo_unloaded.erase(id);
    id_owner_id.erase(id);
    id_type.erase(id);
    return 0;
}

void seaport::set_name(int id, const char* name, int owner_id, int port_type) {
    if (id_point.find(id) != id_point.end()) {
        id_name[id] = name;
        id_owner_id[id] = owner_id;
        id_cargo[id] = 0;
        id_type[id] = port_type;
    } else {
        LOGEP("cannot find seaport id %1%. seaport set name to '%2%' failed.", id, name);
    }
}

long long seaport::query_ts(const int xc0, const int yc0, const int view_scale) const {
    return query_ts(make_chunk_key(xc0, yc0, view_scale));
}

long long seaport::query_ts(const LWTTLCHUNKKEY& chunk_key) const {
    const auto cit = chunk_key_ts.find(chunk_key.v);
    if (cit != chunk_key_ts.cend()) {
        return cit->second;
    }
    return time0_;
}

const char* seaport::query_single_cell(int xc0, int yc0, int& id, int& cargo, int& cargo_loaded, int& cargo_unloaded, std::string& lua_data) const {
    const auto seaport_it = rtree_ptr->qbegin(bgi::intersects(seaport_object::point{ xc0, yc0 }));
    if (seaport_it != rtree_ptr->qend()) {
        id = seaport_it->second;
        const auto cargo_it = id_cargo.find(id);
        if (cargo_it != id_cargo.end()) {
            cargo = cargo_it->second;
        } else {
            cargo = 0;
        }
        const auto cargo_loaded_it = id_cargo_loaded.find(id);
        if (cargo_loaded_it != id_cargo_loaded.end()) {
            cargo_loaded = cargo_loaded_it->second;
        } else {
            cargo_loaded = 0;
        }
        const auto cargo_unloaded_it = id_cargo_unloaded.find(id);
        if (cargo_unloaded_it != id_cargo_unloaded.end()) {
            cargo_unloaded = cargo_unloaded_it->second;
        } else {
            cargo_unloaded = 0;
        }
        // call lua hook
        lua_getglobal(L(), "seaport_debug_query");
        lua_pushnumber(L(), id);
        if (lua_pcall(L(), 1/*arguments*/, 1/*result*/, 0)) {
            LOGEP("error: %1%", lua_tostring(L(), -1));
        } else {
            lua_data = lua_tostring(L(), -1);
            lua_pop(L(), 1);
        }
        return get_seaport_name(seaport_it->second);
    }
    id = -1;
    cargo = 0;
    return nullptr;
}

int seaport::add_cargo(int id, int amount, bool source) {
    if (amount < 0) {
        amount = 0;
    }
    if (amount > MAX_CARGO) {
        amount = MAX_CARGO;
    }
    const auto before_it = id_cargo.find(id);
    if (before_it == id_cargo.end()) {
        LOGEP("SP %1% not exist", id);
        return 0;
    }
    const auto before = before_it->second;
    auto after = before + amount;
    if (after > MAX_CARGO) {
        after = MAX_CARGO;
    }
    const auto actual_added = after - before;
    if (source) {
        id_cargo[id] = after;
    } else {
        id_cargo_unloaded[id] += actual_added;
    }
    return actual_added;
}

int seaport::remove_cargo(int id, int amount, bool sink) {
    if (amount < 0) {
        amount = 0;
    }
    if (amount > MAX_CARGO) {
        amount = MAX_CARGO;
    }
    const auto before_it = id_cargo.find(id);
    if (before_it == id_cargo.end()) {
        LOGEP("SP %1% not exist", id);
        return 0;
    }
    const auto before = before_it->second;
    auto after = before - amount;
    if (after < 0) {
        after = 0;
    }
    id_cargo[id] = after;
    const auto actual_removed = before - after;
    if (sink == false) {
        id_cargo_loaded[id] += actual_removed;
    }
    return actual_removed;
}

int seaport::get_owner_id(int id) const {
    const auto it = id_owner_id.find(id);
    if (it != id_owner_id.end()) {
        return it->second;
    }
    return 0;
}

int seaport::get_type(int id) const {
    const auto it = id_type.find(id);
    if (it != id_type.end()) {
        return it->second;
    }
    return 0;
}

void seaport::update() {
    //float delta_time = update_interval.total_milliseconds() / 1000.0f;

    timer_.expires_at(timer_.expires_at() + update_interval);
    timer_.async_wait(boost::bind(&seaport::update, this));

    convert_cargo();

    // call lua hook for each seaport
    // iterate all seaports
    const auto bounds = rtree_ptr->bounds();
    for (auto it = rtree_ptr->qbegin(bgi::intersects(bounds)); it != rtree_ptr->qend(); it++) {
        const auto seaport_id = it->second;
        lua_getglobal(L(), "seaport_update");
        lua_pushnumber(L(), seaport_id);
        if (lua_pcall(L(), 1/*arguments*/, 0/*result*/, 0)) {
            LOGEP("error: %1%", lua_tostring(L(), -1));
        }
    }
}

void seaport::convert_cargo() {
    // iterate all seaports
    const auto bounds = rtree_ptr->bounds();
    for (auto it = rtree_ptr->qbegin(bgi::intersects(bounds)); it != rtree_ptr->qend(); it++) {
        const auto unloaded = id_cargo_unloaded[it->second];
        if (unloaded > 0) {
            id_cargo_unloaded[it->second]--;
            id_cargo[it->second]++;
        }
    }
}

std::vector<seaport_object::value> seaport::query_nearest(int xc, int yc) const {
    std::vector<seaport_object::value> result_s;
    for (auto it = rtree_ptr->qbegin(bgi::nearest(seaport_object::point{ xc,yc }, 1)); it != rtree_ptr->qend(); it++) {
        result_s.push_back(*it);
    }
    return result_s;
}

void seaport::create_lua_seaport_object(int seaport_id) {
	// create city instance on lua
	lua_getglobal(L(), "seaport_new");
	lua_pushnumber(L(), seaport_id);
	if (lua_pcall(L(), 1/*arguments*/, 0/*result*/, 0)) {
		LOGEP("error: %1%", lua_tostring(L(), -1));
	}
}

int seaport::dock_ship_no_check(int seaport_id, int ship_id) {
    lua_getglobal(L(), "dock_ship");
    lua_pushnumber(L(), seaport_id);
    lua_pushnumber(L(), ship_id);
    if (lua_pcall(L(), 2/*arguments*/, 1/*result*/, 0)) {
        LOGEP("error: %1%", lua_tostring(L(), -1));
        return -3;
    } else {
        return static_cast<int>(lua_tointeger(L(), -1));
    }
}

int seaport::add_resource(int seaport_id, int resource_id, int amount) {
    lua_getglobal(L(), "seaport_add_resource");
    lua_pushnumber(L(), seaport_id);
    lua_pushnumber(L(), resource_id);
    lua_pushnumber(L(), amount);
    if (lua_pcall(L(), 3/*arguments*/, 1/*result*/, 0)) {
        LOGEP("error: %1%", lua_tostring(L(), -1));
        return -3;
    } else {
        return static_cast<int>(lua_tointeger(L(), -1));
    }
}

int seaport::buy_ownership(int xc0, int yc0, const char* requester) {
    if (requester == 0 || requester[0] == 0) {
        // empty requester
        return -1;
    }
    int seaport_id = 0;
    int cargo = 0;
    int cargo_loaded = 0;
    int cargo_unloaded = 0;
    std::string lua_data;
    const char* name = query_single_cell(xc0, yc0, seaport_id, cargo, cargo_loaded, cargo_unloaded, lua_data);
    if (name && seaport_id > 0) {
        lua_getglobal(L(), "seaport_buy_ownership");
        lua_pushnumber(L(), seaport_id);
        lua_pushstring(L(), requester);
        if (lua_pcall(L(), 2/*arguments*/, 1/*result*/, 0)) {
            LOGEP("error: %1%", lua_tostring(L(), -1));
            return -3;
        } else {
            return static_cast<int>(lua_tointeger(L(), -1));
        }
    } else {
        // invalid seaport selected
        return -2;
    }
    return 0;
}
