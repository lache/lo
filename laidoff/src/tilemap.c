#include "tilemap.h"

#define TILEMAP_GAP 0//(0.005f) //(0.005f)
#define TILEMAP_TILE_COUNT_ALONG_AXIS (4)

static const float uv_offset[16][2] = {
    { 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 2 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 3 + TILEMAP_GAP }, // 0000 [all water]
    { 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 0 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 0 + TILEMAP_GAP }, // 0001
    { 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 2 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 0 + TILEMAP_GAP }, // 0010
    { 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 1 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 0 + TILEMAP_GAP }, // 0011
    { 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 0 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 2 + TILEMAP_GAP }, // 0100
    { 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 0 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 1 + TILEMAP_GAP }, // 0101
    { 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 1 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 3 + TILEMAP_GAP }, // 0110
    { 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 3 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 3 + TILEMAP_GAP }, // 0111
    { 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 2 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 2 + TILEMAP_GAP }, // 1000
    { 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 0 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 3 + TILEMAP_GAP }, // 1001
    { 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 2 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 1 + TILEMAP_GAP }, // 1010
    { 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 3 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 1 + TILEMAP_GAP }, // 1011
    { 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 1 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 2 + TILEMAP_GAP }, // 1100
    { 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 3 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 2 + TILEMAP_GAP }, // 1101
    { 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 3 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 0 + TILEMAP_GAP }, // 1110
    { 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 1 + TILEMAP_GAP, 1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS * 1 + TILEMAP_GAP }, // 1111 [all land]
};

static const float uv_scale[] = {
    1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS - 2 * TILEMAP_GAP,
    1.0f / TILEMAP_TILE_COUNT_ALONG_AXIS - 2 * TILEMAP_GAP,
};

unsigned char tilemap_land(int w, int h, const unsigned char* bitmap/*[h][w]*/, int bx, int by) {
    if (by >= h || by < 0) {
        return 0;
    }
    if (bx >= w || bx < 0) {
        return 0;
    }
    return bitmap[by * w + bx];
}

int tilemap_uv_offset_index(int w, int h, const unsigned char* bitmap/*[h][w]*/, int bx, int by) {
    return tilemap_land(w, h, bitmap, bx - 1, by - 1) << 3
        | tilemap_land(w, h, bitmap, bx - 0, by - 1) << 2
        | tilemap_land(w, h, bitmap, bx - 1, by - 0) << 1
        | tilemap_land(w, h, bitmap, bx - 0, by - 0) << 0;
}

const float* tilemap_uv_offset(int w, int h, const unsigned char* bitmap/*[h][w]*/, int bx, int by) {
    return uv_offset[tilemap_uv_offset_index(w, h, bitmap, bx, by)];
}

const float* tilemap_uv_scale() {
    return uv_scale;
}
