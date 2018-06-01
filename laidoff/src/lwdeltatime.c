#include <stdlib.h>
#include "lwdeltatime.h"
#include "lwtimepoint.h"
#include "lwmacro.h"

#define MAX_DELTA_TIME_HISTORY (60)

typedef struct _LWDELTATIME {
	LWTIMEPOINT last_time;
	double delta_time;
	double delta_time_history[MAX_DELTA_TIME_HISTORY];
	int delta_time_history_index;
} LWDELTATIME;

LWDELTATIME* deltatime_new() {
	return calloc(1, sizeof(LWDELTATIME));
}

void deltatime_destroy(LWDELTATIME** p_self) {
	free(*p_self);
	*p_self = 0;
}

void deltatime_tick_delta(LWDELTATIME* self, double delta_time) {
	LWTIMEPOINT cur_time;
	lwtimepoint_now(&cur_time);

	self->delta_time = delta_time;
	self->delta_time_history[(self->delta_time_history_index++) % MAX_DELTA_TIME_HISTORY] = self->delta_time;
	self->last_time = cur_time;
}

void deltatime_tick(LWDELTATIME* self) {
	LWTIMEPOINT cur_time;
	lwtimepoint_now(&cur_time);
	double delta_time = lwtimepoint_diff(&cur_time, &self->last_time);
	deltatime_tick_delta(self, delta_time);
}

double deltatime_history_avg(const LWDELTATIME* self) {
	double delta_time_sum = 0;
	const int cnt = LWMIN(self->delta_time_history_index, MAX_DELTA_TIME_HISTORY);
	for (int i = 0; i < cnt; i++) {
		delta_time_sum += self->delta_time_history[i];
	}
	if (cnt > 0) {
		return delta_time_sum / cnt;
	}
	return 0;
}

long deltatime_last_time_sec(const LWDELTATIME* self) {
	return lwtimepoint_get_second_portion(&self->last_time);
}

long deltatime_last_time_nsec(const LWDELTATIME* self) {
	return lwtimepoint_get_nanosecond_portion(&self->last_time);
}

double deltatime_delta_time(const LWDELTATIME* self) {
	if (self == 0) {
		return 0;
	}
	return self->delta_time;
}

void deltatime_set_to_now(LWDELTATIME* self) {
	lwtimepoint_now(&self->last_time);
}
