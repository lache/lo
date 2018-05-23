#pragma once

#define sea_render_scale (50.0f * 12)
#define earth_globe_render_scale (45.0f * 12)

#define LNGLAT_RES_WIDTH (172824)
#define LNGLAT_RES_HEIGHT (86412)
// extant decribed in R-tree pixel(cell) unit
#define LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS (16)
#define LNGLAT_SEA_PING_EXTENT_IN_DEGREES ((180.0f/LNGLAT_RES_HEIGHT)*LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS)
#define LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG (3)
#define LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT (1)

#define LNGLAT_VIEW_SCALE_PING_MAX (1 << 6) // 64

typedef struct _LWTTLLNGLAT {
    float lng;
    float lat;
} LWTTLLNGLAT;

typedef union _LWTTLCHUNKKEY {
    int v;
    struct {
        unsigned int xcc0 : 14; // right shifted xc0  200,000 pixels / chunk_size
        unsigned int ycc0 : 14; // right shifted yc0
        unsigned int view_scale_msb : 4; // 2^(view_scale_msb) == view_scale; view scale [1(2^0), 2048(2^11)]
    } bf;
} LWTTLCHUNKKEY;

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

static float cell_fx_to_lng(float fx) {
    return -180.0f + fx / LNGLAT_RES_WIDTH * 360.0f;
}

static float cell_fy_to_lat(float fy) {
    return 90.0f - fy / LNGLAT_RES_HEIGHT * 180.0f;
}

static float cell_x_to_lng(int x) {
    return cell_fx_to_lng((float)x);
}

static float cell_y_to_lat(int y) {
    return cell_fy_to_lat((float)y);
}

static float lng_to_render_coords(float lng, const LWTTLLNGLAT* center, int view_scale) {
    return (lng - center->lng) * sea_render_scale / view_scale;
}

static float lat_to_render_coords(float lat, const LWTTLLNGLAT* center, int view_scale) {
    return (lat - center->lat) * sea_render_scale / view_scale;
}

static float cell_x_to_render_coords(int x, const LWTTLLNGLAT* center, int view_scale) {
    return lng_to_render_coords(cell_x_to_lng(x), center, view_scale);
}

static float cell_y_to_render_coords(int y, const LWTTLLNGLAT* center, int view_scale) {
    return lat_to_render_coords(cell_y_to_lat(y), center, view_scale);
}

static float cell_fx_to_render_coords(float fx, const LWTTLLNGLAT* center, int view_scale) {
    return (cell_fx_to_lng(fx) - center->lng) * sea_render_scale / view_scale;
}

static float cell_fy_to_render_coords(float fy, const LWTTLLNGLAT* center, int view_scale) {
    return (cell_fy_to_lat(fy) - center->lat) * sea_render_scale / view_scale;
}

static LWTTLCHUNKKEY make_chunk_key(const int xc0_aligned, const int yc0_aligned, const int view_scale) {
    LWTTLCHUNKKEY chunk_key;
    chunk_key.bf.xcc0 = xc0_aligned >> msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
    chunk_key.bf.ycc0 = yc0_aligned >> msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
    chunk_key.bf.view_scale_msb = msb_index(view_scale);
    return chunk_key;
}

static int aligned_chunk_index(const int cell_index, const int view_scale, const int ex) {
    const auto half_cell_pixel_extent = (ex >> 1) * view_scale;
    return (cell_index + half_cell_pixel_extent) & ~(2 * half_cell_pixel_extent - 1) & ~(view_scale - 1);
}
