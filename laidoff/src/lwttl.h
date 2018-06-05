#pragma once

#include <linmath.h>

#define LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG (1)
#define LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT (1)
#define LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG_WITH_MARGIN (LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG + 1)
#define LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT_WITH_MARGIN (LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT + 1)

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
typedef struct _LWHTMLUI LWHTMLUI;
typedef struct _LWPTTLSINGLECELL LWPTTLSINGLECELL;
typedef struct _LWPTTLSTATICSTATE3 LWPTTLSTATICSTATE3;
typedef struct _LWPTTLROUTEOBJECT LWPTTLROUTEOBJECT;
typedef struct _LWTTLFIELDVIEWPORT LWTTLFIELDVIEWPORT;

typedef struct _LWTTLLNGLAT {
    float lng;
    float lat;
} LWTTLLNGLAT;

typedef enum _LW_TTL_FIELD_VIEWPORT_RENDER_FLAG {
    LTFVRF_MORPHED_EARTH = 1 << 0,
    LTFVRF_LAND_CELL = 1 << 1,
    LTFVRF_WAYPOINT_LINE_SEGMENT = 1 << 2,
    LTFVRF_SHIP = 1 << 3,
    LTFVRF_CELL_PIXEL_SELECTOR = 1 << 4,
    LTFVRF_DRAGGING_WAYPOINT_LINE_SEGMENT = 1 << 5,
    LTFVRF_SEAPORT = 1 << 6,
    LTFVRF_CITY = 1 << 7,
    LTFVRF_SALVAGE = 1 << 8,
    LTFVRF_COORDINATES = 1 << 9,
    LTFVRF_SEA_OBJECT_NAMEPLATE = 1 << 10,
    LTFVRF_SINGLE_CELL_INFO = 1 << 11,
    LTFVRF_WORLD_TEXT = 1 << 12,
    LTFVRF_REGION_NAME = 1 << 13,

    LTFVRF_ALL = -1,
} LW_TTL_FIELD_VIEWPORT_RENDER_FLAG;

