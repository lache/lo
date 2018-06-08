#pragma once
#include "packet.h"
struct xy16 {
    short x;
    short y;
};

struct ifxy32 {
    int i;
    float x;
    float y;
};

struct ixy32 {
    int i;
    int x;
    int y;
};

inline bool operator < (const xy32& lhs, const xy32& rhs) {
    if (lhs.x == rhs.x) {
        return lhs.y < rhs.y;
    }
    return lhs.x < rhs.x;
}

struct xy16xy16 {
    xy16 xy0;
    xy16 xy1;
};

struct xy32xy32 {
    xy32 xy0;
    xy32 xy1;
};

struct xy32xy32xy32 {
    xy32xy32 box;
    xy32 point;
};

struct xy32i {
    xy32 p;
    size_t i;
};

enum XYIBB_ENTER_EXIT {
    XEE_ENTER = 0,
    XEE_EXIT = 1,
};

enum XYIBB_NEIGHBOR_COVERAGE {
    XNC_NO_COVERAGE = 0,
    XNC_COVER_NEAREST = 1 << 0,
    XNC_COVER_TO_FIRST = 1 << 1,
    XNC_COVER_TO_LAST = 1 << 2,
    XNC_COVER_ALL = XNC_COVER_NEAREST | XNC_COVER_TO_FIRST | XNC_COVER_TO_LAST,
};

struct xy32ibb {
    xy32 p;
    size_t i;
    XYIBB_ENTER_EXIT ee;
    int nc;
};
