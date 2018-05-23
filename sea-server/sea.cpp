#include "precompiled.hpp"
#include "sea.hpp"
#include "route.hpp"
#include "udp_admin_server.hpp"
#include "seaport.hpp"
#include "udp_server.hpp"
using namespace ss;

sea::sea(boost::asio::io_service& io_service)
    : io_service(io_service)
    , res_width(WORLD_MAP_PIXEL_RESOLUTION_WIDTH)
    , res_height(WORLD_MAP_PIXEL_RESOLUTION_HEIGHT)
    , km_per_cell(WORLD_CIRCUMFERENCE_IN_KM / res_width) {
}

float sea::lng_to_xc(float lng) const {
    return res_width / 2.0f + lng / 180.0f * res_width / 2.0f;
}

float sea::lat_to_yc(float lat) const {
    return res_height / 2.0f - lat / 90.0f * res_height / 2.0f;
}

void sea::populate_test() {
    boost::random::mt19937 rng;
    boost::random::uniform_real_distribution<float> random_point(-2500, 2500);
    for (int i = 0; i < 0; i++) {
        float x = random_point(rng);
        float y = random_point(rng);
        spawn(i + 1, x, y, 1, 1);
        if ((i + 1) % 5000 == 0) {
            LOGI("Spawning %d...", i + 1);
        }
    }
    //travel_to("Test A", 129.393311f + 1.0f, 35.4739418f, 0.01f);
    LOGI("Spawning finished.");
    std::vector<sea_object> sop_list;
    query_near_to_packet(0, 0, 10, 10, sop_list);
}

int sea::spawn(const char* guid, int type, float x, float y, float w, float h) {
    auto it = sea_guid_to_id.find(guid);
    if (it != sea_guid_to_id.end()) {
        int id = it->second;
        auto it2 = sea_objects.find(id);
        if (it2 != sea_objects.end()) {
            return id;
        } else {
            LOGE("Spawn: Orphan entry found in sea_guid_to_id. (guid=%1%) Removing and respawn...", guid);
            sea_guid_to_id.erase(it);
            return spawn(guid, type, x, y, w, h);
        }
    } else {
        int id = spawn(type, x, y, w, h);
        sea_objects.find(id)->second.set_guid(guid);
        sea_guid_to_id[guid] = id;
        return id;
    }
}

int sea::spawn(int type, float x, float y, float w, float h) {
    int id = static_cast<int>(sea_objects.size()) + 1;
    box b(point(x, y), point(x + w, y + h));
    auto rtree_value = std::make_pair(b, id);
    sea_objects.emplace(std::make_pair(id, sea_object(id, type, x, y, w, h, rtree_value)));
    sea_object_id_by_type[type] = id;
    rtree.insert(rtree_value);
    return id;
}

void sea::despawn(int type) {
    auto it = sea_object_id_by_type.find(type);
    if (it != sea_object_id_by_type.end()) {
        auto obj = get_object(it->second);
        if (obj != nullptr) {
            rtree.remove(obj->get_rtree_value());
        } else {
            LOGE("Cannot get sea object of type %1%!", it->second);
        }
        sea_objects.erase(it->second);
        obj = nullptr;
        sea_object_id_by_type.erase(it);
        it = sea_object_id_by_type.end();
    }
}

void sea::travel_to(const char* guid, float x, float y, float v) {
    auto it = sea_guid_to_id.find(guid);
    if (it != sea_guid_to_id.end()) {
        auto it2 = sea_objects.find(it->second);
        if (it2 != sea_objects.end()) {
            rtree.remove(it2->second.get_rtree_value());
            float cx = 0, cy = 0; // current position
            it2->second.get_xy(cx, cy);
            float dx = x - cx;
            float dy = y - cy;
            float dlen = sqrtf(dx * dx + dy * dy);
            if (dlen > 1e-3) {
                it2->second.set_velocity(dx / dlen * v, dy / dlen * v);
            } else {
                it2->second.set_velocity(0, 0);
            }
            it2->second.set_destination(x, y);
            rtree.insert(it2->second.get_rtree_value());
        } else {
            LOGE("Sea object not found corresponding to guid %1%", guid);
        }
    } else {
        LOGE("Sea object ID not found corresponding to guid %1%", guid);
    }
}

SEA_OBJECT_STATE sea::get_object_state(int id) const {
    auto it = sea_objects.find(id);
    if (it != sea_objects.end()) {
        return it->second.get_state();
    } else {
        LOGE("Sea object not found corresponding to id %1%", id);
    }
    return SOS_ERROR;
}

void sea::set_object_state(int id, SEA_OBJECT_STATE state) {
    auto it = sea_objects.find(id);
    if (it != sea_objects.end()) {
        it->second.set_state(state);
    } else {
        LOGE("Sea object not found corresponding to id %1%", id);
    }
}

sea_object* sea::get_object_by_type(int type) {
    auto it = sea_object_id_by_type.find(type);
    if (it != sea_object_id_by_type.end()) {
        return get_object(it->second);
    } else {
        LOGE("Sea object ID cannot be found using type %1%", type);
    }
    return nullptr;
}

sea_object* sea::get_object(int id) {
    auto it = sea_objects.find(id);
    if (it != sea_objects.end()) {
        return &it->second;
    } else {
        LOGE("Sea object not found corresponding to id %1%", id);
    }
    return nullptr;
}

void sea::teleport_to(int id, float x, float y, float vx, float vy) {
    auto it = sea_objects.find(id);
    if (it != sea_objects.end()) {
        rtree.remove(it->second.get_rtree_value());
        it->second.set_xy(x, y);
        it->second.set_velocity(vx, vy);
        rtree.insert(it->second.get_rtree_value());
    } else {
        LOGE("Sea object not found corresponding to id %1%", id);
    }
}

