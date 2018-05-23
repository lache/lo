#include "precompiled.hpp"
#include "route.hpp"
#include "xy.hpp"

using namespace ss;

float distance_xy(const xy32& a, const xy32& b) {
    return sqrtf(static_cast<float>((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)));
}

route::route(const std::vector<xy32>& waypoints, int seaport1_id, int seaport2_id)
    : waypoints(waypoints)
    , velocity(0)
    , param(0)
    , seaport1_id(seaport1_id)
    , seaport2_id(seaport2_id)
    , reversed(false) {
    if (waypoints.size() < 2) {
        LOGE("route created with less than two waypoints...");
    } else {
        float dist = 0;
        accum_distance.push_back(dist);
        for (size_t i = 0; i < waypoints.size() - 1; i++) {
            dist += distance_xy(waypoints[i], waypoints[i + 1]);
            accum_distance.push_back(dist);
        }
    }
}

void route::update(float delta_time) {
    if (reversed == false) {
        param += velocity * delta_time;
    } else {
        param -= velocity * delta_time;
    }
}

route::fxfyvxvy route::get_pos(bool& finished) const {
    if (accum_distance.size() == 0) {
        return route::fxfyvxvy{ { 0.0f, 0.0f }, { 0.0f, 0.0f } };
    }
    auto it = std::lower_bound(accum_distance.begin(), accum_distance.end(), param);
    auto px = 0.0f, py = 0.0f, dx = 0.0f, dy = 0.0f;
    finished = false;
    if (it == accum_distance.begin()) {
        px = static_cast<float>(waypoints.begin()->x);
        py = static_cast<float>(waypoints.begin()->y);
        dx = static_cast<float>((waypoints.begin() + 1)->x) - px;
        dy = static_cast<float>((waypoints.begin() + 1)->y) - py;
        // begin position is finished position if reversed sailing
        if (param <= 0 && reversed == true) {
            finished = true;
        }
    } else if (it == accum_distance.end()) {
        px = static_cast<float>(waypoints.rbegin()->x);
        py = static_cast<float>(waypoints.rbegin()->y);
        dx = px - static_cast<float>((waypoints.rbegin() + 1)->x);
        dy = py - static_cast<float>((waypoints.rbegin() + 1)->y);
        // end position is finished position if ordinary sailing
        if (reversed == false) {
            finished = true;
        }
    } else {
        auto it_idx = it - accum_distance.begin();
        auto wp1 = waypoints[it_idx - 1];
        auto wp2 = waypoints[it_idx];
        auto d1 = accum_distance[it_idx - 1];
        auto d2 = accum_distance[it_idx];
        auto r = (param - d1) / (d2 - d1);
        if (r < 0) r = 0;
        if (r > 1) r = 1;
        dx = static_cast<float>(wp2.x - wp1.x);
        dy = static_cast<float>(wp2.y - wp1.y);
        px = wp1.x + dx * r;
        py = wp1.y + dy * r;
    }
    return std::make_pair(std::make_pair(px, py), std::make_pair(dx, dy));
}

float route::get_left() const {
    return std::max(0.0f, *accum_distance.rbegin() - param);
}

void route::reverse() {
    reversed = !reversed;
    
    /*
    waypoints_spinlock.lock();
    std::reverse(waypoints.begin(), waypoints.end());
    reversed = !reversed;
    auto total_length = *accum_distance.rbegin();
    for (size_t i = 0; i < accum_distance.size(); i++) {
        accum_distance[i] = total_length - accum_distance[i];
    }
    std::reverse(accum_distance.begin(), accum_distance.end());
    param = total_length - param;
    std::swap(seaport1_id, seaport2_id);
    waypoints_spinlock.unlock();
    */
}

std::vector<xy32> route::clone_waypoints() const {
    waypoints_spinlock.lock();
    auto r = waypoints;
    waypoints_spinlock.unlock();
    return r;
}

int route::get_docked_seaport_id() const {
    if (reversed) {
        return seaport1_id;
    } else {
        return seaport2_id;
    }
}