LWTTL* lwttl_new(float aspect_ratio);
void lwttl_destroy(LWTTL** _ttl);
void lwttl_worldmap_scroll_to(LWTTL* ttl, float lng, float lat, LWUDP* sea_udp);
void lwttl_worldmap_scroll_to_cell_center(LWTTL* ttl, int xc, int yc, LWUDP* sea_udp);
void lwttl_update_aspect_ratio(LWTTL* ttl, int width, int height);
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
void lwttl_udp_send_request_waypoints(const LWTTL* ttl, LWUDP* sea_udp, int ship_id);
void lwttl_udp_update(LWTTL* ttl, LWCONTEXT* pLwc);
void lwttl_lock_rendering_mutex(LWTTL* ttl);
void lwttl_unlock_rendering_mutex(LWTTL* ttl);
const LWPTTLWAYPOINTS* lwttl_get_waypoints(const LWTTL* ttl);
const LWPTTLWAYPOINTS* lwttl_get_waypoints_by_ship_id(const LWTTL* ttl, int ship_id);
void lwttl_write_last_state(const LWTTL* ttl, const LWCONTEXT* pLwc);
void lwttl_read_last_state(LWTTL* ttl, const LWCONTEXT* pLwc);
const LWPTTLROUTESTATE* lwttl_full_state(const LWTTL* ttl);
int lwttl_query_chunk_range_land_vp(const LWTTL* ttl,
                                    const LWTTLFIELDVIEWPORT* vp,
                                    int* chunk_index_array,
                                    const int chunk_index_array_len,
                                    int* xcc0,
                                    int* ycc0,
                                    int* xcc1,
                                    int* ycc1);
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
int lwttl_query_chunk_range_seaport_vp(const LWTTL* ttl,
                                       const LWTTLFIELDVIEWPORT* vp,
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
int lwttl_query_chunk_range_city_vp(const LWTTL* ttl,
                                    const LWTTLFIELDVIEWPORT* vp,
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
int lwttl_query_chunk_range_salvage_vp(const LWTTL* ttl,
                                       const LWTTLFIELDVIEWPORT* vp,
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
LWUDP* lwttl_sea_udp(LWTTL* ttl);
void lwttl_scroll_view_scale(LWTTL* ttl, float offset);
float lwttl_earth_globe_y(const LWTTLLNGLAT* center, float earth_globe_scale);
void lwttl_fill_world_seaports_bookmarks(LWHTMLUI* htmlui);
void lwttl_send_ping_now(LWTTL* ttl);
void lwttl_prerender_mutable_context(LWTTL* ttl, LWCONTEXT* pLwc, LWHTMLUI* htmlui);
int lwttl_selected(const LWTTL* ttl, LWTTLLNGLAT* pos);
int lwttl_selected_int(const LWTTL* ttl, int* xc0, int* yc0);
void lwttl_on_press(LWTTL* ttl, const LWCONTEXT* pLwc, float nx, float ny);
void lwttl_on_move(LWTTL* ttl, const LWCONTEXT* pLwc, float nx, float ny);
void lwttl_on_release(LWTTL* ttl, LWCONTEXT* pLwc, float nx, float ny);
void lwttl_view_proj(const LWTTL* ttl, mat4x4 view, mat4x4 proj);
void lwttl_update_view_proj(const LWTTLFIELDVIEWPORT* vp,
                            const LWTTLFIELDVIEWPORT* vp0,
                            const int viewport_width,
                            const int viewport_height,
                            mat4x4 view,
                            mat4x4 proj);
void lwttl_screen_to_world_pos(const LWTTL* ttl,
                               const float touchnx,
                               const float touchny,
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
const char* lwttl_world_text_vp(const LWTTL* ttl,
                                const void* it,
                                const LWTTLFIELDVIEWPORT* vp,
                                const mat4x4 proj_view,
                                float* ui_point_x,
                                float* ui_point_y,
                                float* scale);
const char* lwttl_world_text(const LWTTL* ttl,
                             const void* it,
                             const LWTTLLNGLAT* center,
                             const float aspect_ratio,
                             const mat4x4 proj_view,
                             const int view_scale,
                             float* ui_point_x,
                             float* ui_point_y,
                             float* scale);
const void* lwttl_world_text_next(const LWTTL* ttl, const void* it);
void lwttl_toggle_cell_grid(LWTTL* ttl);
int lwttl_cell_grid(const LWTTL* ttl);
int lwttl_gold(const LWTTL* ttl);
int lwttl_ports(const LWTTL* ttl);
int lwttl_ships(const LWTTL* ttl);
int lwttl_is_selected_cell(const LWTTL* ttl, int x0, int y0);
int lwttl_is_selected_cell_intersect(const LWTTL* ttl, int x0, int y0);
int lwttl_is_selected_cell_diff(const LWTTL* ttl, int x0, int y0, int* dx0, int* dy0);
float lwttl_selected_cell_popup_height(const LWTTL* ttl, const LWTTLFIELDVIEWPORT* vp);
const char* lwttl_route_state(const LWPTTLROUTEOBJECT* obj);
void lwttl_cam_eye(const LWTTL* ttl, vec3 cam_eye);
void lwttl_set_cam_eye(LWTTL* ttl, const vec3 cam_eye);
void lwttl_cam_look_at(const LWTTL* ttl, vec3 cam_look_at);
void lwttl_set_cam_look_at(LWTTL* ttl, const vec3 cam_look_at);
float cell_fx_to_lng(float fx);
float cell_fy_to_lat(float fy);
float cell_x_to_lng(int x);
float cell_y_to_lat(int y);
float render_coords_to_lng(float rc, const LWTTLLNGLAT* center, int view_scale);
float render_coords_to_lat(float rc, const LWTTLLNGLAT* center, int view_scale);
float cell_fx_to_render_coords(float fx, const LWTTLLNGLAT* center, int view_scale);
float cell_fy_to_render_coords(float fy, const LWTTLLNGLAT* center, int view_scale);
float cell_fx_to_render_coords_vp(float fx, const LWTTLFIELDVIEWPORT* vp);
float cell_fy_to_render_coords_vp(float fy, const LWTTLFIELDVIEWPORT* vp);
void lwttl_udp_send_ttlchat(const LWTTL* ttl, LWUDP* udp, const char* line);
void lwttl_udp_send_ttlping(const LWTTL* ttl, LWUDP* udp, int ping_seq);
int lwttl_add_field_viewport(LWTTL* ttl, const LWTTLFIELDVIEWPORT* vp);
void lwttl_remove_field_viewport(LWTTL* ttl, int viewport_index);
const LWTTLFIELDVIEWPORT* lwttl_viewport(const LWTTL* ttl, int viewport_index);
int lwttl_sizeof_viewport();
LWTTLFIELDVIEWPORT* lwttl_copy_viewport_data(const LWTTL* ttl, int viewport_index, LWTTLFIELDVIEWPORT* vp_copy);
int lwttl_viewport_max_count(const LWTTL* ttl);
int lwttl_calculate_clamped_view_scale(int view_scale, int view_scale_ping_max);
void lwttl_set_viewport_show(LWTTL* ttl, int viewport_index, int show);
float lng_to_render_coords(float lng, const LWTTLFIELDVIEWPORT* vp);
float lat_to_render_coords(float lat, const LWTTLFIELDVIEWPORT* vp);
float cell_x_to_render_coords(int x, const LWTTLFIELDVIEWPORT* vp);
float cell_y_to_render_coords(int y, const LWTTLFIELDVIEWPORT* vp);
float lwttl_vehicle_render_scale(const LWTTL* ttl, const LWTTLFIELDVIEWPORT* vp, float scale);
const vec4* lwttl_viewport_view(const LWTTLFIELDVIEWPORT* vp);
const vec4* lwttl_viewport_proj(const LWTTLFIELDVIEWPORT* vp);
const vec4* lwttl_viewport_ui_proj(const LWTTLFIELDVIEWPORT* vp);
int lwttl_viewport_clamped_view_scale(const LWTTLFIELDVIEWPORT* vp);
int lwttl_viewport_cell_render_info(const LWTTLFIELDVIEWPORT* vp,
                                    const int bound_xc0,
                                    const int bound_yc0,
                                    const int bx,
                                    const int by,
                                    float* x0,
                                    float* y0,
                                    float* cell_x0,
                                    float* cell_y0,
                                    float* cell_z0,
                                    float* cell_w,
                                    float* cell_h);
float lwttl_viewport_aspect_ratio(const LWTTLFIELDVIEWPORT* vp);
int lwttl_viewport_view_scale(const LWTTLFIELDVIEWPORT* vp);
float lwttl_viewport_waypoint_line_segment_thickness(const LWTTLFIELDVIEWPORT* vp);
float lwttl_viewport_icon_width(const LWTTLFIELDVIEWPORT* vp);
float lwttl_viewport_icon_height(const LWTTLFIELDVIEWPORT* vp);
int lwttl_viewport_icon_render_info(const LWTTL* ttl,
                                    const LWTTLFIELDVIEWPORT* vp,
                                    const int xc0,
                                    const int x_scaled_offset_0,
                                    const int yc0,
                                    const int y_scaled_offset_0,
                                    float* cell_x0,
                                    float* cell_y0,
                                    float* cell_z0);
float lwttl_viewport_cell_render_width(const LWTTLFIELDVIEWPORT* vp);
float lwttl_viewport_cell_render_height(const LWTTLFIELDVIEWPORT* vp);
float lwttl_viewport_rt_x(const LWTTLFIELDVIEWPORT* vp);
float lwttl_viewport_rt_y(const LWTTLFIELDVIEWPORT* vp);
void lwttl_viewport_range(const LWTTLFIELDVIEWPORT* vp,
                          int* viewport_x,
                          int* viewport_y,
                          int* viewport_width,
                          int* viewport_height);
void lwttl_set_viewport_view_scale(LWTTL* ttl, int viewport_index, int view_scale);
void lwttl_set_window_size(LWTTL* ttl, int w, int h, float aspect_ratio);
int lwttl_viewport_render_flags(const LWTTLFIELDVIEWPORT* vp);
const LWTTLLNGLAT* lwttl_viewport_view_center(const LWTTLFIELDVIEWPORT* vp);
#ifdef __cplusplus
}
#endif
