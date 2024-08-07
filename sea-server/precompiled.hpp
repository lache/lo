#pragma once
#include <ctime>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <future>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
//#include <boost/asio/spawn.hpp>
#include <boost/thread.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/format.hpp>
#include <boost/random.hpp>
#include <boost/range.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/clamp.hpp>
#include <boost/crc.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/atomic.hpp>
#include "sealog.hpp"
#include "spinlock.hpp"
#include "lwlnglat.h"
#ifdef WIN32
#include <Windows.h>
#include <Wincrypt.h>
#endif
#include "lua.hpp"
#include "jsmn.h"

#define WORLD_MAP_PIXEL_RESOLUTION_WIDTH (172824)
#define WORLD_MAP_PIXEL_RESOLUTION_HEIGHT (86412)
#define WORLD_CIRCUMFERENCE_IN_KM (40075.0f)
#define MAX_CARGO (1000000)

inline std::chrono::steady_clock::duration get_monotonic_uptime_duration() {
    return std::chrono::steady_clock::now().time_since_epoch();
}

inline long long get_monotonic_uptime() {
    return get_monotonic_uptime_duration().count();
}