void sea::teleport_to(const char* guid, float x, float y, float vx, float vy) {
    auto it = sea_guid_to_id.find(guid);
    if (it != sea_guid_to_id.end()) {
        teleport_to(it->second, x, y, vx, vy);
    } else {
        LOGE("Sea object ID not found corresponding to guid %1%", guid);
    }
}

void sea::teleport_by(const char* guid, float dx, float dy) {
    auto it = sea_guid_to_id.find(guid);
    if (it != sea_guid_to_id.end()) {
        auto it2 = sea_objects.find(it->second);
        if (it2 != sea_objects.end()) {
            rtree.remove(it2->second.get_rtree_value());
            it2->second.translate_xy(dx, dy);
            rtree.insert(it2->second.get_rtree_value());
        } else {
            LOGE("Sea object not found corresponding to guid %1%", guid);
        }
    } else {
        LOGE("Sea object ID not found corresponding to guid %1%", guid);
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
            f->second.fill_sop(sop_list[i]);
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
    for (const auto& it : sea_guid_to_id) {
        const auto& it2 = sea_objects.find(it.second);
        if (it2 != sea_objects.cend()) {
            float vx = 0, vy = 0;
            it2->second.get_velocity(vx, vy);
            if (vx || vy) {
                if (it2->second.get_distance_to_destination() > 0.001f) {
                    teleport_by(it.first.c_str(), vx * delta_time, vy * delta_time);
                } else {
                    it2->second.set_velocity(0, 0);
                }
            }
        }
    }
    for (auto& obj : sea_objects) {
        obj.second.update(delta_time);
    }
}

bool sea::update_route(float delta_time,
                       int id,
                       std::shared_ptr<route> r,
                       std::shared_ptr<seaport> sp,
                       udp_server* us) {
    if (!r) {
        // nothing to do
        return true;
    }
    auto finished = false;
    auto pos = r->get_pos(finished);
    auto state = get_object_state(id);
    if (state == SOS_ERROR) {
        return false;
    }
    if (state == SOS_SAILING) {
        r->update(delta_time);
        auto dlen = sqrtf(pos.second.first * pos.second.first + pos.second.second * pos.second.second);
        if (dlen) {
            teleport_to(id, pos.first.first, pos.first.second, pos.second.first / dlen, pos.second.second / dlen);
        } else {
            teleport_to(id, pos.first.first, pos.first.second, 0, 0);
        }
    }
    if (finished && state == SOS_SAILING) {
        // ship docked at seaport2
        // start unloading...
        auto obj = get_object(id);
        if (obj == nullptr) {
            LOGE("Cannot find sea object with ID %1%", id);
            return false;
        }
        obj->set_velocity(0, 0);
        obj->set_state(SOS_UNLOADING);
        float x, y;
        obj->get_xy(x, y);
        const auto unloading_time = boost::posix_time::milliseconds(5000);
        LOGIx("S %1% {%2%,%3%}: start unloading from SP %4%... (%5% ms)",
              id,
              x,
              y,
              r->get_docked_seaport_id(),
              unloading_time.total_milliseconds());
        std::shared_ptr<boost::asio::deadline_timer> t(new boost::asio::deadline_timer(io_service, unloading_time));
        t->async_wait([this, id, t, sp, r, us](const boost::system::error_code& error) {
            if (!error) {
                auto obj = get_object(id);
                if (obj == nullptr) {
                    LOGE("Cannot find sea object with ID %1%", id);
                    return;
                }
                float x, y;
                obj->get_xy(x, y);
                const auto unloaded_cargo = obj->remove_cargo(MAX_CARGO,
                                                              r->get_docked_seaport_id(),
                                                              xy32{ static_cast<int>(x), static_cast<int>(y) });
                LOGIx("S %1% {%2%,%3%}: unloading finished. %4% cargo(s) unloaded.",
                      id,
                      x,
                      y,
                      unloaded_cargo);
                sp->add_cargo(r->get_docked_seaport_id(), unloaded_cargo, false);
                const auto sp_point = sp->get_seaport_point(r->get_docked_seaport_id());
                us->notify_to_client_gold_earned(sp_point.get<0>(), sp_point.get<1>(), 1);
                obj->set_state(SOS_LOADING);
                const auto loading_time = boost::posix_time::milliseconds(5000);
                t->expires_at(t->expires_at() + loading_time);
                LOGIx("S %1% {%2%,%3%}: start loading from SP %4%... (%5% ms)",
                      id,
                      x,
                      y,
                      r->get_docked_seaport_id(),
                      loading_time.total_milliseconds());
                t->async_wait([this, id, t, sp, r](const boost::system::error_code& error) {
                    if (!error) {
                        auto obj = get_object(id);
                        if (obj == nullptr) {
                            LOGE("Cannot find sea object with ID %1%", id);
                            return;
                        }
                        const auto loaded_cargo = sp->remove_cargo(r->get_docked_seaport_id(), 10, false);
                        float x, y;
                        obj->get_xy(x, y);
                        obj->add_cargo(loaded_cargo,
                                       r->get_docked_seaport_id(),
                                       xy32{ static_cast<int>(x), static_cast<int>(y) });

                        LOGIx("S %1% {%2%,%3%}: loading finished. %4% cargo(s) loaded.",
                              id,
                              x,
                              y,
                              loaded_cargo);
                        uas->send_arrival(obj->get_type());
                        // reversing the route
                        r->reverse();
                        LOGIx("S %1% {%2%,%3%}: reversing the route.",
                              id,
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
