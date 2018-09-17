#include "precompiled.hpp"
#include "sea.hpp"
#include "route.hpp"
#include "udp_admin_server.hpp"
#include "seaport.hpp"
#include "udp_server.hpp"
#include "adminmessage.h"
using namespace ss;

void init_lua(lua_State* L, const char* lua_filename);

sea::sea(boost::asio::io_service& io_service, std::shared_ptr<lua_State> lua_state_instance)
    : io_service(io_service)
    , res_width(WORLD_MAP_PIXEL_RESOLUTION_WIDTH)
    , res_height(WORLD_MAP_PIXEL_RESOLUTION_HEIGHT)
    , lua_state_instance(lua_state_instance) {
    init();
}

sea::~sea() {
}

void sea::init() {
    init_lua(lua_state_instance.get(), "assets/l/sea.lua");
}

float sea::lng_to_xc(float lng) const {
    return res_width / 2.0f + lng / 180.0f * res_width / 2.0f;
}

float sea::lat_to_yc(float lat) const {
    return res_height / 2.0f - lat / 90.0f * res_height / 2.0f;
}

int sea::spawn(int expected_db_id, float x, float y, float w, float h, int expect_land, int template_id) {
    auto obj = get_by_db_id(expected_db_id);
    if (obj) {
        LOGEP("Spawn duplicated ID %|| requested; returning the previous instance", expected_db_id);
        return obj->get_db_id();
    }
    box b(point(x, y), point(x + w, y + h));
    auto rtree_value = std::make_pair(b, expected_db_id);
    auto sobj = std::shared_ptr<sea_object>(new sea_object(expected_db_id,
                                                           x,
                                                           y,
                                                           w,
                                                           h,
                                                           rtree_value,
                                                           expect_land,
                                                           template_id));
    sea_objects.emplace(std::make_pair(expected_db_id,
                                       sobj));
    rtree.insert(rtree_value);
    return expected_db_id;
}

int sea::spawn(const spawn_ship_command& spawn_ship_cmd) {
    return spawn(spawn_ship_cmd.expected_db_id,
                 spawn_ship_cmd.x,
                 spawn_ship_cmd.y,
                 1,
                 1,
                 spawn_ship_cmd.expect_land,
                 spawn_ship_cmd.ship_template_id);
}

void sea::despawn(int db_id) {
    auto obj = get_by_db_id(db_id);
    if (obj) {
        rtree.remove(obj->get_rtree_value());
        sea_objects.erase(obj->get_db_id());
        obj = nullptr;
    } else {
        LOGEP("Cannot get sea object ID %||!", db_id);
    }
}

SEA_OBJECT_STATE sea::get_object_state(int id) const {
    auto it = sea_objects.find(id);
    if (it != sea_objects.end()) {
        return it->second->get_state();
    } else {
        LOGEP("Sea object not found corresponding to id %1%", id);
    }
    return SOS_ERROR;
}

void sea::set_object_state(int id, SEA_OBJECT_STATE state) {
    auto it = sea_objects.find(id);
    if (it != sea_objects.end()) {
        it->second->set_state(state);
    } else {
        LOGEP("Sea object not found corresponding to id %1%", id);
    }
}

std::shared_ptr<sea_object> sea::get_by_db_id(int db_id) {
    auto it = sea_objects.find(db_id);
    if (it != sea_objects.end()) {
        if (it->second->get_db_id() != db_id) {
            LOGEP("CRITICAL ERROR - sea_objects key inconsistent!");
            return std::shared_ptr<sea_object>();
        }
        return it->second;
    } else {
        LOGE("Sea object DB ID %1% not found", db_id);
    }
    return std::shared_ptr<sea_object>();
}

int sea::teleport_to(int id, float x, float y, float vx, float vy) {
    auto it = sea_objects.find(id);
    if (it != sea_objects.end()) {
        rtree.remove(it->second->get_rtree_value());
        it->second->set_xy(x, y);
        it->second->set_velocity(vx, vy);
        rtree.insert(it->second->get_rtree_value());
        return 0;
    } else {
        LOGEP("Sea object not found corresponding to id %1%", id);
        return -1;
    }
}

void sea::query_near_lng_lat_to_packet(float lng, float lat, float ex_lng, float ex_lat, std::vector<sea_object>& sop_list) const {
    query_near_to_packet(lng_to_xc(lng), lat_to_yc(lat), ex_lng, ex_lat, sop_list);
}

void sea::query_near_to_packet(float xc, float yc, float ex_lng, float ex_lat, std::vector<sea_object>& sop_list) const {
    auto id_list = query_tree(xc, yc, ex_lng, ex_lat);
    sop_list.resize(id_list.size());
    std::size_t i = 0;
    for (int id : id_list) {
        auto f = sea_objects.find(id);
        if (f != sea_objects.end()) {
            f->second->fill_sop(sop_list[i]);
            i++;
        }
    }
}

std::vector<int> sea::query_tree(float xc, float yc, float ex_lng, float ex_lat) const {
    box query_box(point(xc - ex_lng / 2, yc - ex_lat / 2), point(xc + ex_lng / 2, yc + ex_lat / 2));
    std::vector<value> result_s;
    std::vector<int> id_list;
    rtree.query(bgi::intersects(query_box), std::back_inserter(result_s));
    for (value const& v : result_s) {
        id_list.push_back(v.second);
    }
    return id_list;
}

void sea::update(float delta_time) {
    for (auto& obj : sea_objects) {
        obj.second->update(delta_time);
        // lua hook
        const auto sea_object_id = obj.first;
        lua_getglobal(lua_state_instance.get(), "sea_object_update");
        lua_pushnumber(lua_state_instance.get(), sea_object_id);
        if (lua_pcall(lua_state_instance.get(), 1/*arguments*/, 0/*result*/, 0)) {
            LOGEP("error: %1%", lua_tostring(lua_state_instance.get(), -1));
        }
    }
}

