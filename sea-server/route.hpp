#pragma once

typedef struct _xy32 xy32;

namespace ss {
    class route {
    public:
        typedef std::pair<float, float> fxfy;
        typedef std::pair<fxfy, fxfy> fxfyvxvy;

        route(const std::vector<xy32>& waypoints, int seaport1_id, int seaport2_id, int expectLand);
        void set_velocity(float v) { velocity = v; }
        void update(float delta_time);
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
    };
}
