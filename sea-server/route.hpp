#pragma once

typedef struct _xy32 xy32;

namespace ss {
    class route {
    public:
        typedef std::pair<float, float> fxfy;
        typedef std::pair<fxfy, fxfy> fxfyvxvy;

        route(const std::vector<xy32>& waypoints, int seaport1_id, int seaport2_id, int expectLand);
        void set_velocity(float v) { velocity = v; }
        float get_velocity() const { return velocity; }
        void update(float delta_time);
        void update_breakdown(float delta_time);
        fxfyvxvy get_pos(bool& finished) const;
        float get_param() const { return param; }
        int get_reversed() const { return reversed; }
        float get_left() const;
        void reverse();
        std::vector<xy32> clone_waypoints() const;
        //int get_seaport1_id() const { return seaport1_id; }
        //int get_seaport2_id() const { return seaport2_id; }
        int get_docked_seaport_id() const;
        bool get_land() const { return land; }
        float get_accum_param() const { return accum_param; }
        float get_breakdown_elapsed() const { return breakdown_elapsed; }
        void reset_breakdown_elapsed() { breakdown_elapsed = 0; }
        void update_next_breakdown_drawing_lots_param();
        bool get_breakdown_drawing_lots_raised() const { return accum_param > next_breakdown_drawing_lots_param; }
        bool get_breakdown_recovery_raised() const { return breakdown_elapsed > next_breakdown_recovery_duration; }
    private:
        std::vector<float> accum_distance;
        mutable spinlock waypoints_spinlock;
        std::vector<xy32> waypoints;
        float velocity;
        float param;
        int seaport1_id;
        int seaport2_id;
        bool reversed;
        bool land;
        float accum_param;
        float breakdown_elapsed;
        float accum_breakdown_elapsed;
        float next_breakdown_drawing_lots_param;
        float next_breakdown_recovery_duration;
        boost::random::mt19937 rng;
    };
}
