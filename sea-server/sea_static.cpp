#include "precompiled.hpp"
#include "sea_static.hpp"
#include "astarrtree.hpp"
#include "packet.h"

#define WORLDMAP_RTREE_MMAP_MAX_SIZE_RATIO (sizeof(size_t) / 4)
#define WORLDMAP_RTREE_MMAP_MAX_SIZE(mb) ((mb) * 1024 * 1024 * WORLDMAP_RTREE_MMAP_MAX_SIZE_RATIO)
#define DATA_ROOT "rtree/"
#define WORLDMAP_LAND_MAX_RECT_RTREE_RTREE_FILENAME "worldmap_land_max_rect.dat"
#define WORLDMAP_LAND_MAX_RECT_RTREE_MMAP_MAX_SIZE WORLDMAP_RTREE_MMAP_MAX_SIZE(300)
#define WORLDMAP_WATER_MAX_RECT_RTREE_RTREE_FILENAME "worldmap_water_max_rect.dat"
#define WORLDMAP_WATER_MAX_RECT_RTREE_MMAP_MAX_SIZE WORLDMAP_RTREE_MMAP_MAX_SIZE(300)
#define SEA_WATER_SET_FILENAME "sea_water_set.dat"
#define SEA_WATER_SET_MMAP_MAX_SIZE WORLDMAP_RTREE_MMAP_MAX_SIZE(90)

using namespace ss;

static void load_from_dump_if_empty(sea_static_object::rtree* rtree_ptr, const char* dump_filename) {
    if (rtree_ptr->size() == 0) {
        LOGI("R-tree empty. Trying to create R-tree from dump file %s...",
             dump_filename);
        int rect_count = 0;
        FILE* fin = fopen(dump_filename, "rb");
        if (fin) {
            size_t read_max_count = 100000; // elements
            void* read_buf = malloc(sizeof(xy32xy32) * read_max_count);
            fseek(fin, 0, SEEK_SET);
            while (size_t read_count = fread(read_buf, sizeof(xy32xy32), read_max_count, fin)) {
                for (size_t i = 0; i < read_count; i++) {
                    rect_count++;
                    xy32xy32* r = reinterpret_cast<xy32xy32*>(read_buf) + i;
                    sea_static_object::box box(sea_static_object::point(r->xy0.x, r->xy0.y), sea_static_object::point(r->xy1.x, r->xy1.y));
                    rtree_ptr->insert(std::make_pair(box, rect_count));
                }
            }
            fclose(fin);
            LOGI("Max rect R Tree size (after loaded from %1%): %2%", dump_filename, rtree_ptr->size());
        } else {
            LOGE("[Error] Dump file %s not exist!", dump_filename);
        }
    }
}

sea_static::sea_static()
    : land_file(bi::open_or_create, DATA_ROOT WORLDMAP_LAND_MAX_RECT_RTREE_RTREE_FILENAME, WORLDMAP_LAND_MAX_RECT_RTREE_MMAP_MAX_SIZE)
    , land_alloc(land_file.get_segment_manager())
    , land_rtree_ptr(land_file.find_or_construct<sea_static_object::rtree>("rtree")(sea_static_object::params(), sea_static_object::indexable(), sea_static_object::equal_to(), land_alloc))
    , water_file(bi::open_or_create, DATA_ROOT WORLDMAP_WATER_MAX_RECT_RTREE_RTREE_FILENAME, WORLDMAP_WATER_MAX_RECT_RTREE_MMAP_MAX_SIZE)
    , water_alloc(water_file.get_segment_manager())
    , water_rtree_ptr(water_file.find_or_construct<sea_static_object::rtree>("rtree")(sea_static_object::params(), sea_static_object::indexable(), sea_static_object::equal_to(), water_alloc))
    , res_width(WORLD_MAP_PIXEL_RESOLUTION_WIDTH)
    , res_height(WORLD_MAP_PIXEL_RESOLUTION_HEIGHT)
    //, sea_water_set_file(bi::open_or_create, DATA_ROOT SEA_WATER_SET_FILENAME, SEA_WATER_SET_MMAP_MAX_SIZE)
    //, sea_water_set_alloc(sea_water_set_file.get_segment_manager())
    //, sea_water_set(sea_water_set_file.find_or_construct<sea_water_set_t>("set")(int(), std::hash<int>(), std::equal_to<int>(), sea_water_set_alloc))
{
    time0_ = get_monotonic_uptime();

    load_from_dump_if_empty(land_rtree_ptr, "rtree/land_raw_xy32xy32.bin");
    load_from_dump_if_empty(water_rtree_ptr, "rtree/water_raw_xy32xy32.bin");
    mark_sea_water(water_rtree_ptr);

    //astarrtree::astar_rtree_memory(water_rtree_ptr, { 148858,26779 }, { 149726,27639 }, std::shared_ptr<astarrtree::coro_context>());
    //astarrtree::astar_rtree_memory(land_rtree_ptr, { 0, 0}, { 3, 3 }, std::shared_ptr<astarrtree::coro_context>());

    //astarrtree::astar_rtree_memory(water_rtree_ptr, { 148138, 26988 }, { 147850, 26546 }, std::shared_ptr<astarrtree::coro_context>());

    //astarrtree::astar_rtree_memory(land_rtree_ptr, { 141622, 56081 }, { 146660, 51227 }, std::shared_ptr<astarrtree::coro_context>());
}

