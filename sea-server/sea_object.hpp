#pragma once

#include "container.hpp"
#include "sea_object.hpp"
#include "xy.hpp"

typedef struct _LWPTTLROUTEOBJECT LWPTTLROUTEOBJECT;

namespace ss {
    namespace bg = boost::geometry;
    namespace bgi = boost::geometry::index;

    enum SEA_OBJECT_STATE {
        SOS_NOT_SET,
        SOS_SAILING,
        SOS_LOADING,
        SOS_UNLOADING,
        SOS_BREAKDOWN,
        SOS_ERROR,
    };

    class sea_object {
        typedef bg::model::point<float, 2, bg::cs::cartesian> point;
        typedef bg::model::box<point> box;
        typedef std::pair<box, int> value;
    public:
        explicit sea_object()
            : db_id(0)
            , fx(0)
            , fy(0)
            , fw(0)
            , fh(0)
            , fvx(0)
            , fvy(0)
            , state(SOS_NOT_SET)
            , remain_loading_time(0)
            , cargo(0)
            , cargo_origin_seaport_id(-1)
            , cargo_origin_xy({ 0,0 }) {
        }
        sea_object(int db_id, float fx, float fy, float fw, float fh, const value& rtree_value, int expect_land, int template_id)
            : db_id(db_id)
            , fx(fx)
            , fy(fy)
            , fw(fw)
            , fh(fh)
            , fvx(0)
            , fvy(0)
            , rtree_value(rtree_value)
            , state(SOS_SAILING)
            , remain_loading_time(0)
            , cargo(0)
            , cargo_origin_seaport_id(-1)
            , cargo_origin_xy({ 0,0 })
            , expect_land(expect_land)
            , template_id(template_id) {
        }
        void fill_sop(sea_object& sop) const {
            sop = *this;
        }
        void translate_xy(float dxv, float dyv) {
            set_xy(fx + dxv, fy + dyv);
        }
        void get_xy(float& fx, float& fy) const {
            fx = this->fx;
            fy = this->fy;
        }
        void set_xy(float fx, float fy) {
            this->fx = fx;
            this->fy = fy;
            rtree_value.first.min_corner().set<0>(fx);
            rtree_value.first.min_corner().set<1>(fy);
            rtree_value.first.max_corner().set<0>(fx + fw);
            rtree_value.first.max_corner().set<1>(fy + fh);
        }
        void set_velocity(float fvx, float fvy) {
            this->fvx = fvx;
            this->fvy = fvy;
        }
        void get_velocity(float& fvx, float& fvy) const {
            fvx = this->fvx;
            fvy = this->fvy;
        }
        float get_distance_to_destination() const {
            return sqrtf((dest_fx - fx) * (dest_fx - fx) + (dest_fy - fy) * (dest_fy - fy));
        }
        const value& get_rtree_value() const { return rtree_value; }
        void set_state(SEA_OBJECT_STATE state) { this->state = state; }
        SEA_OBJECT_STATE get_state() const { return state; }
        void set_remain_unloading_time(float remain_unloading_time) {
            this->remain_unloading_time = remain_unloading_time;
        }
        void set_remain_loading_time(float remain_loading_time) {
            this->remain_loading_time = remain_loading_time;
        }
        void update(float delta_time);
        int get_db_id() const { return db_id; }
        int add_cargo(int amount, int cargo_origin_seaport_id, const xy32& cargo_origin_xy);
        int remove_cargo(int amount, int cargo_destination_seaport_id, const xy32& cargo_destination_xy);
        int get_cargo() const { return cargo; }
        void fill_packet(LWPTTLROUTEOBJECT& p) const;
    private:
        int db_id;
        float fx, fy;
        float fw, fh;
        float fvx, fvy;
        float dest_fx, dest_fy;
        value rtree_value;
        SEA_OBJECT_STATE state;
        float remain_unloading_time;
        float remain_loading_time;
        int cargo;
        int cargo_origin_seaport_id;
        xy32 cargo_origin_xy;
        int expect_land;
        int template_id;
    };
}
