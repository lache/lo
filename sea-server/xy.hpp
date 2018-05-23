#pragma once

struct xy16 {
    short x;
    short y;
};

struct xy32 {
    int x;
    int y;
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

enum XYIB_ENTER_EXIT {
    XEE_ENTER,
    XEE_EXIT,
};

struct xy32ib {
    xy32 p;
    size_t i;
    XYIB_ENTER_EXIT ee;
};