int sea_static::lng_to_xc(float lng) const {
    //return static_cast<int>(roundf(res_width / 2 + lng / 180.0f * res_width / 2)) & (res_width - 1);
    return static_cast<int>(roundf(res_width / 2 + lng / 180.0f * res_width / 2));
}

int sea_static::lat_to_yc(float lat) const {
    //return static_cast<int >(roundf(res_height / 2 - lat / 90.0f * res_height / 2)) & (res_height - 1);
    return static_cast<int>(roundf(res_height / 2 - lat / 90.0f * res_height / 2));
}

std::vector<sea_static_object> sea_static::query_near_lng_lat_to_packet(float lng, float lat, float ex) const {
    return query_near_to_packet(lng_to_xc(lng), lat_to_yc(lat), ex, ex);
}

std::vector<sea_static_object> sea_static::query_near_to_packet(int xc, int yc, float ex_lng, float ex_lat) const {
    const auto half_lng_ex = boost::math::iround(ex_lng / 2);
    const auto half_lat_ex = boost::math::iround(ex_lat / 2);
    auto values = query_tree_ex(xc, yc, half_lng_ex, half_lat_ex);
    std::vector<sea_static_object> sop_list;
    for (std::size_t i = 0; i < values.size(); i++) {
        sop_list.emplace_back(sea_static_object(values[i]));
    }
    return sop_list;
}

std::vector<sea_static_object> sea_static::query_near_to_packet(int xc0, int yc0, int xc1, int yc1) const {
    auto values = query_tree(xc0, yc0, xc1, yc1);
    std::vector<sea_static_object> sop_list;
    for (std::size_t i = 0; i < values.size(); i++) {
        sop_list.emplace_back(sea_static_object(values[i]));
    }
    return sop_list;
}

std::vector<sea_static_object> sea_static::query_near_to_packet_water(int xc0, int yc0, int xc1, int yc1) const {
    auto values = query_tree_water(xc0, yc0, xc1, yc1);
    std::vector<sea_static_object> sop_list;
    for (std::size_t i = 0; i < values.size(); i++) {
        sop_list.emplace_back(sea_static_object(values[i]));
    }
    return sop_list;
}

std::vector<sea_static_object::value> sea_static::query_tree_ex(int xc, int yc, int half_lng_ex, int half_lat_ex) const {
    // min-max range should be inclusive-exclusive.
    return query_tree(xc - half_lng_ex,
                      yc - half_lat_ex,
                      xc + half_lng_ex - 1,
                      yc + half_lat_ex - 1);
}

std::vector<sea_static_object::value> sea_static::query_tree(int xc0, int yc0, int xc1, int yc1) const {
    sea_static_object::box query_box(sea_static_object::point(xc0, yc0),
                                     sea_static_object::point(xc1, yc1));
    std::vector<sea_static_object::value> result_s;
    land_rtree_ptr->query(bgi::intersects(query_box), std::back_inserter(result_s));
    return result_s;
}

std::vector<sea_static_object::value> sea_static::query_tree_water(int xc0, int yc0, int xc1, int yc1) const {
    sea_static_object::box query_box(sea_static_object::point(xc0, yc0), sea_static_object::point(xc1, yc1));
    std::vector<sea_static_object::value> result_s;
    water_rtree_ptr->query(bgi::intersects(query_box), std::back_inserter(result_s));
    return result_s;
}

