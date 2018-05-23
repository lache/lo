#include "lwtimepoint.h"
#if LW_TIMESPEC_AVAILABLE
#   if LW_PLATFORM_IOS
#       include <sys/time.h>
#   endif
#else
#   include "GLFW/glfw3.h"
#endif

#if LW_PLATFORM_IOS
#include <mach/mach_time.h>
#define ORWL_NANO (+1.0E-9)
#define ORWL_GIGA UINT64_C(1000000000)

static double orwl_timebase = 0.0;
static uint64_t orwl_timestart = 0;

static struct timespec orwl_gettime(void) {
    // be more careful in a multithreaded environement
    if (!orwl_timestart) {
        mach_timebase_info_data_t tb = { 0 };
        mach_timebase_info(&tb);
        orwl_timebase = tb.numer;
        orwl_timebase /= tb.denom;
        orwl_timestart = mach_absolute_time();
    }
    struct timespec t;
    double diff = (mach_absolute_time() - orwl_timestart) * orwl_timebase;
    t.tv_sec = diff * ORWL_NANO;
    t.tv_nsec = diff - (t.tv_sec * ORWL_GIGA);
    return t;
}
#endif

void lwtimepoint_now(LWTIMEPOINT* tp) {
#if LW_TIMESPEC_AVAILABLE
    struct timespec now = {0,};
#   if LW_PLATFORM_IOS
    // clock_gettime is not supported at '< iOS 10.0'.
//    struct timeval time;
//    gettimeofday(&time, 0);
//    now.tv_sec = time.tv_sec;
//    now.tv_nsec = time.tv_usec * 1000;
    now = orwl_gettime();
#   else
    clock_gettime(CLOCK_MONOTONIC, &now);
#   endif
    tp->last_time = now;
#else
    tp->last_time = glfwGetTime();
#endif
}

double lwtimepoint_now_seconds() {
    LWTIMEPOINT tp;
    lwtimepoint_now(&tp);
#if LW_TIMESPEC_AVAILABLE
    return tp.last_time.tv_sec + tp.last_time.tv_nsec / 1e9;
#else
    return tp.last_time;
#endif
}

double lwtimepoint_diff(const LWTIMEPOINT* a, const LWTIMEPOINT* b) {
#if LW_TIMESPEC_AVAILABLE
    long nsec_diff = a->last_time.tv_nsec - b->last_time.tv_nsec;
    long sec_diff = a->last_time.tv_sec - b->last_time.tv_sec;
    
    if (nsec_diff < 0) {
        nsec_diff += 1000000000LL;
        sec_diff--;
    }
    
    return sec_diff + (double)nsec_diff / 1e9;
#else
    return a->last_time - b->last_time;
#endif
}

long lwtimepoint_get_second_portion(const LWTIMEPOINT* tp) {
#if LW_TIMESPEC_AVAILABLE
    return (long)tp->last_time.tv_sec;
#else
    return (int)tp->last_time;
#endif
}

long lwtimepoint_get_nanosecond_portion(const LWTIMEPOINT* tp) {
#if LW_TIMESPEC_AVAILABLE
    return (long)tp->last_time.tv_nsec;
#else
    return (long)((tp->last_time - (int)tp->last_time) * 1e9);
#endif
}

