#pragma once
#ifdef __cplusplus
extern "C" {;
#endif
const float* tilemap_uv_offset(int w, int h, const unsigned char* bitmap/*[h][w]*/, int bx, int by);
const float* tilemap_uv_scale();
#ifdef __cplusplus
}
#endif