bool sea::update_route(float delta_time,
                       int db_id,
                       std::shared_ptr<route> r,
                       std::shared_ptr<seaport> sp,
                       udp_server* us) {
    if (!r) {
        // nothing to do
        return true;
    }
    auto finished = false;
    auto pos = r->get_pos(finished);
    auto state = get_object_state(db_id);
    if (state == SOS_ERROR) {
        return false;
    }
    if (state == SOS_SAILING) {
        r->update(delta_time);
        auto dlen = sqrtf(pos.second.first * pos.second.first + pos.second.second * pos.second.second);
        if (dlen) {
            teleport_to(db_id, pos.first.first, pos.first.second, pos.second.first / dlen, pos.second.second / dlen);
        } else {
            teleport_to(db_id, pos.first.first, pos.first.second, 0, 0);
        }
        if (r->get_breakdown_drawing_lots_raised()) {
            if (rand() % 2 == 0) {
                set_object_state(db_id, SOS_BREAKDOWN);
                us->gold_used(static_cast<int>(pos.first.first), static_cast<int>(pos.first.second), 10);
            } else {
                r->update_next_breakdown_drawing_lots_param();
            }
        }
    }
    if (state == SOS_BREAKDOWN) {
        r->update_breakdown(delta_time);
        if (r->get_breakdown_recovery_raised()) {
            set_object_state(db_id, SOS_SAILING);
            r->reset_breakdown_elapsed();
            r->update_next_breakdown_drawing_lots_param();
        }
    }
    if (finished && state == SOS_SAILING) {
        // ship docked at seaport2
        // start unloading...
        auto obj = get_by_db_id(db_id);
        if (obj == nullptr) {
            LOGE("Cannot find sea object with ID %1%", db_id);
            return false;
        }
        obj->set_velocity(0, 0);
        obj->set_state(SOS_UNLOADING);
        float x, y;
        obj->get_xy(x, y);
        const auto unloading_time = boost::posix_time::milliseconds(5000);
        LOGIx("S %1% {%2%,%3%}: start unloading from SP %4%... (%5% ms)",
              db_id,
              x,
              y,
              r->get_docked_seaport_id(),
              unloading_time.total_milliseconds());
        std::shared_ptr<boost::asio::deadline_timer> t(new boost::asio::deadline_timer(io_service, unloading_time));
        t->async_wait([this, db_id, t, sp, r, us](const boost::system::error_code& error) {
            if (!error) {
                auto obj = get_by_db_id(db_id);
                if (obj == nullptr) {
                    LOGE("Cannot find sea object with ID %1%", db_id);
                    return;
                }
                float x, y;
                obj->get_xy(x, y);
                const auto unloaded_cargo = obj->remove_cargo(MAX_CARGO,
                                                              r->get_docked_seaport_id(),
                                                              xy32{ static_cast<int>(x), static_cast<int>(y) });
                LOGIx("S %1% {%2%,%3%}: unloading finished. %4% cargo(s) unloaded.",
                      db_id,
                      x,
                      y,
                      unloaded_cargo);
                const auto actual_unloaded = sp->add_cargo(r->get_docked_seaport_id(), unloaded_cargo, false);
                cargo_notifications.emplace_back(cargo_notification{
                    static_cast<int>(x),
                    static_cast<int>(y),
                    static_cast<int>(x),
                    static_cast<int>(y),
                    actual_unloaded,
                    LTCNT_UNLOADED});
                const auto sp_point = sp->get_seaport_point(r->get_docked_seaport_id());
                us->gold_earned(sp_point.get<0>(), sp_point.get<1>(), 1);
                obj->set_state(SOS_LOADING);
                const auto loading_time = boost::posix_time::milliseconds(5000);
                t->expires_at(t->expires_at() + loading_time);
                LOGIx("S %1% {%2%,%3%}: start loading from SP %4%... (%5% ms)",
                      db_id,
                      x,
                      y,
                      r->get_docked_seaport_id(),
                      loading_time.total_milliseconds());
                t->async_wait([this, db_id, t, sp, r](const boost::system::error_code& error) {
                    if (!error) {
                        auto obj = get_by_db_id(db_id);
                        if (obj == nullptr) {
                            LOGE("Cannot find sea object with ID %1%", db_id);
                            return;
                        }
                        const auto loaded_cargo = sp->remove_cargo(r->get_docked_seaport_id(), 10, false);
                        float x, y;
                        obj->get_xy(x, y);
                        const auto actual_loaded = obj->add_cargo(loaded_cargo,
                                                                  r->get_docked_seaport_id(),
                                                                  xy32{ static_cast<int>(x), static_cast<int>(y) });
                        cargo_notifications.emplace_back(cargo_notification{
                            static_cast<int>(x),
                            static_cast<int>(y),
                            static_cast<int>(x),
                            static_cast<int>(y),
                            actual_loaded,
                            LTCNT_LOADED });
                        LOGIx("S %1% {%2%,%3%}: loading finished. %4% cargo(s) loaded.",
                              db_id,
                              x,
                              y,
                              loaded_cargo);
                        uas->send_arrival(obj->get_db_id());
                        // reversing the route
                        r->reverse();
                        LOGIx("S %1% {%2%,%3%}: reversing the route.",
                              db_id,
                              x,
                              y);
                        // sail again
                        obj->set_state(SOS_SAILING);
                    } else {
                        LOGE(error.message());
                    }
                });
            } else {
                LOGE(error.message());
            }
        });
    }
    return true;
}

std::vector<cargo_notification>&& sea::flush_cargo_notifications() {
    return std::move(cargo_notifications);
}
