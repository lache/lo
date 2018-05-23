#pragma once

#ifdef __cplusplus
extern "C" {;
#endif
typedef struct _LWDELTATIME LWDELTATIME;

LWDELTATIME* deltatime_new();
void deltatime_destroy(LWDELTATIME** p_self);
void deltatime_tick_delta(LWDELTATIME* self, double delta_time);
void deltatime_tick(LWDELTATIME* self);
double deltatime_history_avg(const LWDELTATIME* self);
long deltatime_last_time_sec(const LWDELTATIME* self);
long deltatime_last_time_nsec(const LWDELTATIME* self);
double deltatime_delta_time(const LWDELTATIME* self);
void deltatime_set_to_now(LWDELTATIME* self);
#ifdef __cplusplus
};
#endif