void sea_static::mark_sea_water(sea_static_object::rtree* rtree) {
    const char* sea_water_dump_filename = "rtree/sea_water_dump.dat";
    LOGI("Checking %1%...", sea_water_dump_filename);
    struct stat stat_buffer;
    if (stat(sea_water_dump_filename, &stat_buffer) != 0) {
        std::unordered_set<int> sea_water_set;
        LOGI("%1% not exists. Creating...", sea_water_dump_filename);
        LOGI("R-tree total node: %1%", rtree->size());
        std::vector<sea_static_object::rtree::const_query_iterator> open_set;
        for (auto it = rtree->qbegin(bgi::intersects(astarrtree::box_t_from_xy(xy32{ 0,0 }))); it != rtree->qend(); it++) {
            sea_water_set.insert(it->second);
            open_set.push_back(it);
        }
        std::unordered_set<int> visited_set;
        while (open_set.size() > 0) {
            std::vector<sea_static_object::rtree::const_query_iterator> next_open_set;
            for (auto open_it : open_set) {
                for (auto it = rtree->qbegin(bgi::intersects(open_it->first)); it != rtree->qend(); it++) {
                    if (visited_set.find(it->second) == visited_set.end()) {
                        visited_set.insert(it->second);
                        sea_water_set.insert(it->second);
                        next_open_set.push_back(it);
                    }
                }
            }
            open_set = next_open_set;
            LOGI("Sea water set size: %1% (%2% %%)",
                 sea_water_set.size(),
                 ((float)sea_water_set.size() / rtree->size() * 100.0f));
        }
        sea_water_vector.assign(sea_water_set.begin(), sea_water_set.end());
        std::sort(sea_water_vector.begin(), sea_water_vector.end());
        FILE* sea_water_dump_file = fopen(sea_water_dump_filename, "wb");
        fwrite(&sea_water_vector[0], sizeof(int), sea_water_vector.size(), sea_water_dump_file);
        fclose(sea_water_dump_file);
        sea_water_dump_file = 0;
    } else {
        size_t count = stat_buffer.st_size / sizeof(int);
        LOGI("Sea water dump count: %1%", count);
        FILE* sea_water_dump_file = fopen(sea_water_dump_filename, "rb");
        sea_water_vector.resize(count);
        auto read_sea_water_vector_count = fread(&sea_water_vector[0], sizeof(int), sea_water_vector.size(), sea_water_dump_file);
        if (read_sea_water_vector_count != sea_water_vector.size()) {
            LOGE("Sea water dump file read count error! (Expected: %1%, Actual: %2%) Aborting...",
                 sea_water_vector.size(),
                 read_sea_water_vector_count);
            abort();
        }
        LOGI("Sea water vector count: %1%", sea_water_vector.size());
    }
}

std::vector<xy32> sea_static::calculate_waypoints(const xy32 & from, const xy32 & to, int expect_land, std::shared_ptr<astarrtree::coro_context> coro) const {
    auto from_box = astarrtree::box_t_from_xy(from);
    std::vector<astarrtree::value> from_result_s;
    auto& rtree_ptr = expect_land == 0 ? water_rtree_ptr : land_rtree_ptr;
    auto check_cell = expect_land == 0 ? &sea_static::is_water : &sea_static::is_land;
    rtree_ptr->query(bgi::contains(from_box), std::back_inserter(from_result_s));
    xy32 new_from = from;
    bool new_from_sea_water = false;
    if (astarrtree::find_nearest_point_if_empty(rtree_ptr, new_from, from_box, from_result_s)) {
        new_from_sea_water = (this->*check_cell)(xy32{ new_from.x, new_from.y });
        LOGI("  'From' point changed to (%1%,%2%) [sea water=%3%]",
             new_from.x,
             new_from.y,
             new_from_sea_water);
    } else {
        new_from_sea_water = (this->*check_cell)(xy32{ new_from.x, new_from.y });
        LOGI("  'From' point (%1%,%2%) [sea water=%3%]",
             new_from.x,
             new_from.y,
             new_from_sea_water);
    }

    auto to_box = astarrtree::box_t_from_xy(to);
    std::vector<astarrtree::value> to_result_s;
    rtree_ptr->query(bgi::contains(to_box), std::back_inserter(to_result_s));
    xy32 new_to = to;
    bool new_to_sea_water = false;
    if (astarrtree::find_nearest_point_if_empty(rtree_ptr, new_to, to_box, to_result_s)) {
        new_to_sea_water = (this->*check_cell)(xy32{ new_to.x, new_to.y });
        LOGI("  'To' point changed to (%1%,%2%) [sea water=%3%]",
             new_to.x,
             new_to.y,
             new_to_sea_water);
    } else {
        new_to_sea_water = (this->*check_cell)(xy32{ new_to.x, new_to.y });
        LOGI("  'To' point (%1%,%2%) [sea water=%3%]",
             new_to.x,
             new_to.y,
             new_to_sea_water);
    }

    if (new_from_sea_water && new_to_sea_water) {
        return astarrtree::astar_rtree_memory(rtree_ptr, new_from, new_to, coro);
    } else {
        LOGE("ERROR: Both 'From' and 'To' should be in sea water to generate waypoints!");
        return std::vector<xy32>();
    }
}

