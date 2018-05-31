#include "precompiled.hpp"
#include "sea_object.hpp"
#include "packet.h"

using namespace ss;

void sea_object::update(float delta_time) {
}

int sea_object::add_cargo(int amount, int cargo_origin_seaport_id, const xy32& cargo_origin_xy) {
    if (!(this->cargo_origin_seaport_id == 0 || this->cargo_origin_seaport_id == cargo_origin_seaport_id)) {
        LOGEP("cargo origin seaport inconsistent!");
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
        LOGEP("cargo destination seaport inconsistent!");
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

void sea_object::fill_packet(LWPTTLROUTEOBJECT& p) const {
    p.db_id = db_id;
    p.route_flags.land = expect_land;
    if (state == SOS_LOADING) {
        p.route_flags.loading = 1;
    } else if (state == SOS_UNLOADING) {
        p.route_flags.unloading = 1;
    } else if (state == SOS_SAILING) {
        p.route_flags.sailing = 1;
    } else if (state == SOS_BREAKDOWN) {
        p.route_flags.breakdown = 1;
    }
}
