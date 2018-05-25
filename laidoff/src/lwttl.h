#pragma once

#include <linmath.h>

#ifdef __cplusplus
extern "C" {;
#endif
typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWUDP LWUDP;
typedef struct _LWTTL LWTTL;
typedef struct _LWPTTLWAYPOINTS LWPTTLWAYPOINTS;
typedef struct _LWPTTLROUTESTATE LWPTTLROUTESTATE;
typedef struct _LWPTTLSTATICSTATE2 LWPTTLSTATICSTATE2;
typedef struct _LWPTTLSEAPORTSTATE LWPTTLSEAPORTSTATE;
typedef struct _LWPTTLSTATICOBJECT2 LWPTTLSTATICOBJECT2;
typedef struct _LWPTTLSEAPORTOBJECT LWPTTLSEAPORTOBJECT;
typedef struct _LWPTTLCITYOBJECT LWPTTLCITYOBJECT;
typedef struct _LWPTTLSALVAGEOBJECT LWPTTLSALVAGEOBJECT;
typedef struct _LWTTLLNGLAT LWTTLLNGLAT;
typedef struct _LWHTMLUI LWHTMLUI;
typedef struct _LWPTTLSINGLECELL LWPTTLSINGLECELL;
typedef struct _LWPTTLSTATICSTATE3 LWPTTLSTATICSTATE3;

LWTTL* lwttl_new(float aspect_ratio);
void lwttl_destroy(LWTTL** _ttl);
void lwttl_worldmap_scroll_to(LWTTL* ttl, float lng, float lat, LWUDP* sea_udp);
void lwttl_worldmap_scroll_to_cell_center(LWTTL* ttl, int xc, int yc, LWUDP* sea_udp);
void lwttl_update_aspect_ratio(LWTTL* ttl, float aspect_ratio);
const LWTTLLNGLAT* lwttl_center(const LWTTL* ttl);
void lwttl_update(LWTTL* ttl, LWCONTEXT* pLwc, float delta_time);
const char* lwttl_http_header(const LWTTL* ttl);
int lwttl_track_object_id(const LWTTL* ttl);
void lwttl_set_track_object_id(LWTTL* ttl, int v);
int lwttl_track_object_ship_id(const LWTTL* ttl);
void lwttl_set_track_object_ship_id(LWTTL* ttl, int v);
void lwttl_request_waypoints(const LWTTL* ttl, int v);
void lwttl_set_center(LWTTL* ttl, float lng, float lat);
void lwttl_set_seaarea(LWTTL* ttl, const char* name);
const char* lwttl_seaarea(LWTTL* ttl);
void lwttl_set_view_scale(LWTTL* ttl, int v);
int lwttl_view_scale(const LWTTL* ttl);
int lwttl_view_scale_max(const LWTTL* ttl);
int lwttl_clamped_view_scale(const LWTTL* ttl);
void lwttl_set_sea_udp(LWTTL* ttl, LWUDP* sea_udp);
void lwttl_udp_send_ttlping(const LWTTL* ttl, LWUDP* udp, int ping_seq);
void lwttl_udp_send_request_waypoints(const LWTTL* ttl, LWUDP* sea_udp, int ship_id);
void lwttl_udp_update(LWTTL* ttl, LWCONTEXT* pLwc);
void lwttl_lock_rendering_mutex(LWTTL* ttl);
void lwttl_unlock_rendering_mutex(LWTTL* ttl);
const LWPTTLWAYPOINTS* lwttl_get_waypoints(const LWTTL* ttl);
const LWPTTLWAYPOINTS* lwttl_get_waypoints_by_ship_id(const LWTTL* ttl, int ship_id);
void lwttl_write_last_state(const LWTTL* ttl, const LWCONTEXT* pLwc);
void lwttl_read_last_state(LWTTL* ttl, const LWCONTEXT* pLwc);
const LWPTTLROUTESTATE* lwttl_full_state(const LWTTL* ttl);
int lwttl_query_chunk_range_land(const LWTTL* ttl,
                                 const float lng_min,
                                 const float lat_min,
                                 const float lng_max,
                                 const float lat_max,
                                 const int view_scale,
                                 int* chunk_index_array,
                                 const int chunk_index_array_len,
                                 int* xcc0,
                                 int* ycc0,
                                 int* xcc1,
                                 int* ycc1);
int lwttl_query_chunk_range_seaport(const LWTTL* ttl,
                                    const float lng_min,
                                    const float lat_min,
                                    const float lng_max,
                                    const float lat_max,
                                    const int view_scale,
                                    int* chunk_index_array,
                                    const int chunk_index_array_len,
                                    int* xcc0,
                                    int* ycc0,
                                    int* xcc1,
                                    int* ycc1);
int lwttl_query_chunk_range_city(const LWTTL* ttl,
                                 const float lng_min,
                                 const float lat_min,
                                 const float lng_max,
                                 const float lat_max,
                                 const int view_scale,
                                 int* chunk_index_array,
                                 const int chunk_index_array_len,
                                 int* xcc0,
                                 int* ycc0,
                                 int* xcc1,
                                 int* ycc1);
int lwttl_query_chunk_range_salvage(const LWTTL* ttl,
                                    const float lng_min,
                                    const float lat_min,
                                    const float lng_max,
                                    const float lat_max,
                                    const int view_scale,
                                    int* chunk_index_array,
                                    const int chunk_index_array_len,
                                    int* xcc0,
                                    int* ycc0,
                                    int* xcc1,
                                    int* ycc1);
const LWPTTLSTATICOBJECT2* lwttl_query_chunk_land(const LWTTL* ttl,
                                                  const int chunk_index,
                                                  int* xc0,
                                                  int* yc0,
                                                  int* count);
const LWPTTLSEAPORTOBJECT* lwttl_query_chunk_seaport(const LWTTL* ttl,
                                                     const int chunk_index,
                                                     int* xc0,
                                                     int* yc0,
                                                     int* count);
const LWPTTLCITYOBJECT* lwttl_query_chunk_city(const LWTTL* ttl,
                                               const int chunk_index,
                                               int* xc0,
                                               int* yc0,
                                               int* count);
const LWPTTLSALVAGEOBJECT* lwttl_query_chunk_salvage(const LWTTL* ttl,
                                                     const int chunk_index,
                                                     int* xc0,
                                                     int* yc0,
                                                     int* count);
float lwttl_half_lng_extent_in_degrees(const int view_scale);
float lwttl_half_lat_extent_in_degrees(const int view_scale);
int lwttl_lng_to_floor_int(float lng);
int lwttl_lat_to_floor_int(float lat);
int lwttl_lng_to_ceil_int(float lng);
int lwttl_lat_to_ceil_int(float lat);
LWUDP* lwttl_sea_udp(LWTTL* ttl);
void lwttl_set_earth_globe_scale(LWTTL* ttl, float earth_globe_scale);
void lwttl_scroll_view_scale(LWTTL* ttl, float offset);
float lwttl_earth_globe_scale(LWTTL* ttl);
float lwttl_earth_globe_morph_weight(float earth_globe_scale);
float lwttl_earth_globe_y(const LWTTLLNGLAT* center, float earth_globe_scale, float earth_globe_morph_weight);
void lwttl_fill_world_seaports_bookmarks(LWHTMLUI* htmlui);
void lwttl_send_ping_now(LWTTL* ttl);
void lwttl_prerender_mutable_context(LWTTL* ttl, LWCONTEXT* pLwc, LWHTMLUI* htmlui);
int lwttl_selected(const LWTTL* ttl, LWTTLLNGLAT* pos);
int lwttl_selected_int(const LWTTL* ttl, int* xc0, int* yc0);
void lwttl_on_press(LWTTL* ttl, const LWCONTEXT* pLwc, float nx, float ny);
void lwttl_on_move(LWTTL* ttl, const LWCONTEXT* pLwc, float nx, float ny);
void lwttl_on_release(LWTTL* ttl, const LWCONTEXT* pLwc, float nx, float ny);
void lwttl_view_proj(const LWTTL* ttl, mat4x4 view, mat4x4 proj);
void lwttl_update_view_proj(LWTTL* ttl, float aspect_ratio);
void lwttl_screen_to_world_pos(const float touchx,
                               const float touchy,
                               const float screenw,
                               const float screenh,
                               const mat4x4 proj,
                               const mat4x4 view_model,
                               vec2 world_pos);
void lwttl_clear_selected_pressed_pos(LWTTL* ttl);
const LWPTTLSINGLECELL* lwttl_single_cell(const LWTTL* ttl);
int lwttl_press_menu_info(const LWTTL* ttl,
                          float* press_menu_gauge_total,
                          float* press_menu_gauge_appear_delay,
                          float* press_at);
void lwttl_change_selected_cell_to(LWTTL* ttl,
                                   int xc,
                                   int yc,
                                   const LWTTLLNGLAT* lnglat);
int lwttl_press_ring_info(const LWTTL* ttl,
                          const float app_time,
                          float* press_menu_gauge_current,
                          float* press_menu_gauge_total);
int lwttl_dragging_info(const LWTTL* ttl,
                        int* xc0,
                        int* yc0,
                        int* xc1,
                        int* yc1);
const char* lwttl_get_or_create_user_id(LWTTL* ttl,
                                        LWCONTEXT* pLwc);
int lwttl_ping_send_interval_multiplier(const LWTTL* ttl);
void lwttl_get_cell_bound(const float lng_min,
                          const float lat_min,
                          const float lng_max,
                          const float lat_max,
                          int* xc0,
                          int* yc0,
                          int* xc1,
                          int* yc1);
const void* lwttl_world_text_begin(const LWTTL* ttl);
const char* lwttl_world_text(const LWTTL* ttl, const void* it, int* xc, int* yc, float* age, float* lifetime);
const void* lwttl_world_text_next(const LWTTL* ttl, const void* it);
#ifdef __cplusplus
}
#endif
