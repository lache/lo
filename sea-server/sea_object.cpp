#include "precompiled.hpp"
#include "sea_object.hpp"
#include "packet.h"

using namespace ss;

void sea_object::update(float delta_time) {
    /*if (remain_unloading_time > 0) {
        assert(state == SOS_UNLOADING);
        remain_unloading_time -= delta_time;
        if (remain_unloading_time <= 0) {
            remain_unloading_time = 0;
            state = SOS_LOADING;
        }
    }
    if (remain_loading_time > 0) {
        assert(state == SOS_LOADING);
        remain_loading_time -= delta_time;
        if (remain_loading_time <= 0) {
            remain_loading_time = 0;
            state = SOS_SAILING;
        }
    }*/
}

int sea_object::add_cargo(int amount, int cargo_origin_seaport_id, const xy32& cargo_origin_xy) {
    if (!(this->cargo_origin_seaport_id == 0 || this->cargo_origin_seaport_id == cargo_origin_seaport_id)) {
        LOGE("%1%: cargo origin seaport inconsistent!", __func__);
        return 0;
    }
    if (amount < 0) {
        amount = 0;
    }
    if (amount > MAX_CARGO) {
        amount = MAX_CARGO;
    }
    const auto before = cargo;
    auto after = before + amount;
    if (after > MAX_CARGO) {
        after = MAX_CARGO;
    }
    cargo = after;
    this->cargo_origin_seaport_id = cargo_origin_seaport_id;
    this->cargo_origin_xy = cargo_origin_xy;
    return after - before;
}

int sea_object::remove_cargo(int amount, int cargo_destination_seaport_id, const xy32& cargo_destination_xy) {
    if (!(this->cargo_origin_seaport_id != cargo_destination_seaport_id)) {
        LOGE("%1%: cargo destination seaport inconsistent!", __func__);
        return 0;
    }
    if (amount < 0) {
        amount = 0;
    }
    if (amount > MAX_CARGO) {
        amount = MAX_CARGO;
    }
    const auto before = cargo;
    auto after = before - amount;
    if (after < 0) {
        after = 0;
    }
    cargo = after;
    if (cargo == 0) {
        this->cargo_origin_seaport_id = 0;
        this->cargo_origin_xy = xy32{ 0,0 };
    }
    return before - after;
}

void sea_object::fill_packet(LWPTTLDYNAMICSTATEOBJECT& p) const {
    p.obj_id = id;
    p.obj_db_id = type;
    strcpy(p.guid, guid.c_str());
    if (state == SOS_LOADING) {
        strcat(p.guid, "[LOADING]");
    } else if (state == SOS_UNLOADING) {
        strcat(p.guid, "[UNLOADING]");
    }
}