std::vector<xy32> sea_static::calculate_waypoints(const sea_static_object::point & from, const sea_static_object::point & to, int expect_land, std::shared_ptr<astarrtree::coro_context> coro) const {
    xy32 fromxy;
    xy32 toxy;
    fromxy.x = from.get<0>();
    fromxy.y = from.get<1>();
    toxy.x = to.get<0>();
    toxy.y = to.get<1>();
    return calculate_waypoints(fromxy, toxy, expect_land, coro);
}

bool sea_static::is_water(const xy32& cell) const {
    auto cell_box = astarrtree::box_t_from_xy(cell);
    return water_rtree_ptr->qbegin(bgi::contains(cell_box)) != water_rtree_ptr->qend();
}

bool sea_static::is_sea_water(const xy32& cell) const {
    auto cell_box = astarrtree::box_t_from_xy(cell);
    auto it = water_rtree_ptr->qbegin(bgi::contains(cell_box));
    if (it != water_rtree_ptr->qend()) {
        return std::binary_search(sea_water_vector.begin(), sea_water_vector.end(), it->second);
    }
    return false;
}

bool sea_static::is_land(const xy32& cell) const {
    auto cell_box = astarrtree::box_t_from_xy(cell);
    return land_rtree_ptr->qbegin(bgi::contains(cell_box)) != land_rtree_ptr->qend();
}

long long sea_static::query_ts(const int xc0, const int yc0, const int view_scale) const {
    return query_ts(make_chunk_key(xc0, yc0, view_scale));
}

long long sea_static::query_ts(const LWTTLCHUNKKEY& chunk_key) const {
    const auto cit = chunk_key_ts.find(chunk_key.v);
    if (cit != chunk_key_ts.cend()) {
        return cit->second;
    }
    return time0_;
}

