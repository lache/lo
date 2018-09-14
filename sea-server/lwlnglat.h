#pragma once
//
// Shared constants between server and client
//
#define LNGLAT_RES_WIDTH (172824)
#define LNGLAT_RES_HEIGHT (86412)
// extant decribed in R-tree pixel(cell) unit
#define LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS (16)
#define LNGLAT_SEA_PING_EXTENT_IN_DEGREES ((180.0f/LNGLAT_RES_HEIGHT)*LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS)
#define LNGLAT_VIEW_SCALE_PING_MAX (1 << 6) // 64

//
// Shared structs between server and client
//
typedef union _LWTTLCHUNKKEY {
    int v;
    struct {
        unsigned int xcc0 : 14; // right shifted xc0  200,000 pixels / chunk_size
        unsigned int ycc0 : 14; // right shifted yc0
        unsigned int view_scale_msb : 4; // 2^(view_scale_msb) == view_scale; view scale [1(2^0), 2048(2^11)]
    } bf;
} LWTTLCHUNKKEY;

//
// Shared functions between server and client
//
#ifdef __GNUC__
int __builtin_ctz(unsigned int x);
static int msb_index(unsigned int v) {
    return __builtin_ctz(v);
}
#else
// MSVC perhaps...
#include <intrin.h> 
#pragma intrinsic(_BitScanReverse)
static int msb_index(unsigned int v) {
    unsigned long view_scale_msb_index = 0;
    _BitScanReverse(&view_scale_msb_index, (unsigned long)v);
    return (int)view_scale_msb_index;
}
#endif

static inline LWTTLCHUNKKEY make_chunk_key(const int xc0_aligned, const int yc0_aligned, const int view_scale) {
    LWTTLCHUNKKEY chunk_key;
    chunk_key.bf.xcc0 = xc0_aligned >> msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
    chunk_key.bf.ycc0 = yc0_aligned >> msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
    chunk_key.bf.view_scale_msb = msb_index(view_scale);
    return chunk_key;
}

static inline int aligned_chunk_index(const int cell_index, const int view_scale, const int ex) {
    const int half_cell_pixel_extent = (ex >> 1) * view_scale;
    return (cell_index + half_cell_pixel_extent) & ~(2 * half_cell_pixel_extent - 1) & ~(view_scale - 1);
}