void sea_static::update_chunk_key_ts(int xc0, int yc0) {
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

unsigned int sea_static::query_single_cell(int xc0,
                                           int yc0,
                                           bool& land_box_valid,
                                           sea_static_object::box& land_box,
                                           bool& water_box_valid,
                                           sea_static_object::box& water_box) const {
    unsigned int attr = 0;
    const auto xy32pos = xy32{ xc0, yc0 };
    const auto box = astarrtree::box_t_from_xy(xy32pos);
    // land info
    const auto land_it = land_rtree_ptr->qbegin(bgi::contains(box));
    land_box_valid = land_it != land_rtree_ptr->qend();
    attr |= land_box_valid ? (1 << 0) : 0;
    if (land_box_valid) {
        land_box = land_it->first;
    }
    // water info
    const auto water_it = water_rtree_ptr->qbegin(bgi::contains(box));
    water_box_valid = water_it != water_rtree_ptr->qend();
    attr |= water_box_valid ? (1 << 1) : 0;
    if (water_box_valid) {
        water_box = water_it->first;
    }
    // seawater info
    attr |= is_sea_water(xy32pos) ? (1 << 2) : 0;
    return attr;
}

void sea_static::update_single_chunk_key_ts(const LWTTLCHUNKKEY& chunk_key, long long monotonic_uptime) {
    chunk_key_ts[chunk_key.v] = monotonic_uptime;
    /*auto it = chunk_key_ts.find(chunk_key.v);
    if (it != chunk_key_ts.end()) {
    it->second++;
    } else {
    chunk_key_ts[chunk_key.v] = monotonic_uptime;
    }*/
}

static bool check_not_zero_size(const xy32xy32& cell) {
    return (cell.xy1.x - cell.xy0.x) > 0
        && (cell.xy1.y - cell.xy0.y) > 0;
}

static void divide_cell_by_single_cell(const xy32xy32& cell, int xc0, int yc0, std::vector<xy32xy32>& result) {
    // [xc0, yc0] is not inside cell: no cuts
    if (cell.xy0.x > xc0 || cell.xy1.x <= xc0 || cell.xy0.y > yc0 || cell.xy1.y <= yc0) {
        LOGEP("Division should be made inside cell");
        abort();
        return;
    }
    xy32xy32 c{ { xc0, yc0 },{ xc0 + 1, yc0 + 1 } };
    int divided_count = 0;
    xy32xy32 r;
    r.xy0 = cell.xy0;
    r.xy1 = xy32{ c.xy0.x, c.xy1.y };
    if (check_not_zero_size(r)) {
        result.push_back(r);
        divided_count++;
    }
    r.xy0 = xy32{ c.xy0.x, cell.xy0.y };
    r.xy1 = xy32{ cell.xy1.x, c.xy0.y };
    if (check_not_zero_size(r)) {
        result.push_back(r);
        divided_count++;
    }
    r.xy0 = xy32{ c.xy1.x, c.xy0.y };
    r.xy1 = cell.xy1;
    if (check_not_zero_size(r)) {
        result.push_back(r);
        divided_count++;
    }
    r.xy0 = xy32{ cell.xy0.x, c.xy1.y };
    r.xy1 = xy32{ c.xy1.x, cell.xy1.y };
    if (check_not_zero_size(r)) {
        result.push_back(r);
        divided_count++;
    }
    // no cuts. only one possibility: cell === c
    if (divided_count == 0) {
        if (cell.xy0.x == c.xy0.x
            && cell.xy0.y == c.xy0.y
            && cell.xy1.x - cell.xy0.x == 1
            && cell.xy1.y - cell.xy0.y == 1) {
            return;
        } else {
            LOGE("Logic error");
            abort();
            return;
        }
    }
}

void sea_static::transform_single_cell(int xc0, int yc0, int to) {
    sea_static_object::rtree* from_rtree = to == 0 ? water_rtree_ptr : land_rtree_ptr;
    sea_static_object::rtree* to_rtree = to == 0 ? land_rtree_ptr : water_rtree_ptr;
    const char* from_str = to == 0 ? "water" : "land";
    const char* to_str = to == 0 ? "land" : "water";
    if (xc0 < 0 || xc0 >= LNGLAT_RES_WIDTH || yc0 < 0 || yc0 >= LNGLAT_RES_HEIGHT) {
        printf("Cell [%d,%d] is out of range.\n", xc0, yc0);
        return;
    }
    sea_static_object::box transform_target_cell{ { xc0,yc0 },{ xc0 + 1,yc0 + 1 } };
    std::vector<sea_static_object::value> prev_land_cells;
    to_rtree->query(bgi::contains(transform_target_cell), std::back_inserter(prev_land_cells));
    if (prev_land_cells.size() > 0) {
        printf("Cell [%d,%d] is land already. Cannot transform it to %s again.\n", xc0, yc0, to_str);
        return;
    }
    std::vector<sea_static_object::value> prev_water_cells;
    from_rtree->query(bgi::contains(transform_target_cell), std::back_inserter(prev_water_cells));
    if (prev_water_cells.size() == 0) {
        printf("Cell [%d,%d] is should be %s, but it is not found on water R-tree?!?!? Aborting...\n", xc0, yc0, from_str);
        abort();
        return;
    }
    if (prev_water_cells.size() > 1) {
        printf("Cell [%d,%d] is %s, but there are overlapping cells exists?!?!? Aborting...\n", xc0, yc0, from_str);
        abort();
        return;
    }
    // [1] Divide the overlapped water cell into subcells with cut by [xc0, yc0]. (up to four subcells created)
    auto prev_water_cell = astarrtree::xyxy_from_box_t(prev_water_cells[0].first);
    std::vector<xy32xy32> divided;
    divide_cell_by_single_cell(prev_water_cell, xc0, yc0, divided);
    // [2] Insert divided cells and remove the overlapped water cell. (insert/remove order matters)
    size_t from_rect_count = from_rtree->size();
    for (const auto& d : divided) {
        from_rect_count++;
        from_rtree->insert(std::make_pair(astarrtree::box_t_from_xyxy(d),
                                          static_cast<int>(from_rect_count)));
    }
    from_rtree->remove(prev_water_cells[0]);
    // [3] Insert single cell to land cell
    size_t to_rect_count = to_rtree->size();
    to_rect_count++;
    to_rtree->insert(std::make_pair(transform_target_cell,
                                    static_cast<int>(to_rect_count)));
    // [4] Update timestamp for affected chunks (xc0, yc0 only? entire affected area?)
    update_chunk_key_ts(xc0, yc0);
    /*for (int y = prev_water_cell.xy0.y; y < prev_water_cell.xy1.y; y++) {
        for (int x = prev_water_cell.xy0.x; x < prev_water_cell.xy1.x; x++) {
            update_chunk_key_ts(x, y);
        }
    }*/
    printf("Cell [%d,%d] successfully transformed from %s to %s.\n", xc0, yc0, from_str, to_str);
}

void sea_static::transform_single_cell_water_to_land(int xc0, int yc0) {
    transform_single_cell(xc0, yc0, 0);
}

void sea_static::transform_single_cell_land_to_water(int xc0, int yc0) {
    transform_single_cell(xc0, yc0, 1);
}
