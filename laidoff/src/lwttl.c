#include "lwttl.h"
#include <stdio.h>
#include <stdlib.h>
#include "file.h"
#include "lwmacro.h"
#include "lwgl.h"
#include "lwcontext.h"
#include "laidoff.h"
//#include "render_solid.h"
#include "lwudp.h"
#include "lwlnglat.h"
#include "lwlog.h"
#include "lwmutex.h"
#include "lz4.h"
#include "lwttl.h"
#include "htmlui.h"
#include "lwlnglat.h"
#include "script.h"
#include "input.h"
#include "logic.h"
#include "pcg_basic.h"
#include "lwmath.h"
#include <assert.h>
#include "iconchar.h"

#define LOCAL_SAVE_FILE_MAGIC (0x19850506)
#define LOCAL_SAVE_FILE_LATEST_VERSION (2)

#define TTL_OBJECT_CACHE_CHUNK_COUNT (256*1024)
#define TTL_OBJECT_CACHE_LAND_COUNT (TTL_OBJECT_CACHE_CHUNK_COUNT*32)
#define TTL_OBJECT_CACHE_SEAPORT_COUNT (TTL_OBJECT_CACHE_CHUNK_COUNT*32)
#define TTL_OBJECT_CACHE_CITY_COUNT (TTL_OBJECT_CACHE_CHUNK_COUNT*16)
#define TTL_OBJECT_CACHE_SALVAGE_COUNT (TTL_OBJECT_CACHE_CHUNK_COUNT*8)

typedef struct _LWTTLCHUNKVALUE {
    LWTTLCHUNKKEY key;
    long long ts;
    int start;
    int count;
} LWTTLCHUNKVALUE;

typedef struct _LWTTLCELLBOUND {
    int xc0;
    int yc0;
    int xc1;
    int yc1;
} LWTTLCELLBOUND;

typedef struct _LWTTLCHUNKBOUND {
    int xcc0;
    int ycc0;
    int xcc1;
    int ycc1;
} LWTTLCHUNKBOUND;

typedef struct _LWTTLOBJECTCACHE {
    int count;
    LWTTLCHUNKKEY key_array[TTL_OBJECT_CACHE_CHUNK_COUNT];
    LWTTLCHUNKVALUE value_array[TTL_OBJECT_CACHE_CHUNK_COUNT];
} LWTTLOBJECTCACHE;

typedef struct _LWTTLOBJECTCACHEGROUP {
    LWTTLOBJECTCACHE land_cache;
    LWPTTLSTATICOBJECT2 land_array[TTL_OBJECT_CACHE_LAND_COUNT];
    int land_count;

    //LWTTLOBJECTCACHE land_bitmap_cache;
    //LWPTTLSTATICSTATE3 land_bitmap_array[TTL_OBJECT_CACHE_LAND_BITMAP_COUNT];
    //int land_count;

    LWTTLOBJECTCACHE seaport_cache;
    LWPTTLSEAPORTOBJECT seaport_array[TTL_OBJECT_CACHE_SEAPORT_COUNT];
    int seaport_count;

    LWTTLOBJECTCACHE city_cache;
    LWPTTLCITYOBJECT city_array[TTL_OBJECT_CACHE_CITY_COUNT];
    int city_count;

    LWTTLOBJECTCACHE salvage_cache;
    LWPTTLSALVAGEOBJECT salvage_array[TTL_OBJECT_CACHE_SALVAGE_COUNT];
    int salvage_count;
} LWTTLOBJECTCACHEGROUP;

typedef struct _LWTTLSAVEDATA {
    int magic;
    int version;
    float lng;
    float lat;
    int view_scale;
    vec3 cam_eye;
    vec3 cam_look_at;
} LWTTLSAVEDATA;

typedef struct _LWTTLSELECTED {
    int selected;
    LWTTLLNGLAT pos;
    int pos_xc;
    int pos_yc;
    int press_pos_xc;
    int press_pos_yc;
    int dragging_pos_xc;
    int dragging_pos_yc;
    float press_at;
    int pressing;
    float press_menu_gauge_appear_delay;
    float press_menu_gauge_total;
    int dragging;
    float selected_cell_height;
    float selected_cell_max_height;
    float selected_cell_height_speed;
} LWTTLSELECTED;

typedef struct _LWTTLFIELDVIEWPORT {
    int valid;
    int show;
    int field_viewport_x;
    int field_viewport_y;
    int field_viewport_width;
    int field_viewport_height;
    float field_viewport_aspect_ratio;
    float field_viewport_rt_x;
    float field_viewport_rt_y;
    mat4x4 view;
    mat4x4 proj;
    mat4x4 ui_proj;
    LWTTLLNGLAT view_center;
    int view_scale;
    int view_scale_msb;
    int clamped_view_scale;
    int clamped_to_original_view_scale_ratio;
    float half_lng_extent_in_deg;
    float half_lat_extent_in_deg;
    float lng_min;
    float lng_max;
    float lat_min;
    float lat_max;
    // scaled ('view_scale'-applied) rendering size of cell
    float cell_render_width;
    float cell_render_height;
    int cell_bound_xc0;
    int cell_bound_yc0;
    int cell_bound_xc1;
    int cell_bound_yc1;
    int clamped_view_scale_msb;
    LWTTLSELECTED selected;
    vec3 cam_eye;
    vec3 cam_look_at;
    int cell_grid;
    int render_flags;
} LWTTLFIELDVIEWPORT;

typedef enum _LW_TTL_WORLD_TEXT_ANIM_TYPE {
    LTWTAT_STOP,
    LTWTAT_UP,
    LTWTAT_DOWN,
    LTWTAT_MOVE,
    LTWTAT_SCALE_0_TO_1_TO_0,
    LTWTAT_SCALE_1_TO_0,
    LTWTAT_SCALE_0_TO_1,
} LW_TTL_WORLD_TEXT_ANIM_TYPE;

typedef struct _LWTTLWORLDTEXT {
    int valid;
    int xc;
    int yc;
    int xc1;
    int yc1;
    float age;
    float lifetime;
    float move_dist;
    char text[128];
    LW_TTL_WORLD_TEXT_ANIM_TYPE anim_type;
} LWTTLWORLDTEXT;

typedef struct _LWTTLCELLBOX {
    int xc0, yc0, xc1, yc1;
} LWTTLCELLBOX;

typedef struct _LWTTL {
    int version;
    int track_object_id;
    int track_object_ship_id;
    char seaarea[128]; // should match with LWPTTLSEAAREA.name size
    int view_scale_max;
    int view_scale_ping_max;
    LWMUTEX rendering_mutex;
    LWUDP* sea_udp;
    LWPTTLWAYPOINTS waypoints;
    // packet cache
    LWPTTLROUTESTATE ttl_dynamic_state;
    LWPTTLSINGLECELL ttl_single_cell;
    LWTTLOBJECTCACHEGROUP object_cache;
    char user_id_str[512];
    int panning;
    LWPTTLWAYPOINTS waypoints_cache[512];
    int waypoints_cache_count;
    LWTTLWORLDTEXT world_text[64];
    int gold;
    int ports;
    int ships;
    LWTTLFIELDVIEWPORT viewports[4];
    int cell_box_count;
    LWTTLCELLBOX cell_box[512];
} LWTTL;

static const LWTTLSELECTED* lwttl_main_viewport_selected(const LWTTL* ttl) {
    const LWTTLFIELDVIEWPORT* vp = lwttl_viewport(ttl, 0);
    if (vp) {
        return &vp->selected;
    }
    LOGEP("Main viewport is null.");
    return 0;
}

static float lwttl_lng_to_int_float(float lng) {
    return LNGLAT_RES_WIDTH / 2 + (lng / 180.0f) * LNGLAT_RES_WIDTH / 2;
}

static int lwttl_lng_to_round_int(float lng) {
    return (int)(roundf(lwttl_lng_to_int_float(lng)));
}

static int lwttl_lng_to_floor_int(float lng) {
    return (int)(floorf(lwttl_lng_to_int_float(lng)));
}

static int lwttl_lng_to_ceil_int(float lng) {
    return (int)(ceilf(lwttl_lng_to_int_float(lng)));
}

static float lwttl_lat_to_int_float(float lat) {
    return LNGLAT_RES_HEIGHT / 2 - (lat / 90.0f) * LNGLAT_RES_HEIGHT / 2;
}

static int lwttl_lat_to_round_int(float lat) {
    return (int)(roundf(lwttl_lat_to_int_float(lat)));
}

static int lwttl_lat_to_floor_int(float lat) {
    return (int)(floorf(lwttl_lat_to_int_float(lat)));
}

static int lwttl_lat_to_ceil_int(float lat) {
    return (int)(ceilf(lwttl_lat_to_int_float(lat)));
}

static void lwttl_update_viewport_data(const LWTTL* ttl,
                                       LWTTLFIELDVIEWPORT* vp,
                                       int x,
                                       int y,
                                       int w,
                                       int h,
                                       int view_scale,
                                       const LWTTLLNGLAT* view_center,
                                       float half_lng_extent_in_deg,
                                       float half_lat_extent_in_deg) {
    vp->field_viewport_x = x;
    vp->field_viewport_y = y;
    vp->field_viewport_width = w;
    vp->field_viewport_height = h;
    lwttl_update_view_proj(vp,
                           &ttl->viewports[0], // main viewport ref
                           w,
                           h,
                           vp->view,
                           vp->proj);
    logic_update_default_ui_proj(w,
                                 h,
                                 vp->ui_proj);
    vp->view_scale = LWCLAMP(view_scale, 1, ttl->view_scale_max);
    vp->clamped_view_scale = lwttl_calculate_clamped_view_scale(view_scale, LNGLAT_VIEW_SCALE_PING_MAX);
    memcpy(&vp->view_center, view_center, sizeof(LWTTLLNGLAT));
    vp->field_viewport_aspect_ratio = (float)vp->field_viewport_width / vp->field_viewport_height;
    lwcontext_rt_corner(vp->field_viewport_aspect_ratio,
                        &vp->field_viewport_rt_x,
                        &vp->field_viewport_rt_y);
    vp->view_scale_msb = msb_index(vp->view_scale);
    vp->clamped_view_scale_msb = msb_index(vp->clamped_view_scale);
    vp->clamped_to_original_view_scale_ratio = vp->view_scale / vp->clamped_view_scale;
    vp->half_lng_extent_in_deg = LWCLAMP(half_lng_extent_in_deg, 0, lwttl_half_lng_extent_in_degrees(vp->clamped_view_scale));
    vp->half_lat_extent_in_deg = LWCLAMP(half_lat_extent_in_deg, 0, lwttl_half_lat_extent_in_degrees(vp->clamped_view_scale));
    vp->lng_min = vp->view_center.lng - vp->half_lng_extent_in_deg;
    vp->lng_max = vp->view_center.lng + vp->half_lng_extent_in_deg;
    vp->lat_min = vp->view_center.lat - vp->half_lat_extent_in_deg;
    vp->lat_max = vp->view_center.lat + vp->half_lat_extent_in_deg;
    vp->cell_render_width = cell_x_to_render_coords(1, vp) - cell_x_to_render_coords(0, vp);
    vp->cell_render_height = cell_y_to_render_coords(0, vp) - cell_y_to_render_coords(1, vp);
    lwttl_get_cell_bound(vp->lng_min,
                         vp->lat_min,
                         vp->lng_max,
                         vp->lat_max,
                         &vp->cell_bound_xc0,
                         &vp->cell_bound_yc0,
                         &vp->cell_bound_xc1,
                         &vp->cell_bound_yc1);
    vp->selected.press_menu_gauge_total = 0.45f;
    vp->selected.press_menu_gauge_appear_delay = 0.2f;
    vp->selected.selected_cell_max_height = 0.1f;
    vp->selected.selected_cell_height_speed = 4.0f;
}

LWTTL* lwttl_new(float aspect_ratio) {
    LWTTL* ttl = (LWTTL*)calloc(1, sizeof(LWTTL));
    ttl->version = 1;
    ttl->view_scale_max = 1 << 13; // should be no more than 2^15 (== 2 ^ MAX(LWTTLCHUNKKEY.view_scale_msb))
    ttl->view_scale_ping_max = LNGLAT_VIEW_SCALE_PING_MAX;
    LWMUTEX_INIT(ttl->rendering_mutex);
    LWTTLFIELDVIEWPORT vp;
    memset(&vp, 0, sizeof(LWTTLFIELDVIEWPORT));
    // add main viewport
    lwttl_update_viewport_data(ttl,
                               &vp,
                               0,
                               0,
                               1,
                               1,
                               1,
                               &ttl->viewports[0].view_center,
                               lwttl_half_lng_extent_in_degrees(1),
                               lwttl_half_lat_extent_in_degrees(1));
    vp.cell_grid = 1;
    vp.cam_eye[0] = +10;
    vp.cam_eye[1] = -10;
    vp.cam_eye[2] = +10;
    vp.cam_look_at[0] = 0;
    vp.cam_look_at[1] = 0;
    vp.cam_look_at[2] = 0;
    vp.cell_grid = 1;
    vp.render_flags = LTFVRF_ALL;
    vp.render_flags &= ~LTFVRF_COORDINATES;
#if !LW_PLATFORM_WIN32
    vp.render_flags &= ~LTFVRF_CELL_BOX_BOUNDARY;
#endif
    lwttl_add_field_viewport(ttl, &vp);
    lwttl_update_view_proj(&ttl->viewports[0],
                           &ttl->viewports[0], // main viewport ref
                           ttl->viewports[0].field_viewport_width,
                           ttl->viewports[0].field_viewport_height,
                           ttl->viewports[0].view,
                           ttl->viewports[0].proj);
    lwttl_clear_selected_pressed_pos(ttl);

    // add aux1 viewport
    lwttl_update_viewport_data(ttl,
                               &vp,
                               0,
                               0,
                               200,
                               200,
                               1,
                               &ttl->viewports[0].view_center,
                               LNGLAT_SEA_PING_EXTENT_IN_DEGREES / LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * 5,
                               LNGLAT_SEA_PING_EXTENT_IN_DEGREES / LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * 5);
    vp.cell_grid = 1;
    vp.cam_eye[0] = +10;
    vp.cam_eye[1] = -10;
    vp.cam_eye[2] = +10;
    vp.cam_look_at[0] = 0;
    vp.cam_look_at[1] = 0;
    vp.cam_look_at[2] = 0;
    vp.cell_grid = 1;
    vp.render_flags = LTFVRF_LAND_CELL | LTFVRF_CELL_PIXEL_SELECTOR | LTFVRF_SEAPORT | LTFVRF_CITY | LTFVRF_SALVAGE;
    // fix cell selector height to maximum on aux1 viewport
    vp.selected.selected_cell_height = vp.selected.selected_cell_max_height;
    lwttl_add_field_viewport(ttl, &vp);
    lwttl_update_view_proj(&ttl->viewports[1],
                           &ttl->viewports[0], // main viewport ref
                           ttl->viewports[1].field_viewport_width,
                           ttl->viewports[1].field_viewport_height,
                           ttl->viewports[1].view,
                           ttl->viewports[1].proj);
    // Ulsan
    lwttl_worldmap_scroll_to(ttl, 129.496f, 35.494f, 0);
    return ttl;
}

void lwttl_destroy(LWTTL** __ttl) {
    LWTTL* ttl = *(LWTTL**)__ttl;
    LWMUTEX_DESTROY(ttl->rendering_mutex);
    free(*__ttl);
    *__ttl = 0;
}

void lwttl_worldmap_scroll_to(LWTTL* ttl, float lng, float lat, LWUDP* sea_udp) {
    // cancel tracking if user want to scroll around
    lwttl_set_track_object_ship_id(ttl, 0);
    // wrapping drift on float variable can be significant on some cases...
    // only wrap if out of range
    if (lng < -180.0f || lng > +180.0f) {
        ttl->viewports[0].view_center.lng = numcomp_wrap_min_max(lng, -180.0f, +180.0f);
    } else {
        ttl->viewports[0].view_center.lng = lng;
    }
    ttl->viewports[0].view_center.lat = LWCLAMP(lat, -90.0f, +90.0f);
    lwttl_update_viewport_data(ttl,
                               &ttl->viewports[0],
                               ttl->viewports[0].field_viewport_x,
                               ttl->viewports[0].field_viewport_y,
                               ttl->viewports[0].field_viewport_width,
                               ttl->viewports[0].field_viewport_height,
                               ttl->viewports[0].view_scale,
                               &ttl->viewports[0].view_center,
                               ttl->viewports[0].half_lng_extent_in_deg,
                               ttl->viewports[0].half_lat_extent_in_deg);
    if (sea_udp) {
        lwttl_udp_send_ttlping(ttl, sea_udp, 0);
    }
}

void lwttl_worldmap_scroll_to_cell_center(LWTTL* ttl, int xc, int yc, LWUDP* sea_udp) {
    lwttl_worldmap_scroll_to(ttl,
                             cell_fx_to_lng(xc + 0.5f),
                             cell_fy_to_lat(yc + 0.5f),
                             sea_udp);
}

void lwttl_update_aspect_ratio(LWTTL* ttl, int width, int height) {
    lwttl_update_view_proj(&ttl->viewports[0],
                           &ttl->viewports[0], // main viewport ref
                           ttl->viewports[0].field_viewport_width,
                           ttl->viewports[0].field_viewport_height,
                           ttl->viewports[0].view,
                           ttl->viewports[0].proj);
}

const LWTTLLNGLAT* lwttl_center(const LWTTL* ttl) {
    return &ttl->viewports[0].view_center;
}

void lwttl_set_center(LWTTL* ttl, float lng, float lat) {
    lwttl_worldmap_scroll_to(ttl, lng, lat, 0);
}

void lwttl_set_seaarea(LWTTL* ttl, const char* name) {
    strncpy(ttl->seaarea, name, ARRAY_SIZE(ttl->seaarea) - 1);
    ttl->seaarea[ARRAY_SIZE(ttl->seaarea) - 1] = 0;
}

const char* lwttl_seaarea(LWTTL* ttl) {
    return ttl->seaarea;
}

const char* lwttl_http_header(const LWTTL* ttl) {
    static char http_header[2048];
    const LWTTLLNGLAT* lnglat = lwttl_center(ttl);
    LWTTLLNGLAT selected_pos;
    const int selected = lwttl_selected(ttl, &selected_pos);
    if (ttl->user_id_str[0] == 0) {
        LOGEP("ttl->user_id_str length is 0");
    }
    const LWTTLSELECTED* main_viewport_selected = lwttl_main_viewport_selected(ttl);
    if (main_viewport_selected == 0) {
        return 0;
    }
    snprintf(http_header,
             ARRAY_SIZE(http_header),
             "X-U: %s\r\n"        // ttl-user-id.dat
             "X-Lng: %d\r\n"      // view center longitude cell index
             "X-Lat: %d\r\n"      // view center latitude cell index
             "X-S-Lng: %d\r\n"    // selected longitude cell index (-1 if no selection)
             "X-S-Lat: %d\r\n"    // selected latitude cell index (-1 if no selection)
             "X-D-XC0: %d\r\n"    // drag begin x
             "X-D-YC0: %d\r\n"    // drag begin y
             "X-D-XC1: %d\r\n"    // drag end x
             "X-D-YC1: %d\r\n",   // drag end y
             ttl->user_id_str,
             lwttl_lng_to_floor_int(lnglat->lng),
             lwttl_lat_to_floor_int(lnglat->lat),
             selected ? lwttl_lng_to_floor_int(selected_pos.lng) : -1,
             selected ? lwttl_lat_to_floor_int(selected_pos.lat) : -1,
             main_viewport_selected->pos_xc,
             main_viewport_selected->pos_yc,
             main_viewport_selected->dragging_pos_xc,
             main_viewport_selected->dragging_pos_yc);
    script_http_header(script_context()->L,
                       http_header + strlen(http_header),
                       ARRAY_SIZE(http_header) - strlen(http_header));
    return http_header;
}

int lwttl_track_object_id(const LWTTL* ttl) {
    return ttl->track_object_id;
}

void lwttl_set_track_object_id(LWTTL* ttl, int v) {
    ttl->track_object_id = v;
}

int lwttl_track_object_ship_id(const LWTTL* ttl) {
    return ttl->track_object_ship_id;
}

void lwttl_set_track_object_ship_id(LWTTL* ttl, int v) {
    ttl->track_object_ship_id = v;
}

void lwttl_request_waypoints(const LWTTL* ttl, int v) {
    lwttl_udp_send_request_waypoints(ttl, ttl->sea_udp, v);
}

void lwttl_set_view_scale(LWTTL* ttl, int v) {
    if (ttl->viewports[0].view_scale != v) {
        LOGIx("ttl->viewports[0].view_scale %d -> %d", ttl->viewports[0].view_scale, v);
        lwttl_set_viewport_view_scale(ttl, 0, v);
    }
}

int lwttl_view_scale(const LWTTL* ttl) {
    return ttl->viewports[0].view_scale;
}

int lwttl_view_scale_max(const LWTTL* ttl) {
    return ttl->view_scale_max;
}

int lwttl_calculate_clamped_view_scale(int view_scale, int view_scale_ping_max) {
    return LWCLAMP(view_scale, 1, view_scale_ping_max);
}

int lwttl_clamped_view_scale(const LWTTL* ttl) {
    // max value of view_scale bounded by LWTTLCHUNKKEY
    return lwttl_calculate_clamped_view_scale(lwttl_view_scale(ttl), ttl->view_scale_ping_max);
}

static int lower_bound_int(const int* a, int len, int v) {
    if (len <= 0) {
        return -1;
    }
    // Lower then the first element
    int beg_value = a[0];
    if (v < beg_value) {
        return -1;
    }
    // only one element
    if (len == 1) {
        return 0;
    }
    // Higher than the last element
    int end_value = a[len - 1];
    if (v >= end_value) {
        return len - 1;
    }
    int beg = 0;
    int end = len - 1;
    while (end - beg > 1) {
        int mid = (beg + end) / 2;
        int mid_value = a[mid];
        if (mid_value < v) {
            beg = mid;
        } else if (v < mid_value) {
            end = mid;
        } else {
            return mid;
        }
    }
    return beg;
}

static int find_chunk_index(const LWTTLOBJECTCACHE* c, const LWTTLCHUNKKEY chunk_key) {
    int chunk_index = lower_bound_int(&c->key_array[0].v, c->count, chunk_key.v);
    if (chunk_index >= 0 && c->key_array[chunk_index].v == chunk_key.v) {
        return chunk_index;
    }
    return -1;
}

static long long find_chunk_ts(const LWTTLOBJECTCACHE* c, const LWTTLCHUNKKEY chunk_key) {
    const int chunk_index = find_chunk_index(c, chunk_key);
    if (chunk_index >= 0 && chunk_index < c->count) {
        return c->value_array[chunk_index].ts;
    }
    return 0;
}

static int add_to_object_cache(LWTTLOBJECTCACHE* c,
                               int* cache_count,
                               void* cache_array,
                               const size_t entry_size,
                               const int entry_max_count,
                               const long long ts,
                               const int xc0,
                               const int yc0,
                               const int view_scale,
                               const int count,
                               const void* obj) {
    if (c == 0) {
        LOGEP("c == 0");
        abort();
        return -1;
    }
    if (cache_count == 0) {
        LOGEP("cache_count == 0");
        abort();
        return -2;
    }
    if (cache_array == 0) {
        LOGEP("cache_array == 0");
        abort();
        return -3;
    }
    if (entry_size == 0) {
        LOGEP("entry_size == 0");
        abort();
        return -4;
    }
    if (view_scale <= 0) {
        LOGEP("view_scale <= 0");
        abort();
        return -5;
    }
    if (obj == 0) {
        LOGEP("obj == 0");
        abort();
        return -6;
    }
    // check for existing chunk entry
    LWTTLCHUNKKEY chunk_key = make_chunk_key(xc0, yc0, view_scale);
    int chunk_index = lower_bound_int(&c->key_array[0].v, c->count, chunk_key.v);
    if (chunk_index >= 0 && c->key_array[chunk_index].v == chunk_key.v) {
        // chunk key found.
        // check for cache entry in chunk value array
        if (c->value_array[chunk_index].key.v == chunk_key.v) {
            if (c->value_array[chunk_index].ts < ts) {
                // Latest chunk arrived, so we should update our data.
                // UPDATE PROCEDURE
                // [0] Compare existing data count and updated data count
                if (c->value_array[chunk_index].count < count) {
                    // [1] Data count increased. Check capacity.
                    const int count_delta = count - c->value_array[chunk_index].count;
                    if (*cache_count + count_delta > entry_max_count) {
                        LOGEP("entry_max_count exceeded.");
                        return -7;
                    }
                    const int start0 = c->value_array[chunk_index].start;
                    const int count0 = c->value_array[chunk_index].count;
                    // [2] Make room in cache_array by shifting
                    //     cache_array[start0 + count0] to cache_array[start0 + count0 + count_delta]
                    //     in reverse order
                    for (int from = *cache_count - 1; from >= start0 + count0; from--) {
                        char* to_data = (char*)cache_array + entry_size * (from + count_delta);
                        const char* from_data = (char*)cache_array + entry_size * (from);
                        memcpy(to_data,
                               from_data,
                               entry_size);
                    }
                    // [3] For all chunks except at index chunk_index,
                    //     increment start by count_delta where
                    //     start is equal or greater to 'start0 + count0'
                    for (int i = 0; i < c->count; i++) {
                        if (i == chunk_index) {
                            continue;
                        }
                        if (c->value_array[i].start >= start0 + count0) {
                            c->value_array[i].start += count_delta;
                        }
                    }
                    // [4] Update chunk count and timestamp
                    c->value_array[chunk_index].count = count;
                    c->value_array[chunk_index].ts = ts;
                    *cache_count += count_delta;
                    // [5] Copy data
                    memcpy((char*)cache_array + entry_size * c->value_array[chunk_index].start,
                           obj,
                           entry_size * count);
                    return 1;
                } else if (c->value_array[chunk_index].count > count) {
                    // [1] Data count decreased.
                    const int count_delta = c->value_array[chunk_index].count - count;
                    const int start0 = c->value_array[chunk_index].start;
                    const int count0 = c->value_array[chunk_index].count;
                    // [2] Reduce space in cache_array by shifting
                    //     cache_array[start0 + count0] to cache_array[start0 + count0 - count_delta]
                    //     in forward order
                    for (int from = start0 + count0; from < *cache_count; from++) {
                        char* to_data = (char*)cache_array + entry_size * (from - count_delta);
                        const char* from_data = (char*)cache_array + entry_size * (from);
                        memcpy(to_data,
                               from_data,
                               entry_size);
                    }
                    // [3] For all chunks except at index chunk_index,
                    //     decrement start by count_delta where
                    //     start is equal or greater to 'start0 + count0'
                    for (int i = 0; i < c->count; i++) {
                        if (i == chunk_index) {
                            continue;
                        }
                        if (c->value_array[i].start >= start0 + count0) {
                            c->value_array[i].start -= count_delta;
                        }
                    }
                    // [4] Update chunk count and timestamp
                    c->value_array[chunk_index].count = count;
                    c->value_array[chunk_index].ts = ts;
                    *cache_count -= count_delta;
                    // [5] Copy data
                    memcpy((char*)cache_array + entry_size * c->value_array[chunk_index].start,
                           obj,
                           entry_size * count);
                    return 2;
                } else {
                    // [1] Lucky! The same count!
                    // [2] Update timestamp
                    c->value_array[chunk_index].ts = ts;
                    // [3] Copy data
                    memcpy((char*)cache_array + entry_size * c->value_array[chunk_index].start,
                           obj,
                           entry_size * count);
                    return 3;
                }
            } else {
                // chunk with older timestamp compared to our's arrived.
                // just ignore this chunk data.
                return 4;
            }
            // should not reach here!
            abort();
        } else {
            // cache key exists but cache entry not exists.
            // what's going on here?
            LOGE("Cache key found, but cache entry not found! Cache entry will be added at this time but it is strange...");
        }
    } else {
        // new chunk entry received.
        // check current capacity
        if (c->count >= TTL_OBJECT_CACHE_CHUNK_COUNT) {
            LOGEP("TTL_OBJECT_CACHE_CHUNK_COUNT exceeded.");
            return -8;
        }
        if (*cache_count + count > entry_max_count) {
            LOGEP("entry_max_count exceeded.");
            return -9;
        }
        // safe to insert a new chunk at 'chunk_index'
        // move chunk key and value (move by 1-index by copying in backward direction)
        for (int from = c->count - 1; from >= chunk_index + 1; from--) {
            c->key_array[from + 1] = c->key_array[from];
            c->value_array[from + 1] = c->value_array[from];
        }
        c->count++;
        chunk_index++;
    }
    c->key_array[chunk_index] = chunk_key;
    c->value_array[chunk_index].key = chunk_key;
    c->value_array[chunk_index].ts = ts;
    c->value_array[chunk_index].start = *cache_count;
    c->value_array[chunk_index].count = count;
    // copy data
    memcpy((char*)cache_array + entry_size * (*cache_count),
           obj,
           entry_size * count);
    // increase count indices
    *cache_count += count;
    return 0;
}

static int add_to_object_cache_land(LWTTLOBJECTCACHE* c,
                                    LWPTTLSTATICOBJECT2* land_array,
                                    const size_t land_array_size,
                                    int* land_count,
                                    const LWPTTLSTATICSTATE2* s2) {
    return add_to_object_cache(c,
                               land_count,
                               land_array,
                               sizeof(LWPTTLSTATICOBJECT2),
                               land_array_size,
                               s2->ts,
                               s2->xc0,
                               s2->yc0,
                               s2->view_scale,
                               s2->count,
                               s2->obj);
}

static int add_to_object_cache_seaport(LWTTLOBJECTCACHE* c,
                                       LWPTTLSEAPORTOBJECT* seaport_array,
                                       const size_t seaport_array_size,
                                       int* seaport_count,
                                       const LWPTTLSEAPORTSTATE* s2) {
    return add_to_object_cache(c,
                               seaport_count,
                               seaport_array,
                               sizeof(LWPTTLSEAPORTOBJECT),
                               seaport_array_size,
                               s2->ts,
                               s2->xc0,
                               s2->yc0,
                               s2->view_scale,
                               s2->count,
                               s2->obj);
}

static int add_to_object_cache_city(LWTTLOBJECTCACHE* c,
                                    LWPTTLCITYOBJECT* city_array,
                                    const size_t city_array_size,
                                    int* city_count,
                                    const LWPTTLCITYSTATE* s2) {
    return add_to_object_cache(c,
                               city_count,
                               city_array,
                               sizeof(LWPTTLCITYOBJECT),
                               city_array_size,
                               s2->ts,
                               s2->xc0,
                               s2->yc0,
                               s2->view_scale,
                               s2->count,
                               s2->obj);
}

static int add_to_object_cache_salvage(LWTTLOBJECTCACHE* c,
                                       LWPTTLSALVAGEOBJECT* salvage_array,
                                       const size_t salvage_array_size,
                                       int* salvage_count,
                                       const LWPTTLSALVAGESTATE* s2) {
    return add_to_object_cache(c,
                               salvage_count,
                               salvage_array,
                               sizeof(LWPTTLSALVAGEOBJECT),
                               salvage_array_size,
                               s2->ts,
                               s2->xc0,
                               s2->yc0,
                               s2->view_scale,
                               s2->count,
                               s2->obj);
}

static void cell_bound_to_chunk_bound(const LWTTLCELLBOUND* cell_bound, const int view_scale, LWTTLCHUNKBOUND* chunk_bound) {
    const int half_cell_pixel_extent = LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS / 2 * view_scale;
    chunk_bound->xcc0 = aligned_chunk_index(cell_bound->xc0, view_scale, LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS) >> msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
    chunk_bound->ycc0 = aligned_chunk_index(cell_bound->yc0, view_scale, LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS) >> msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
    chunk_bound->xcc1 = aligned_chunk_index(cell_bound->xc1, view_scale, LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS) >> msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
    chunk_bound->ycc1 = aligned_chunk_index(cell_bound->yc1, view_scale, LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS) >> msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
    //chunk_bound->xcc0--;
    //chunk_bound->ycc0--;
    // make 0-1 (min-max) range inclusive,exclusive style
    chunk_bound->xcc1++;
    chunk_bound->ycc1++;
}

static void send_ttlping(const LWTTL* ttl,
                         LWUDP* udp,
                         const float lng,
                         const float lat,
                         const int view_scale,
                         const float ex_lng,
                         const float ex_lat) {
    LWPTTLPING p;
    memset(&p, 0, sizeof(LWPTTLPING));
    p.type = LPGP_LWPTTLPING;
    p.lng = lng;
    p.lat = lat;
    p.ex_lng = ex_lng;
    p.ex_lat = ex_lat;
    p.track_object_id = lwttl_track_object_id(ttl);
    p.track_object_ship_id = lwttl_track_object_ship_id(ttl);
    p.view_scale = view_scale;
    udp_send(udp, (const char*)&p, sizeof(LWPTTLPING));
}

static void send_ttlpingchunk(const LWTTL* ttl,
                              LWUDP* udp,
                              const LWTTLCHUNKKEY chunk_key,
                              const unsigned char static_object,
                              const long long ts) {
    LWPTTLPINGCHUNK p;
    memset(&p, 0, sizeof(LWPTTLPINGCHUNK));
    p.type = LPGP_LWPTTLPINGCHUNK;
    p.static_object = static_object;
    p.chunk_key = chunk_key.v;
    p.ts = ts;
    udp_send(udp, (const char*)&p, sizeof(LWPTTLPINGCHUNK));
}

static void send_ttlpingsinglecell(const LWTTL* ttl,
                                   LWUDP* udp,
                                   const int xc0,
                                   const int yc0) {
    LWPTTLPINGSINGLECELL p;
    memset(&p, 0, sizeof(LWPTTLPINGSINGLECELL));
    p.type = LPGP_LWPTTLPINGSINGLECELL;
    p.xc0 = xc0;
    p.yc0 = yc0;
    udp_send(udp, (const char*)&p, sizeof(LWPTTLPINGSINGLECELL));
}

static void send_ttlpingflush(LWTTL* ttl) {
    LWPTTLPINGFLUSH p;
    memset(&p, 0, sizeof(LWPTTLPINGFLUSH));
    p.type = LPGP_LWPTTLPINGFLUSH;
    udp_send(ttl->sea_udp, (const char*)&p, sizeof(LWPTTLPINGFLUSH));
}

float lwttl_half_lng_extent_in_degrees(const int view_scale) {
    return LNGLAT_SEA_PING_EXTENT_IN_DEGREES / 2 * view_scale * LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG;
}

float lwttl_half_lat_extent_in_degrees(const int view_scale) {
    return LNGLAT_SEA_PING_EXTENT_IN_DEGREES / 2 * view_scale * LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT;
}

static void send_ttlping_with_timestamp(const LWTTL* ttl,
                                        LWUDP* udp,
                                        const LWTTLOBJECTCACHE* c,
                                        const LWTTLCHUNKKEY chunk_key,
                                        const unsigned char static_object,
                                        const int compare_ts) {
    if (compare_ts) {
        const long long ts = find_chunk_ts(c, chunk_key);
        send_ttlpingchunk(ttl,
                          udp,
                          chunk_key,
                          static_object,
                          ts);
    } else {
        if (find_chunk_index(c, chunk_key) == -1) {
            send_ttlpingchunk(ttl,
                              udp,
                              chunk_key,
                              static_object,
                              0);
        }
    }
}

void lwttl_get_cell_bound(const float lng_min,
                          const float lat_min,
                          const float lng_max,
                          const float lat_max,
                          int* xc0,
                          int* yc0,
                          int* xc1,
                          int* yc1) {
    *xc0 = lwttl_lng_to_ceil_int(lng_min);
    *yc0 = lwttl_lat_to_ceil_int(lat_max);
    *xc1 = lwttl_lng_to_floor_int(lng_max);
    *yc1 = lwttl_lat_to_floor_int(lat_min);
}

void lwttl_udp_send_ttlping(const LWTTL* ttl, LWUDP* udp, int ping_seq) {
    LWTTLCELLBOUND cell_bound;
    lwttl_get_cell_bound(ttl->viewports[0].lng_min,
                         ttl->viewports[0].lat_min,
                         ttl->viewports[0].lng_max,
                         ttl->viewports[0].lat_max,
                         &cell_bound.xc0,
                         &cell_bound.yc0,
                         &cell_bound.xc1,
                         &cell_bound.yc1);
    LWTTLCHUNKBOUND chunk_bound;
    cell_bound_to_chunk_bound(&cell_bound,
                              ttl->viewports[0].clamped_view_scale,
                              &chunk_bound);
    //chunk_bound.xcc0--;
    //chunk_bound.xcc1++;
    //chunk_bound.ycc0--;
    //chunk_bound.ycc1++;
    const int xc_count = chunk_bound.xcc1 - chunk_bound.xcc0;// +1;
    const int yc_count = chunk_bound.ycc1 - chunk_bound.ycc0;// +1;
    int chunk_index_count = 0;
    for (int i = 0; i < xc_count; i++) {
        for (int j = 0; j < yc_count; j++) {
            const int xcc0 = chunk_bound.xcc0 + i;
            const int ycc0 = chunk_bound.ycc0 + j;
            LWTTLCHUNKKEY chunk_key;
            chunk_key.bf.xcc0 = xcc0;
            chunk_key.bf.ycc0 = ycc0;
            chunk_key.bf.view_scale_msb = msb_index(ttl->viewports[0].clamped_view_scale);
            const struct {
                const LWTTLOBJECTCACHE* c;
                const unsigned char static_object;
                const int compare_ts;
            } cache_list[] = {
                { &ttl->object_cache.land_cache, LTSOT_LAND_CELL, 1 },
                { &ttl->object_cache.seaport_cache, LTSOT_SEAPORT, 1 },
                { &ttl->object_cache.city_cache, LTSOT_CITY, 1 },
                { &ttl->object_cache.salvage_cache, LTSOT_SALVAGE, 1 },
            };
            for (int k = 0; k < ARRAY_SIZE(cache_list); k++) {
                send_ttlping_with_timestamp(ttl,
                                            udp,
                                            cache_list[k].c,
                                            chunk_key,
                                            cache_list[k].static_object,
                                            cache_list[k].compare_ts);
            }
        }
    }
    // ping for dynamic object (ships)
    const int xc0 = lwttl_lng_to_round_int(ttl->viewports[0].view_center.lng) & ~(ttl->viewports[0].clamped_view_scale - 1);
    const int yc0 = lwttl_lat_to_round_int(ttl->viewports[0].view_center.lat) & ~(ttl->viewports[0].clamped_view_scale - 1);
    send_ttlping(ttl,
                 udp,
                 cell_x_to_lng(xc0),
                 cell_y_to_lat(yc0),
                 ttl->viewports[0].clamped_view_scale,
                 LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG,
                 LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT);
}

void lwttl_udp_send_ttlchat(const LWTTL* ttl, LWUDP* udp, const char* line) {
    LWPTTLCHAT p;
    memset(&p, 0, sizeof(LWPTTLCHAT));
    p.type = LPGP_LWPTTLCHAT;
    memcpy(p.line, line, sizeof(p.line));
    udp_send(udp, (const char*)&p, sizeof(LWPTTLCHAT));
}

void lwttl_udp_send_ttltransformsinglecell(const LWTTL* ttl, LWUDP* udp, int xc0, int yc0, int to) {
    LWPTTLTRANSFORMSINGLECELL p;
    memset(&p, 0, sizeof(LWPTTLTRANSFORMSINGLECELL));
    p.type = LPGP_LWPTTLTRANSFORMSINGLECELL;
    p.xc0 = xc0;
    p.yc0 = yc0;
    p.to = to;
    udp_send(udp, (const char*)&p, sizeof(LWPTTLTRANSFORMSINGLECELL));
}

void lwttl_udp_send_request_waypoints(const LWTTL* ttl, LWUDP* sea_udp, int ship_id) {
    LWPTTLREQUESTWAYPOINTS p;
    memset(&p, 0, sizeof(LWPTTLREQUESTWAYPOINTS));
    p.type = LPGP_LWPTTLREQUESTWAYPOINTS;
    p.ship_id = ship_id;
    udp_send(sea_udp, (const char*)&p, sizeof(LWPTTLREQUESTWAYPOINTS));
}

void lwttl_lock_rendering_mutex(LWTTL* ttl) {
    LWMUTEX_LOCK(ttl->rendering_mutex);
}

void lwttl_unlock_rendering_mutex(LWTTL* ttl) {
    LWMUTEX_UNLOCK(ttl->rendering_mutex);
}

void lwttl_set_sea_udp(LWTTL* ttl, LWUDP* sea_udp) {
    ttl->sea_udp = sea_udp;
}

static void set_ttl_full_state(LWTTL* ttl, const LWPTTLROUTESTATE* p) {
    memcpy(&ttl->ttl_dynamic_state, p, sizeof(LWPTTLROUTESTATE));
    for (int i = 0; i < p->count; i++) {
        const int ship_id = p->obj[i].db_id;
        int cache_hit = 0;
        for (int j = 0; j < ttl->waypoints_cache_count; j++) {
            if (ttl->waypoints_cache[j].ship_id == ship_id) {
                cache_hit = 1;
            }
        }
        if (cache_hit == 0) {
            lwttl_request_waypoints(ttl, ship_id);
        }
    }
}

static void set_ttl_waypoints(LWTTL* ttl, const LWPTTLWAYPOINTS* p) {
    memcpy(&ttl->waypoints, p, sizeof(LWPTTLWAYPOINTS));
    // overwrite if exist
    for (int i = 0; i < ttl->waypoints_cache_count; i++) {
        if (ttl->waypoints_cache[i].ship_id == p->ship_id) {
            memcpy(&ttl->waypoints_cache[i], p, sizeof(LWPTTLWAYPOINTS));
            return;
        }
    }
    // new!
    if (ttl->waypoints_cache_count < ARRAY_SIZE(ttl->waypoints_cache)) {
        memcpy(&ttl->waypoints_cache[ttl->waypoints_cache_count], p, sizeof(LWPTTLWAYPOINTS));
        ttl->waypoints_cache_count++;
    } else {
        LOGEP("maximum capacity exceeded");
    }
}

static void update_world_text(LWTTL* ttl, float delta_time) {
    for (int i = 0; i < ARRAY_SIZE(ttl->world_text); i++) {
        if (ttl->world_text[i].valid) {
            ttl->world_text[i].age += delta_time;
            if (ttl->world_text[i].age > ttl->world_text[i].lifetime) {
                ttl->world_text[i].valid = 0;
            }
        }
    }
}

static LWTTLWORLDTEXT* empty_world_text(LWTTL* ttl) {
    for (int i = 0; i < ARRAY_SIZE(ttl->world_text); i++) {
        if (ttl->world_text[i].valid == 0) {
            return &ttl->world_text[i];
        }
    }
    return 0;
}

static const LWTTLWORLDTEXT* valid_world_text_start(const LWTTL* ttl, const LWTTLWORLDTEXT* start_it) {
    const int i0 = ((const char*)start_it - (const char*)ttl->world_text) / sizeof(LWTTLWORLDTEXT);
    for (int i = i0; i < ARRAY_SIZE(ttl->world_text); i++) {
        if (ttl->world_text[i].valid) {
            return &ttl->world_text[i];
        }
    }
    return 0;
}

static const LWTTLWORLDTEXT* first_valid_world_text(const LWTTL* ttl) {
    return valid_world_text_start(ttl, ttl->world_text);
}

static void spawn_world_text_move(LWTTL* ttl, const char* text, int xc, int yc, int xc1, int yc1, LW_TTL_WORLD_TEXT_ANIM_TYPE anim_type) {
    LWTTLWORLDTEXT* world_text = empty_world_text(ttl);
    if (world_text) {
        world_text->valid = 1;
        world_text->age = 0;
        world_text->lifetime = 1.0f;
        world_text->move_dist = 5.0f;
        strncpy(world_text->text, text, ARRAY_SIZE(world_text->text));
        world_text->text[ARRAY_SIZE(world_text->text) - 1] = 0;
        world_text->xc = xc;
        world_text->yc = yc;
        world_text->xc1 = xc1;
        world_text->yc1 = yc1;
        world_text->anim_type = anim_type;
    } else {
        LOGEP("size exceeded");
    }
}

static void spawn_world_text(LWTTL* ttl, const char* text, int xc, int yc) {
    spawn_world_text_move(ttl, text, xc, yc, xc, yc, LTWTAT_UP);
}

const LWPTTLWAYPOINTS* lwttl_get_waypoints(const LWTTL* ttl) {
    return &ttl->waypoints;
}

const LWPTTLWAYPOINTS* lwttl_get_waypoints_by_ship_id(const LWTTL* ttl, int ship_id) {
    for (int i = 0; i < ttl->waypoints_cache_count; i++) {
        if (ttl->waypoints_cache[i].ship_id == ship_id) {
            return &ttl->waypoints_cache[i];
        }
    }
    return 0;
}

int lwttl_get_waypoints_count(const LWTTL* ttl) {
    return ttl->waypoints_cache_count;
}

const LWPTTLWAYPOINTS* lwttl_get_waypoints_by_index(const LWTTL* ttl, int index) {
    return &ttl->waypoints_cache[index];
}

void lwttl_write_last_state(const LWTTL* ttl, const LWCONTEXT* pLwc) {
    LWTTLSAVEDATA save;
    save.magic = LOCAL_SAVE_FILE_MAGIC;
    save.version = LOCAL_SAVE_FILE_LATEST_VERSION;
    save.lng = ttl->viewports[0].view_center.lng;
    save.lat = ttl->viewports[0].view_center.lat;
    save.view_scale = lwttl_view_scale(ttl);
    memcpy(save.cam_eye, ttl->viewports[0].cam_eye, sizeof(vec3));
    memcpy(save.cam_look_at, ttl->viewports[0].cam_look_at, sizeof(vec3));
    write_file_binary(pLwc->user_data_path, "lwttl.dat", (const char*)&save, sizeof(LWTTLSAVEDATA));
}

void lwttl_read_last_state(LWTTL* ttl, const LWCONTEXT* pLwc) {
    if (is_file_exist(pLwc->user_data_path, "lwttl.dat")) {
        LWTTLSAVEDATA save;
        memset(&save, 0, sizeof(LWTTLSAVEDATA));
        int ret = read_file_binary(pLwc->user_data_path, "lwttl.dat", sizeof(LWTTLSAVEDATA), (char*)&save);
        if (ret == 0) {
            if (save.magic != LOCAL_SAVE_FILE_MAGIC) {
                LOGIP("TTL save data magic not match. Will be ignored and rewritten upon exit.");
            } else if (save.version != LOCAL_SAVE_FILE_LATEST_VERSION) {
                LOGIP("TTL save data version not match. Will be ignored and rewritten upon exit.");
            } else {
                lwttl_worldmap_scroll_to(ttl, save.lng, save.lat, 0);
                lwttl_set_cam_eye(ttl, save.cam_eye);
                lwttl_set_cam_look_at(ttl, save.cam_look_at);
                lwttl_set_viewport_view_scale(ttl, 0, save.view_scale);
            }
        } else {
            LOGEP("TTL save data read failed! - return code: %d", ret);
        }
    }
}

const LWPTTLROUTESTATE* lwttl_full_state(const LWTTL* ttl) {
    return &ttl->ttl_dynamic_state;
}

static int lwttl_query_chunk_range(const float lng_min,
                                   const float lat_min,
                                   const float lng_max,
                                   const float lat_max,
                                   const int view_scale,
                                   const LWTTLOBJECTCACHE* c,
                                   int* chunk_index_array,
                                   const int chunk_index_array_len,
                                   int* xcc0,
                                   int* ycc0,
                                   int* xcc1,
                                   int* ycc1) {
    LWTTLCELLBOUND cell_bound;
    lwttl_get_cell_bound(lng_min,
                         lat_min,
                         lng_max,
                         lat_max,
                         &cell_bound.xc0,
                         &cell_bound.yc0,
                         &cell_bound.xc1,
                         &cell_bound.yc1);
    LWTTLCHUNKBOUND chunk_bound;
    cell_bound_to_chunk_bound(&cell_bound, view_scale, &chunk_bound);
    *xcc0 = chunk_bound.xcc0;
    *ycc0 = chunk_bound.ycc0;
    *xcc1 = chunk_bound.xcc1;
    *ycc1 = chunk_bound.ycc1;
    const int xc_count = chunk_bound.xcc1 - chunk_bound.xcc0;
    const int yc_count = chunk_bound.ycc1 - chunk_bound.ycc0;
    int chunk_index_count = 0;
    for (int i = 0; i < xc_count; i++) {
        for (int j = 0; j < yc_count; j++) {
            const int xcc0 = chunk_bound.xcc0 + i;
            const int ycc0 = chunk_bound.ycc0 + j;
            LWTTLCHUNKKEY chunk_key;
            chunk_key.bf.xcc0 = xcc0;
            chunk_key.bf.ycc0 = ycc0;
            chunk_key.bf.view_scale_msb = msb_index(view_scale);
            const int chunk_index = find_chunk_index(c, chunk_key);
            if (chunk_index >= 0) {
                if (chunk_index_count < chunk_index_array_len) {
                    chunk_index_array[chunk_index_count] = chunk_index;
                }
                chunk_index_count++;
            }
        }
    }
    return chunk_index_count;
}

int lwttl_query_chunk_range_land_vp(const LWTTL* ttl,
                                    const LWTTLFIELDVIEWPORT* vp,
                                    int* chunk_index_array,
                                    const int chunk_index_array_len,
                                    int* xcc0,
                                    int* ycc0,
                                    int* xcc1,
                                    int* ycc1) {
    return lwttl_query_chunk_range_land(ttl,
                                        vp->lng_min,
                                        vp->lat_min,
                                        vp->lng_max,
                                        vp->lat_max,
                                        vp->clamped_view_scale,
                                        chunk_index_array,
                                        chunk_index_array_len,
                                        xcc0,
                                        ycc0,
                                        xcc1,
                                        ycc1);
}

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
                                 int* ycc1) {
    return lwttl_query_chunk_range(lng_min,
                                   lat_min,
                                   lng_max,
                                   lat_max,
                                   view_scale,
                                   &ttl->object_cache.land_cache,
                                   chunk_index_array,
                                   chunk_index_array_len,
                                   xcc0,
                                   ycc0,
                                   xcc1,
                                   ycc1);
}

int lwttl_query_chunk_range_seaport_vp(const LWTTL* ttl,
                                       const LWTTLFIELDVIEWPORT* vp,
                                       int* chunk_index_array,
                                       const int chunk_index_array_len,
                                       int* xcc0,
                                       int* ycc0,
                                       int* xcc1,
                                       int* ycc1) {
    return lwttl_query_chunk_range_seaport(ttl,
                                           vp->lng_min,
                                           vp->lat_min,
                                           vp->lng_max,
                                           vp->lat_max,
                                           vp->clamped_view_scale,
                                           chunk_index_array,
                                           chunk_index_array_len,
                                           xcc0,
                                           ycc0,
                                           xcc1,
                                           ycc1);
}

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
                                    int* ycc1) {
    return lwttl_query_chunk_range(lng_min,
                                   lat_min,
                                   lng_max,
                                   lat_max,
                                   view_scale,
                                   &ttl->object_cache.seaport_cache,
                                   chunk_index_array,
                                   chunk_index_array_len,
                                   xcc0,
                                   ycc0,
                                   xcc1,
                                   ycc1);
}

int lwttl_query_chunk_range_city_vp(const LWTTL* ttl,
                                    const LWTTLFIELDVIEWPORT* vp,
                                    int* chunk_index_array,
                                    const int chunk_index_array_len,
                                    int* xcc0,
                                    int* ycc0,
                                    int* xcc1,
                                    int* ycc1) {
    return lwttl_query_chunk_range_city(ttl,
                                        vp->lng_min,
                                        vp->lat_min,
                                        vp->lng_max,
                                        vp->lat_max,
                                        vp->clamped_view_scale,
                                        chunk_index_array,
                                        chunk_index_array_len,
                                        xcc0,
                                        ycc0,
                                        xcc1,
                                        ycc1);
}

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
                                 int* ycc1) {
    return lwttl_query_chunk_range(lng_min,
                                   lat_min,
                                   lng_max,
                                   lat_max,
                                   view_scale,
                                   &ttl->object_cache.city_cache,
                                   chunk_index_array,
                                   chunk_index_array_len,
                                   xcc0,
                                   ycc0,
                                   xcc1,
                                   ycc1);
}

int lwttl_query_chunk_range_salvage_vp(const LWTTL* ttl,
                                       const LWTTLFIELDVIEWPORT* vp,
                                       int* chunk_index_array,
                                       const int chunk_index_array_len,
                                       int* xcc0,
                                       int* ycc0,
                                       int* xcc1,
                                       int* ycc1) {
    return lwttl_query_chunk_range_salvage(ttl,
                                           vp->lng_min,
                                           vp->lat_min,
                                           vp->lng_max,
                                           vp->lat_max,
                                           vp->clamped_view_scale,
                                           chunk_index_array,
                                           chunk_index_array_len,
                                           xcc0,
                                           ycc0,
                                           xcc1,
                                           ycc1);
}

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
                                    int* ycc1) {
    return lwttl_query_chunk_range(lng_min,
                                   lat_min,
                                   lng_max,
                                   lat_max,
                                   view_scale,
                                   &ttl->object_cache.salvage_cache,
                                   chunk_index_array,
                                   chunk_index_array_len,
                                   xcc0,
                                   ycc0,
                                   xcc1,
                                   ycc1);
}

static const void* lwttl_query_chunk(const LWTTL* ttl,
                                     const LWTTLOBJECTCACHE* c,
                                     const int chunk_index,
                                     const void* cache_array,
                                     const size_t entry_size,
                                     int* xc0,
                                     int* yc0,
                                     int* count) {
    if (chunk_index >= 0 && chunk_index < c->count) {
        const int start = c->value_array[chunk_index].start;
        const int view_scale = 1 << c->key_array[chunk_index].bf.view_scale_msb;
        *xc0 = c->key_array[chunk_index].bf.xcc0 << msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
        *yc0 = c->key_array[chunk_index].bf.ycc0 << msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
        *count = c->value_array[chunk_index].count;
        return (const char*)cache_array + start * entry_size;
    } else {
        return 0;
    }
}

const LWPTTLSTATICOBJECT2* lwttl_query_chunk_land(const LWTTL* ttl,
                                                  const int chunk_index,
                                                  int* xc0,
                                                  int* yc0,
                                                  int* count) {
    return lwttl_query_chunk(ttl,
                             &ttl->object_cache.land_cache,
                             chunk_index,
                             ttl->object_cache.land_array,
                             sizeof(LWPTTLSTATICOBJECT2),
                             xc0,
                             yc0,
                             count);
}

const LWPTTLSEAPORTOBJECT* lwttl_query_chunk_seaport(const LWTTL* ttl,
                                                     const int chunk_index,
                                                     int* xc0,
                                                     int* yc0,
                                                     int* count) {
    return lwttl_query_chunk(ttl,
                             &ttl->object_cache.seaport_cache,
                             chunk_index,
                             ttl->object_cache.seaport_array,
                             sizeof(LWPTTLSEAPORTOBJECT),
                             xc0,
                             yc0,
                             count);
}

const LWPTTLCITYOBJECT* lwttl_query_chunk_city(const LWTTL* ttl,
                                               const int chunk_index,
                                               int* xc0,
                                               int* yc0,
                                               int* count) {
    return lwttl_query_chunk(ttl,
                             &ttl->object_cache.city_cache,
                             chunk_index,
                             ttl->object_cache.city_array,
                             sizeof(LWPTTLCITYOBJECT),
                             xc0,
                             yc0,
                             count);
}

const LWPTTLSALVAGEOBJECT* lwttl_query_chunk_salvage(const LWTTL* ttl,
                                                     const int chunk_index,
                                                     int* xc0,
                                                     int* yc0,
                                                     int* count) {
    return lwttl_query_chunk(ttl,
                             &ttl->object_cache.salvage_cache,
                             chunk_index,
                             ttl->object_cache.salvage_array,
                             sizeof(LWPTTLSALVAGEOBJECT),
                             xc0,
                             yc0,
                             count);
}

LWUDP* lwttl_sea_udp(LWTTL* ttl) {
    return ttl->sea_udp;
}

void lwttl_scroll_view_scale(LWTTL* ttl, float offset) {
    int view_scale = lwttl_view_scale(ttl);
    lwttl_set_view_scale(ttl,
                         LWCLAMP(offset > 0 ? (view_scale >> 1) : (view_scale << 1),
                                 1,
                                 ttl->view_scale_max));
    lwttl_send_ping_now(ttl);
}

float lwttl_earth_globe_y(const LWTTLLNGLAT* center, float earth_globe_scale) {
    //const float globe_y = -sinf((float)LWDEG2RAD(center->lat));
    const float plane_y = -(float)M_PI / 2 * center->lat / 90.0f * earth_globe_scale; // plane height is M_PI
    return plane_y;
}

void lwttl_fill_world_seaports_bookmarks(LWHTMLUI* htmlui) {
    char script[128];
    htmlui_clear_loop(htmlui, "world-seaport");

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "Yokohama");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            139.0f + 39.0f / 60.0f,
            35.0f + 27.0f / 60.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "Shanghai");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            121.0f + 29.0f / 60.0f,
            31.0f + 14.0f / 60.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "Tianjin");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            117.0f + 12.0f / 60.0f,
            39.0f + 2.0f / 60.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "Busan");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            129.0f + 3.0f / 60.0f,
            35.0f + 8.0f / 60.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "Panama Canal");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            -(79.0f + 40.0f / 60.0f),
            9.0f + 4.0f / 60.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "As Suways(Suez)");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            32.0f + 31.0f / 60.0f,
            29.0f + 58.0f / 60.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "Hong Kong Central");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            114.0f + 9.0f / 60.0f,
            22.0f + 16.0f / 60.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "Jurong/Singapore");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            103.0f + 42.0f / 60.0f,
            1.0f + 20.0f / 60.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "Port Of South Louisiana");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            -(90 + 30 / 60.0f),
            30 + 3 / 60.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "Pilottown");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            -(89 + 15 / 60.0f),
            29 + 11 / 60.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "Sydney");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            151 + 12 / 60.0f,
            -(33 + 51 / 60.0f));
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "Honolulu");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            -(157 + 51 / 60.0f),
            21 + 18 / 60.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "Seoul");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            126 + 56 / 60.0f,
            37 + 31 / 60.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "Civitavecchia");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            11 + 48 / 60.0f,
            42 + 6 / 60.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "Rome (Roma)");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            12 + 29 / 60.0f,
            41 + 54 / 60.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "Sunderland");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            -(1 + 23 / 60.0f),
            54 + 54 / 60.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "Dokdo");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            131 + 51 / 60.0f,
            37 + 14 / 60.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "Dokdo Precise");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            131 + 52 / 60.0f + 8.045f / 3600.0f,
            37 + 14 / 60.0f + 21.786f / 3600.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "[0,0] (LNG -180.0 LAT 90.0)");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            -180 + 0 / 60.0f,
            90 + 0 / 60.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);

    htmlui_set_loop_key_value(htmlui, "world-seaport", "name", "[C,C] (LNG 0.0 LAT 0.0)");
    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
            0.0f,
            0.0f);
    htmlui_set_loop_key_value(htmlui, "world-seaport", "script", script);
}

void lwttl_send_ping_now(LWTTL* ttl) {
    lwttl_udp_send_ttlping(ttl, ttl->sea_udp, 0);
}

void lwttl_prerender_mutable_context(LWTTL* ttl, LWCONTEXT* pLwc, LWHTMLUI* htmlui) {
    if (htmlui && htmlui_get_refresh_html_body(htmlui)) {
        const char* send_ping_now = "local c = lo.script_context();lo.lwttl_send_ping_now(c.ttl)";
        logic_emit_evalute_with_name_async(pLwc,
                                           send_ping_now,
                                           strlen(send_ping_now),
                                           "send_ping_now");
    }
}

int lwttl_selected(const LWTTL* ttl, LWTTLLNGLAT* pos) {
    if (pos) {
        memcpy(pos, &ttl->viewports[0].selected.pos, sizeof(LWTTLLNGLAT));
    }
    return ttl->viewports[0].selected.selected;
}

int lwttl_selected_int(const LWTTL* ttl, int* xc0, int* yc0) {
    *xc0 = lwttl_selected_int_x(ttl);
    *yc0 = lwttl_selected_int_y(ttl);
    return ttl->viewports[0].selected.selected;
}

int lwttl_selected_int_x(const LWTTL* ttl) {
    return ttl->viewports[0].selected.pos_xc;
}

int lwttl_selected_int_y(const LWTTL* ttl) {
    return ttl->viewports[0].selected.pos_yc;
}

static float inner_product(const float a[3], const float b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void lwttl_screen_to_world_pos(const LWTTL* ttl,
                               const float touchnx,
                               const float touchny,
                               vec2 world_pos) {
    // Auxiliary matrix and vectors
    // to deal with ogl.
    mat4x4 inverted_matrix;
    mat4x4 transform_matrix;
    vec4 normalized_in_point;
    vec4 out_point;

    /* Transform the screen point to clip space in ogl (-1,1) */
    normalized_in_point[0] = touchnx;
    normalized_in_point[1] = touchny;
    normalized_in_point[2] = -1.0f;
    normalized_in_point[3] = 1.0f;

    /* Obtain the transform matrix and
    then the inverse. */
    mat4x4_mul(transform_matrix, ttl->viewports[0].proj, ttl->viewports[0].view /* ttl->view_model */);
    mat4x4_invert(inverted_matrix, transform_matrix);
    /* Apply the inverse to the point
    in clip space */
    mat4x4_mul_vec4(out_point, inverted_matrix, normalized_in_point);

    if (out_point[3] == 0.0) {
        // Avoid /0 error.
        LOGE("World coords ERROR!");
        world_pos[0] = 0;
        world_pos[1] = 0;
    } else {
        // Divide by the 3rd component to find
        // out the real position.
        out_point[0] /= out_point[3];
        out_point[1] /= out_point[3];
        out_point[2] /= out_point[3];

        // project out_point to z=0 plane and get (x, y)
        // assuming we are on orthographic projection

        const float cam_to_look_at[3] = {
            ttl->viewports[0].cam_look_at[0] - ttl->viewports[0].cam_eye[0],
            ttl->viewports[0].cam_look_at[1] - ttl->viewports[0].cam_eye[1],
            ttl->viewports[0].cam_look_at[2] - ttl->viewports[0].cam_eye[2],
        };

        const float d = -out_point[2] / cam_to_look_at[2];
        const float p[3] = {
            out_point[0] + d * cam_to_look_at[0],
            out_point[1] + d * cam_to_look_at[1],
            out_point[2] + d * cam_to_look_at[2],
        };

        world_pos[0] = p[0];
        world_pos[1] = p[1];
    }
}

static void nx_ny_to_lng_lat(const LWTTL* ttl,
                             float nx,
                             float ny,
                             int width,
                             int height,
                             int* xc,
                             int* yc,
                             LWTTLLNGLAT* lnglat) {
    vec2 world_pos;
    lwttl_screen_to_world_pos(ttl,
                              nx,
                              ny,
                              world_pos);
    const LWTTLLNGLAT* center = &ttl->viewports[0].view_center;
    const int view_scale = lwttl_view_scale(ttl);
    lnglat->lng = render_coords_to_lng(world_pos[0], center, view_scale);
    lnglat->lat = render_coords_to_lat(world_pos[1], center, view_scale);
    *xc = lwttl_lng_to_floor_int(lnglat->lng);
    *yc = lwttl_lat_to_floor_int(lnglat->lat);
}

void lwttl_change_selected_cell_to(LWTTL* ttl,
                                   int xc,
                                   int yc,
                                   const LWTTLLNGLAT* lnglat) {
    // check touch press cell and touch release cell are the same cell
    LWTTLSELECTED* selected = &ttl->viewports[0].selected;
    if (selected->press_pos_xc == xc
        && selected->press_pos_yc == yc) {
        // check the same cell selected twice
        if (selected->selected == 1
            && selected->pos_xc == xc
            && selected->pos_yc == yc) {
            // selected the same cell
            //selected->selected = 0;
            selected->selected_cell_height = -selected->selected_cell_max_height;
        } else {
            // select a new cell
            memset(&ttl->ttl_single_cell, 0, sizeof(LWPTTLSINGLECELL));
            selected->selected = 1;
            selected->pos = *lnglat;
            selected->pos_xc = xc;
            selected->pos_yc = yc;
            selected->selected_cell_height = -selected->selected_cell_max_height;
            send_ttlpingsinglecell(ttl, ttl->sea_udp, xc, yc);

            // set aux1 viewport center
            LWTTLLNGLAT selected_center_cell;
            selected_center_cell.lng = cell_fx_to_lng(xc + 0.5f);
            selected_center_cell.lat = cell_fy_to_lat(yc + 0.5f);
            lwttl_update_viewport_data(ttl,
                                       &ttl->viewports[1],
                                       ttl->viewports[1].field_viewport_x,
                                       ttl->viewports[1].field_viewport_y,
                                       ttl->viewports[1].field_viewport_width,
                                       ttl->viewports[1].field_viewport_height,
                                       ttl->viewports[1].view_scale,
                                       &selected_center_cell,
                                       ttl->viewports[1].half_lng_extent_in_deg,
                                       ttl->viewports[1].half_lat_extent_in_deg);
        }
    }
}

void lwttl_on_press(LWTTL* ttl, const LWCONTEXT* pLwc, float nx, float ny) {
    LWTTLSELECTED* selected = &ttl->viewports[0].selected;
    int xc, yc;
    LWTTLLNGLAT lnglat;
    nx_ny_to_lng_lat(ttl,
                     nx,
                     ny,
                     ttl->viewports[0].field_viewport_width,
                     ttl->viewports[0].field_viewport_height,
                     &xc,
                     &yc,
                     &lnglat);
    selected->press_pos_xc = xc;
    selected->press_pos_yc = yc;
    selected->pressing = 1;
    selected->press_at = (float)pLwc->app_time;
}

void lwttl_on_move(LWTTL* ttl, const LWCONTEXT* pLwc, float nx, float ny) {
    LWTTLSELECTED* selected = &ttl->viewports[0].selected;
    if (selected->dragging) {
        int xc, yc;
        LWTTLLNGLAT lnglat;
        nx_ny_to_lng_lat(ttl,
                         nx,
                         ny,
                         ttl->viewports[0].field_viewport_width,
                         ttl->viewports[0].field_viewport_height,
                         &xc,
                         &yc,
                         &lnglat);
        selected->dragging_pos_xc = xc;
        selected->dragging_pos_yc = yc;
    }
}

void lwttl_on_release(LWTTL* ttl, LWCONTEXT* pLwc, float nx, float ny) {
    LWTTLSELECTED* selected = &ttl->viewports[0].selected;
    if (selected->pressing) {
        int xc, yc;
        LWTTLLNGLAT lnglat;
        nx_ny_to_lng_lat(ttl,
                         nx,
                         ny,
                         ttl->viewports[0].field_viewport_width,
                         ttl->viewports[0].field_viewport_height,
                         &xc,
                         &yc,
                         &lnglat);
        lwttl_change_selected_cell_to(ttl,
                                      xc,
                                      yc,
                                      &lnglat);
        selected->pressing = 0;
    }
    if (selected->dragging) {
        if (selected->pos_xc != selected->dragging_pos_xc || selected->pos_yc != selected->dragging_pos_yc) {
            //htmlui_execute_anchor_click(pLwc->htmlui, "/link");
            script_evaluate_async(pLwc, "link()", strlen("link()"));
        }
        selected->dragging = 0;
    }
}

void lwttl_view_proj(const LWTTL* ttl, mat4x4 view, mat4x4 proj) {
    memcpy(view, ttl->viewports[0].view, sizeof(mat4x4));
    memcpy(proj, ttl->viewports[0].proj, sizeof(mat4x4));
}

void lwttl_update_view_proj(const LWTTLFIELDVIEWPORT* vp,
                            const LWTTLFIELDVIEWPORT* vp0,
                            const int viewport_width,
                            const int viewport_height,
                            mat4x4 view,
                            mat4x4 proj) {
    const float aspect_ratio = (float)viewport_width / viewport_height;
    // half_height := (how many cells in vertical axis) / 2
    // (relative to main viewport height)
    float half_height = 6.0f * (float)viewport_height / vp0->field_viewport_height;
    float near_z = 0.1f;
    float far_z = 1000.0f;
    // eye(camera) position
    vec3 eye = {
        vp->cam_eye[0],
        vp->cam_eye[1],
        vp->cam_eye[2]
    };
    // look position
    vec3 center = {
        vp->cam_look_at[0],
        vp->cam_look_at[1],
        vp->cam_look_at[2]
    };
    vec3 center_to_eye;
    vec3_sub(center_to_eye, eye, center);
    float cam_a = atan2f(center_to_eye[1], center_to_eye[0]);
    // right := rotate (1, 0, 0) by cam_a in +Z axis
    vec3 right = { 0, 0, 0 };
    if (center_to_eye[0] == 0 && center_to_eye[1] == 0) {
        right[0] = 1;
    } else {
        right[0] = cosf((float)(M_PI / 2) + cam_a);
        right[1] = sinf((float)(M_PI / 2) + cam_a);
    }
    vec3 eye_right;
    vec3_mul_cross(eye_right, center_to_eye, right);
    vec3 up;
    vec3_norm(up, eye_right);
    mat4x4_ortho(proj,
                 -half_height * aspect_ratio,
                 +half_height * aspect_ratio,
                 -half_height,
                 +half_height,
                 near_z,
                 far_z);
    mat4x4_look_at(view, eye, center, up);
}

void lwttl_clear_selected_pressed_pos(LWTTL* ttl) {
    LWTTLSELECTED* selected = &ttl->viewports[0].selected;
    selected->press_pos_xc = -1;
    selected->press_pos_yc = -1;
}

const LWPTTLSINGLECELL* lwttl_single_cell(const LWTTL* ttl) {
    return &ttl->ttl_single_cell;
}

int lwttl_press_menu_info(const LWTTL* ttl,
                          float* press_menu_gauge_total,
                          float* press_menu_gauge_appear_delay,
                          float* press_at) {
    const LWTTLSELECTED* selected = &ttl->viewports[0].selected;
    *press_menu_gauge_total = selected->press_menu_gauge_total;
    *press_menu_gauge_appear_delay = selected->press_menu_gauge_appear_delay;
    *press_at = selected->press_at;
    return selected->press_pos_xc >= 0
        && selected->press_pos_yc >= 0
        && selected->pressing;
}

int lwttl_press_ring_info(const LWTTL* ttl,
                          const float app_time,
                          float* press_menu_gauge_current,
                          float* press_menu_gauge_total) {
    float press_menu_gauge_appear_delay;
    float press_at;
    if (lwttl_press_menu_info(ttl,
                              press_menu_gauge_total,
                              &press_menu_gauge_appear_delay,
                              &press_at)
        && app_time > press_at + press_menu_gauge_appear_delay
        && app_time - press_at < *press_menu_gauge_total) {
        *press_menu_gauge_current = app_time - press_at;
        return 1;
    }
    return 0;
}

int lwttl_dragging_info(const LWTTL* ttl,
                        int* xc0,
                        int* yc0,
                        int* xc1,
                        int* yc1) {
    const LWTTLSELECTED* selected = &ttl->viewports[0].selected;
    *xc0 = selected->pos_xc;
    *yc0 = selected->pos_yc;
    *xc1 = selected->dragging_pos_xc;
    *yc1 = selected->dragging_pos_yc;
    return selected->dragging;
}

const char* lwttl_get_or_create_user_id(LWTTL* ttl,
                                        LWCONTEXT* pLwc) {
    if (is_file_exist(pLwc->user_data_path, "ttl-user-id.dat") == 0) {
        sprintf(ttl->user_id_str, "%08X%08X%08X%08X",
                pcg32_random(),
                pcg32_random(),
                pcg32_random(),
                pcg32_random());
        write_file_string(pLwc->user_data_path, "ttl-user-id.dat", ttl->user_id_str);
    } else {
        read_file_string(pLwc->user_data_path, "ttl-user-id.dat", sizeof(ttl->user_id_str), ttl->user_id_str);
    }
    return ttl->user_id_str;
}

int lwttl_ping_send_interval_multiplier(const LWTTL* ttl) {
    if (ttl->panning) {
        return 50;
    } else {
        return 200;
    }
}

const void* lwttl_world_text_begin(const LWTTL* ttl) {
    return first_valid_world_text(ttl);
}

const char* lwttl_world_text_vp(const LWTTL* ttl,
                                const void* it,
                                const LWTTLFIELDVIEWPORT* vp,
                                const mat4x4 proj_view,
                                float* ui_point_x,
                                float* ui_point_y,
                                float* scale) {
    return lwttl_world_text(ttl,
                            it,
                            &vp->view_center,
                            vp->field_viewport_aspect_ratio,
                            proj_view,
                            vp->view_scale,
                            ui_point_x,
                            ui_point_y,
                            scale);
}

const char* lwttl_world_text(const LWTTL* ttl,
                             const void* it,
                             const LWTTLLNGLAT* center,
                             const float aspect_ratio,
                             const mat4x4 proj_view,
                             const int view_scale,
                             float* ui_point_x,
                             float* ui_point_y,
                             float* scale) {
    const LWTTLWORLDTEXT* wt = (const LWTTLWORLDTEXT*)it;
    if (wt < ttl->world_text || wt >= &ttl->world_text[ARRAY_SIZE(ttl->world_text)]) {
        LOGEP("out of bound it");
        return 0;
    }
    if (((const char*)wt - (const char*)ttl->world_text) % sizeof(LWTTLWORLDTEXT) != 0) {
        LOGEP("misaligned it");
        return 0;
    }
    const float age = wt->age;
    float lifetime = wt->lifetime;
    if (lifetime <= 0) {
        lifetime = 1;
    }
    const float x = cell_fx_to_render_coords((float)wt->xc + 0.5f, center, view_scale);
    const float y = cell_fy_to_render_coords((float)wt->yc + 0.5f, center, view_scale);
    vec4 obj_pos_vec4 = {
        x,
        y,
        0,
        1,
    };
    vec2 ui_point;
    calculate_ui_point_from_world_point(aspect_ratio, proj_view, obj_pos_vec4, ui_point);
    *scale = 1.0f;
    const float ratio = LWCLAMP(age / lifetime, 0.0f, 1.0f);
    if (wt->anim_type == LTWTAT_UP) {
        // move +Y direction animation
        ui_point[1] += ratio / 5.0f;
    } else if (wt->anim_type == LTWTAT_DOWN) {
        // move -Y direction animation
        ui_point[1] += 1.0f / 5.0f - ratio / 5.0f;
    } else if (wt->anim_type == LTWTAT_MOVE) {
        const float x1 = cell_fx_to_render_coords((float)wt->xc1 + 0.5f, center, view_scale);
        const float y1 = cell_fy_to_render_coords((float)wt->yc1 + 0.5f, center, view_scale);
        const vec4 obj_pos1_vec4 = {
            x1,
            y1,
            0,
            1,
        };
        vec2 ui_point1;
        calculate_ui_point_from_world_point(aspect_ratio, proj_view, obj_pos1_vec4, ui_point1);
        ui_point[0] = ui_point[0] * (1.0f - ratio) + ui_point1[0] * ratio;
        ui_point[1] = ui_point[1] * (1.0f - ratio) + ui_point1[1] * ratio;
    } else if (wt->anim_type == LTWTAT_SCALE_0_TO_1_TO_0) {
        if (ratio < 0.5f) {
            *scale = 2.0f * ratio;
        } else {
            *scale = -2.0f * ratio + 2.0f;
        }
    } else if (wt->anim_type == LTWTAT_SCALE_1_TO_0) {
        *scale = -ratio + 1.0f;
    } else if (wt->anim_type == LTWTAT_SCALE_0_TO_1) {
        *scale = ratio;
    } else {
        // no anim
    }

    *ui_point_x = ui_point[0];
    *ui_point_y = ui_point[1];

    return wt->text;
}

const void* lwttl_world_text_next(const LWTTL* ttl, const void* it) {
    const LWTTLWORLDTEXT* wt = (const LWTTLWORLDTEXT*)it;
    return valid_world_text_start(ttl, wt + 1);
}

void lwttl_udp_update(LWTTL* ttl, LWCONTEXT* pLwc) {
    if (ttl == 0 || ttl->sea_udp == 0) {
        return;
    }
    if (pLwc == 0 || pLwc->game_scene != LGS_TTL) {
        return;
    }
    if (ttl->sea_udp->reinit_next_update) {
        destroy_udp(&ttl->sea_udp);
        ttl->sea_udp = new_udp();
        udp_update_addr_host(ttl->sea_udp,
                             pLwc->sea_udp_host_addr.host,
                             pLwc->sea_udp_host_addr.port,
                             pLwc->sea_udp_host_addr.port_str);
        ttl->sea_udp->reinit_next_update = 0;
    }
    if (ttl->sea_udp == 0) {
        LOGEP("ttl->sea_udp null");
    }
    LWUDP* udp = ttl->sea_udp;
    if (udp->ready == 0) {
        return;
    }
    const float app_time = (float)pLwc->app_time;
    const float ping_send_interval = lwcontext_update_interval(pLwc) * lwttl_ping_send_interval_multiplier(ttl);
    if (udp->last_updated == 0 || app_time > udp->last_updated + ping_send_interval) {
        lwttl_udp_send_ttlping(ttl, udp, udp->ping_seq);
        udp->ping_seq++;
        udp->last_updated = app_time;
    }

    FD_ZERO(&udp->readfds);
    FD_SET(udp->s, &udp->readfds);
    int rv = 0;
    while ((rv = select(udp->s + 1, &udp->readfds, NULL, NULL, &udp->tv)) == 1) {
        if ((udp->recv_len = recvfrom(udp->s, udp->buf, LW_UDP_BUFLEN, 0, (struct sockaddr*)&udp->si_other, (socklen_t*)&udp->slen)) == SOCKET_ERROR) {
#if LW_PLATFORM_WIN32
            int wsa_error_code = WSAGetLastError();
            if (wsa_error_code == WSAECONNRESET) {
                // UDP server not ready?
                // Go back to single play mode
                //udp->master = 1;
                return;
            } else {
                LOGEP("recvfrom() failed with error code : %d", wsa_error_code);
                exit(EXIT_FAILURE);
            }
#else
            // Socket recovery needed
            LOGEP("UDP socket error! Socket recovery needed...");
            udp->ready = 0;
            udp->reinit_next_update = 1;
            return;
#endif
        }

        char decompressed[1500 * 255]; // maximum lz4 compression ratio is 255...
        int decompressed_bytes = LZ4_decompress_safe(udp->buf, decompressed, udp->recv_len, ARRAY_SIZE(decompressed));
        if (decompressed_bytes > 0) {
            const int packet_type = *(int*)decompressed;
            switch (packet_type) {
            case LPGP_LWPTTLSEAAREA:
            {
                if (decompressed_bytes != sizeof(LWPTTLSEAAREA)) {
                    LOGE("LWPTTLSEAAREA: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLSEAAREA));
                }

                LWPTTLSEAAREA* p = (LWPTTLSEAAREA*)decompressed;
                LOGIx("LWPTTLSEAAREA: name=%s", p->name);
                lwttl_set_seaarea(ttl, p->name);
                break;
            }
            case LPGP_LWPTTLTRACKCOORDS:
            {
                if (decompressed_bytes != sizeof(LWPTTLTRACKCOORDS)) {
                    LOGE("LPGP_LWPTTLTRACKCOORDS: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLTRACKCOORDS));
                }

                LWPTTLTRACKCOORDS* p = (LWPTTLTRACKCOORDS*)decompressed;
                if (p->id) {
                    LOGIx("LWPTTLTRACKCOORDS: id=%d x=%f y=%f", p->id, p->x, p->y);
                    const float lng = cell_fx_to_lng(p->x);
                    const float lat = cell_fy_to_lat(p->y);
                    lwttl_set_center(ttl, lng, lat);
                } else {
                    lwttl_set_track_object_id(ttl, 0);
                    lwttl_set_track_object_ship_id(ttl, 0);
                }
                break;
            }
            case LPGP_LWPTTLROUTESTATE:
            {
                if (decompressed_bytes != sizeof(LWPTTLROUTESTATE)) {
                    LOGE("LWPTTLROUTESTATE: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLROUTESTATE));
                }

                LWPTTLROUTESTATE* p = (LWPTTLROUTESTATE*)decompressed;
                LOGIx("LWPTTLROUTESTATE: %d objects.", p->count);
                set_ttl_full_state(ttl, p);
                break;
            }
            case LPGP_LWPTTLSTATICSTATE2:
            {
                if (decompressed_bytes != sizeof(LWPTTLSTATICSTATE2)) {
                    LOGE("LWPTTLSTATICSTATE2: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLSTATICSTATE2));
                }

                LWPTTLSTATICSTATE2* p = (LWPTTLSTATICSTATE2*)decompressed;
                LOGIx("LWPTTLSTATICSTATE2: %d objects.", p->count);

                const int add_ret = add_to_object_cache_land(&ttl->object_cache.land_cache,
                                                             ttl->object_cache.land_array,
                                                             ARRAY_SIZE(ttl->object_cache.land_array),
                                                             &ttl->object_cache.land_count,
                                                             p);
                if (add_ret == 1) {
                    //send_ttlpingflush(ttl);
                }
                break;
            }
            case LPGP_LWPTTLSTATICSTATE3:
            {
                if (decompressed_bytes != sizeof(LWPTTLSTATICSTATE3)) {
                    LOGE("LWPTTLSTATICSTATE3: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLSTATICSTATE3));
                }

                LWPTTLSTATICSTATE3* p = (LWPTTLSTATICSTATE3*)decompressed;
                //add_to_object_cache_land(&ttl->object_cache.land_cache,
                //                         ttl->object_cache.land_array,
                //                         ARRAY_SIZE(ttl->object_cache.land_array),
                //                         &ttl->object_cache.land_count,
                //                         p);
                break;
            }
            case LPGP_LWPTTLSEAPORTSTATE:
            {
                if (decompressed_bytes != sizeof(LWPTTLSEAPORTSTATE)) {
                    LOGE("LWPTTLSEAPORTSTATE: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLSEAPORTSTATE));
                }

                LWPTTLSEAPORTSTATE* p = (LWPTTLSEAPORTSTATE*)decompressed;
                LOGIx("LWPTTLSEAPORTSTATE: %d objects.", p->count);

                //memcpy(&ttl->ttl_seaport_state, p, sizeof(LWPTTLSEAPORTSTATE));

                const int add_ret = add_to_object_cache_seaport(&ttl->object_cache.seaport_cache,
                                                                ttl->object_cache.seaport_array,
                                                                ARRAY_SIZE(ttl->object_cache.seaport_array),
                                                                &ttl->object_cache.seaport_count,
                                                                p);
                if (add_ret == 1) {
                    //send_ttlpingflush(ttl);
                }

                break;
            }
            case LPGP_LWPTTLCITYSTATE:
            {
                if (decompressed_bytes != sizeof(LWPTTLCITYSTATE)) {
                    LOGE("LWPTTLCITYSTATE: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLCITYSTATE));
                }

                LWPTTLCITYSTATE* p = (LWPTTLCITYSTATE*)decompressed;
                LOGIx("LWPTTLCITYSTATE: %d objects.", p->count);

                //memcpy(&ttl->ttl_seaport_state, p, sizeof(LWPTTLSEAPORTSTATE));

                const int add_ret = add_to_object_cache_city(&ttl->object_cache.city_cache,
                                                             ttl->object_cache.city_array,
                                                             ARRAY_SIZE(ttl->object_cache.city_array),
                                                             &ttl->object_cache.city_count,
                                                             p);
                if (add_ret == 1) {
                    //send_ttlpingflush(ttl);
                }

                break;
            }
            case LPGP_LWPTTLSALVAGESTATE:
            {
                if (decompressed_bytes != sizeof(LWPTTLSALVAGESTATE)) {
                    LOGE("LWPTTLSALVAGESTATE: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLSALVAGESTATE));
                }
                LWPTTLSALVAGESTATE* p = (LWPTTLSALVAGESTATE*)decompressed;
                LOGIx("LWPTTLSALVAGESTATE: %d objects.", p->count);
                const int add_ret = add_to_object_cache_salvage(&ttl->object_cache.salvage_cache,
                                                                ttl->object_cache.salvage_array,
                                                                ARRAY_SIZE(ttl->object_cache.salvage_array),
                                                                &ttl->object_cache.salvage_count,
                                                                p);
                break;
            }
            case LPGP_LWPTTLWAYPOINTS:
            {
                if (decompressed_bytes != sizeof(LWPTTLWAYPOINTS)) {
                    LOGE("LWPTTLWAYPOINTS: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLWAYPOINTS));
                }
                LWPTTLWAYPOINTS* p = (LWPTTLWAYPOINTS*)decompressed;
                LOGIx("LWPTTLWAYPOINTS: %d objects.", p->count);
                set_ttl_waypoints(ttl, p);
                break;
            }
            case LPGP_LWPTTLSINGLECELL:
            {
                if (decompressed_bytes != sizeof(LWPTTLSINGLECELL)) {
                    LOGE("LWPTTLSINGLECELL: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLWAYPOINTS));
                }
                LWPTTLSINGLECELL* p = (LWPTTLSINGLECELL*)decompressed;
                LOGIx("LWPTTLSINGLECELL: %d,%d L[%d], W[%d], SW[%d], PID[%d], PNAME[%s]",
                      p->xc0,
                      p->yc0,
                      (p->attr >> 0) & 1,
                      (p->attr >> 1) & 1,
                      (p->attr >> 2) & 1,
                      p->port_id,
                      p->port_name);
                memcpy(&ttl->ttl_single_cell, p, sizeof(LWPTTLSINGLECELL));
                assert((p->land_box_valid && p->water_box_valid) == 0);
                if (p->land_box_valid) {
                    ttl->cell_box[ttl->cell_box_count].xc0 = p->land_box[0];
                    ttl->cell_box[ttl->cell_box_count].yc0 = p->land_box[1];
                    ttl->cell_box[ttl->cell_box_count].xc1 = p->land_box[2];
                    ttl->cell_box[ttl->cell_box_count].yc1 = p->land_box[3];
                    ttl->cell_box_count++;
                } else if (p->water_box_valid) {
                    ttl->cell_box[ttl->cell_box_count].xc0 = p->water_box[0];
                    ttl->cell_box[ttl->cell_box_count].yc0 = p->water_box[1];
                    ttl->cell_box[ttl->cell_box_count].xc1 = p->water_box[2];
                    ttl->cell_box[ttl->cell_box_count].yc1 = p->water_box[3];
                    ttl->cell_box_count++;
                }
                break;
            }
            case LPGP_LWPTTLGOLDEARNED:
            {
                if (decompressed_bytes != sizeof(LWPTTLGOLDEARNED)) {
                    LOGE("LWPTTLGOLDEARNED: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLGOLDEARNED));
                }
                LWPTTLGOLDEARNED* p = (LWPTTLGOLDEARNED*)decompressed;
                LOGIx("LWPTTLGOLDEARNED");
                char text[64];
                snprintf(text,
                         ARRAY_SIZE(text) - 1,
                         "%s%d",
                         LW_UTF8_TTL_CHAR_ICON_GOLD,
                         p->amount);
                text[ARRAY_SIZE(text) - 1] = 0;
                spawn_world_text(ttl, text, p->xc0, p->yc0);
                break;
            }
            case LPGP_LWPTTLCARGONOTIFICATION:
            {
                if (decompressed_bytes != sizeof(LWPTTLCARGONOTIFICATION)) {
                    LOGE("LWPTTLCARGONOTIFICATION: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLCARGONOTIFICATION));
                }
                LWPTTLCARGONOTIFICATION* p = (LWPTTLCARGONOTIFICATION*)decompressed;
                LOGIx("LWPTTLCARGONOTIFICATION");
                char text[64];
                snprintf(text,
                         ARRAY_SIZE(text) - 1,
                         "%s%d",
                         LW_UTF8_TTL_CHAR_ICON_CARGO,
                         p->amount);
                text[ARRAY_SIZE(text) - 1] = 0;
                LW_TTL_WORLD_TEXT_ANIM_TYPE anim_type = LTWTAT_STOP;
                switch (p->cargo_notification_type) {
                case LTCNT_CREATED:
                    anim_type = LTWTAT_MOVE;
                    break;
                case LTCNT_LOADED:
                    anim_type = LTWTAT_SCALE_1_TO_0;
                    break;
                case LTCNT_UNLOADED:
                    anim_type = LTWTAT_SCALE_0_TO_1;
                    break;
                case LTCNT_CONSUMED:
                    anim_type = LTWTAT_SCALE_1_TO_0;
                    break;
                case LTCNT_CONVERTED:
                    anim_type = LTWTAT_SCALE_0_TO_1_TO_0;
                    break;
                }
                spawn_world_text_move(ttl, text, p->xc0, p->yc0, p->xc1, p->yc1, anim_type);
                break;
            }
            case LPGP_LWPTTLSTAT:
            {
                if (decompressed_bytes != sizeof(LWPTTLSTAT)) {
                    LOGE("LWPTTLSTAT: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLSTAT));
                }
                LWPTTLSTAT* p = (LWPTTLSTAT*)decompressed;
                LOGIx("LWPTTLSTAT");
                ttl->gold = p->gold;
                ttl->ports = p->ports;
                ttl->ships = p->ships;
                break;
            }
            case LPGP_LWPTTLGOLDUSED:
            {
                if (decompressed_bytes != sizeof(LWPTTLGOLDUSED)) {
                    LOGE("LWPTTLGOLDUSED: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLGOLDUSED));
                }
                LWPTTLGOLDUSED* p = (LWPTTLGOLDUSED*)decompressed;
                LOGIx("LWPTTLGOLDUSED");
                char text[64];
                snprintf(text,
                         ARRAY_SIZE(text) - 1,
                         "%s%d",
                         LW_UTF8_TTL_CHAR_ICON_GOLD,
                         -p->amount);
                text[ARRAY_SIZE(text) - 1] = 0;
                spawn_world_text(ttl, text, p->xc0, p->yc0);
                break;
            }
            case LPGP_LWPTTLCHAT:
            {
                if (decompressed_bytes != sizeof(LWPTTLCHAT)) {
                    LOGE("LWPTTLCHAT: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLCHAT));
                }
                LWPTTLCHAT* p = (LWPTTLCHAT*)decompressed;
                LOGIx("LWPTTLCHAT");
                lwchatringbuffer_add(&pLwc->chat_ring_buffer, p->line);
                break;
            }
            default:
            {
                LOGEP("Unknown UDP packet");
                break;
            }
            }
        } else {
            LOGEP("lz4 decompression failed!");
        }
    }
}

void lwttl_update(LWTTL* ttl, LWCONTEXT* pLwc, float delta_time) {
    if (ttl == 0) {
        return;
    }
    const float app_time = (float)pLwc->app_time;
    for (int i = 0; i < ttl->ttl_dynamic_state.count; i++) {
        int route_dir = ttl->ttl_dynamic_state.obj[i].route_flags.reversed ? (-1) : (+1);
        if (ttl->ttl_dynamic_state.obj[i].route_flags.sailing) {
            ttl->ttl_dynamic_state.obj[i].route_param += (float)delta_time * ttl->ttl_dynamic_state.obj[i].route_speed * route_dir;
        }
    }

    if (ttl->sea_udp) {
        lwttl_udp_update(ttl, pLwc);
    }

    LWTTLSELECTED* selected = &ttl->viewports[0].selected;
    float dx = 0, dy = 0, dlen = 0;
    if ((lw_pinch() == 0)
        && (selected->dragging == 0)
        && lw_get_normalized_dir_pad_input(pLwc, &pLwc->left_dir_pad, &dx, &dy, &dlen)
        && (dx || dy)
        && (dlen > 0.05f)) {
        // dx, dy in world space coordinates
        vec2 dworld;
        lwttl_screen_to_world_pos(ttl, dx, dy, dworld);

        const float dworld_len = sqrtf(dworld[0] * dworld[0] + dworld[1] * dworld[1]);
        dworld[0] /= dworld_len;
        dworld[1] /= dworld_len;

        // cancel tracking if user want to scroll around
        lwttl_set_track_object_ship_id(ttl, 0);
        const int view_scale = lwttl_view_scale(ttl);
        // direction inverted
        lwttl_worldmap_scroll_to(ttl,
                                 ttl->viewports[0].view_center.lng + (-dworld[0]) / 50.0f * delta_time * view_scale,
                                 ttl->viewports[0].view_center.lat + (-dworld[1]) / 50.0f * delta_time * view_scale,
                                 0);
        // prevent unintentional change of cell selection
        // while spanning the map
        lwttl_clear_selected_pressed_pos(ttl);
        // send ping persistently while map panning
        ttl->panning = 1;
    } else {
        ttl->panning = 0;
    }

    if (selected->press_pos_xc >= 0 && selected->press_pos_yc >= 0) {
        if (selected->pressing) {
            if (app_time > selected->press_at + selected->press_menu_gauge_appear_delay
                && (selected->press_pos_xc != selected->pos_xc || selected->press_pos_yc != selected->pos_yc)) {
                // change selection after 'press_menu_gauge_appear_delay'
                // even if touch is not released
                LWTTLLNGLAT lnglat;
                lnglat.lng = cell_x_to_lng(selected->press_pos_xc);
                lnglat.lat = cell_y_to_lat(selected->press_pos_yc);
                lwttl_change_selected_cell_to(ttl,
                                              selected->press_pos_xc,
                                              selected->press_pos_yc,
                                              &lnglat);
            } else if (selected->dragging == 0
                       && app_time > selected->press_at + selected->press_menu_gauge_total) {
                // change to selection-dragging mode
                selected->dragging = 1;
                selected->dragging_pos_xc = selected->pos_xc;
                selected->dragging_pos_yc = selected->pos_yc;
            }
        }
    }
    update_world_text(ttl, delta_time);
    selected->selected_cell_height = LWCLAMP(selected->selected_cell_height + delta_time * selected->selected_cell_height_speed,
                                             -selected->selected_cell_max_height,
                                             +selected->selected_cell_max_height);
}

void lwttl_toggle_cell_grid(LWTTL* ttl) {
    ttl->viewports[0].cell_grid = !ttl->viewports[0].cell_grid;
}

int lwttl_cell_grid(const LWTTL* ttl) {
    return ttl->viewports[0].cell_grid;
}

int lwttl_gold(const LWTTL* ttl) {
    return ttl->gold;
}

int lwttl_ports(const LWTTL* ttl) {
    return ttl->ports;
}

int lwttl_ships(const LWTTL* ttl) {
    return ttl->ships;
}

int lwttl_is_selected_cell(const LWTTL* ttl, int x0, int y0) {
    const LWTTLSELECTED* selected = &ttl->viewports[0].selected;
    return selected->selected
        && selected->pos_xc == x0
        && selected->pos_yc == y0;
}

int lwttl_is_selected_cell_intersect(const LWTTL* ttl, int x0, int y0) {
    const LWTTLSELECTED* selected = &ttl->viewports[0].selected;
    return selected->selected
        && (selected->pos_xc - x0 == 0 || selected->pos_xc - x0 == -1)
        && (selected->pos_yc - y0 == 0 || selected->pos_yc - y0 == -1);
}

int lwttl_is_selected_cell_diff(const LWTTL* ttl, int x0, int y0, int* dx0, int* dy0) {
    const LWTTLSELECTED* selected = &ttl->viewports[0].selected;
    *dx0 = selected->pos_xc - x0;
    *dy0 = selected->pos_yc - y0;
    return selected->selected;
}

float lwttl_selected_cell_popup_height(const LWTTL* ttl, const LWTTLFIELDVIEWPORT* vp) {
    return vp->selected.selected_cell_height;
}

const char* lwttl_route_state(const LWPTTLROUTEOBJECT* obj) {
    if (obj->route_flags.breakdown) {
        return "BREAKDOWN";
    } else if (obj->route_flags.loading) {
        return "LOADING";
    } else if (obj->route_flags.unloading) {
        return "UNLOADING";
    } else {
        return "";
    }
}

void lwttl_cam_eye(const LWTTL* ttl, vec3 cam_eye) {
    memcpy(cam_eye, ttl->viewports[0].cam_eye, sizeof(vec3));
}

void lwttl_set_cam_eye(LWTTL* ttl, const vec3 cam_eye) {
    memcpy(ttl->viewports[0].cam_eye, cam_eye, sizeof(vec3));
}

void lwttl_cam_look_at(const LWTTL* ttl, vec3 cam_look_at) {
    memcpy(cam_look_at, ttl->viewports[0].cam_look_at, sizeof(vec3));
}

void lwttl_set_cam_look_at(LWTTL* ttl, const vec3 cam_look_at) {
    memcpy(ttl->viewports[0].cam_look_at, cam_look_at, sizeof(vec3));
}

float cell_fx_to_lng(float fx) {
    return -180.0f + fx / LNGLAT_RES_WIDTH * 360.0f;
}

float cell_fy_to_lat(float fy) {
    return 90.0f - fy / LNGLAT_RES_HEIGHT * 180.0f;
}

float cell_x_to_lng(int x) {
    return cell_fx_to_lng((float)x);
}

float cell_y_to_lat(int y) {
    return cell_fy_to_lat((float)y);
}

float lng_to_render_coords(float lng, const LWTTLFIELDVIEWPORT* vp) {
    return (lng - vp->view_center.lng) * LNGLAT_SEA_CELL_UNIT_PER_DEGREES / vp->view_scale;
}

float lat_to_render_coords(float lat, const LWTTLFIELDVIEWPORT* vp) {
    return (lat - vp->view_center.lat) * LNGLAT_SEA_CELL_UNIT_PER_DEGREES / vp->view_scale;
}

float cell_x_to_render_coords(int x, const LWTTLFIELDVIEWPORT* vp) {
    return lng_to_render_coords(cell_x_to_lng(x), vp);
}

float cell_y_to_render_coords(int y, const LWTTLFIELDVIEWPORT* vp) {
    return lat_to_render_coords(cell_y_to_lat(y), vp);
}

float lwttl_vehicle_render_scale(const LWTTL* ttl, const LWTTLFIELDVIEWPORT* vp, float scale) {
    return scale / vp->view_scale;
}

float render_coords_to_lng(float rc, const LWTTLLNGLAT* center, int view_scale) {
    return rc * view_scale / LNGLAT_SEA_CELL_UNIT_PER_DEGREES + center->lng;
}

float render_coords_to_lat(float rc, const LWTTLLNGLAT* center, int view_scale) {
    return rc * view_scale / LNGLAT_SEA_CELL_UNIT_PER_DEGREES + center->lat;
}

float cell_fx_to_render_coords(float fx, const LWTTLLNGLAT* center, int view_scale) {
    return (cell_fx_to_lng(fx) - center->lng) * LNGLAT_SEA_CELL_UNIT_PER_DEGREES / view_scale;
}

float cell_fy_to_render_coords(float fy, const LWTTLLNGLAT* center, int view_scale) {
    return (cell_fy_to_lat(fy) - center->lat) * LNGLAT_SEA_CELL_UNIT_PER_DEGREES / view_scale;
}

float cell_fx_to_render_coords_vp(float fx, const LWTTLFIELDVIEWPORT* vp) {
    return (cell_fx_to_lng(fx) - vp->view_center.lng) * LNGLAT_SEA_CELL_UNIT_PER_DEGREES / vp->view_scale;
}

float cell_fy_to_render_coords_vp(float fy, const LWTTLFIELDVIEWPORT* vp) {
    return (cell_fy_to_lat(fy) - vp->view_center.lat) * LNGLAT_SEA_CELL_UNIT_PER_DEGREES / vp->view_scale;
}

int lwttl_add_field_viewport(LWTTL* ttl, const LWTTLFIELDVIEWPORT* vp) {
    for (int i = 0; i < ARRAY_SIZE(ttl->viewports); i++) {
        if (ttl->viewports[i].valid) {
            continue;
        }
        memcpy(&ttl->viewports[i], vp, sizeof(LWTTLFIELDVIEWPORT));
        ttl->viewports[i].valid = 1;
        return i;
    }
    LOGEP("Maximum viewports (count=%d) exceeded.", (int)ARRAY_SIZE(ttl->viewports));
    return -1;
}

void lwttl_remove_field_viewport(LWTTL* ttl, int viewport_index) {
    if (viewport_index < 0 || viewport_index >= ARRAY_SIZE(ttl->viewports)) {
        LOGEP("Viewport index %d is out of range.", viewport_index);
    } else {
        if (ttl->viewports[viewport_index].valid) {
            ttl->viewports[viewport_index].valid = 0;
        } else {
            LOGEP("Viewport data at index %d is already invalid.", viewport_index);
        }
    }
}

const LWTTLFIELDVIEWPORT* lwttl_viewport(const LWTTL* ttl, int viewport_index) {
    if (viewport_index < 0 || viewport_index >= ARRAY_SIZE(ttl->viewports)) {
        LOGEP("Viewport index %d is out of range.", viewport_index);
    } else {
        if (ttl->viewports[viewport_index].valid) {
            return &ttl->viewports[viewport_index];
        } else {
            //LOGEP("Viewport data at index %d is invalid.", viewport_index);
            return 0;
        }
    }
    return 0;
}

int lwttl_sizeof_viewport() {
    return (int)sizeof(LWTTLFIELDVIEWPORT);
}

LWTTLFIELDVIEWPORT* lwttl_copy_viewport_data(const LWTTL* ttl, int viewport_index, LWTTLFIELDVIEWPORT* vp_copy) {
    const LWTTLFIELDVIEWPORT* vp = lwttl_viewport(ttl, viewport_index);
    if (vp) {
        memcpy(vp_copy, vp, sizeof(LWTTLFIELDVIEWPORT));
        return vp_copy;
    }
    return 0;
}

int lwttl_viewport_max_count(const LWTTL* ttl) {
    return ARRAY_SIZE(ttl->viewports);
}

void lwttl_set_viewport_show(LWTTL* ttl, int viewport_index, int show) {
    if (viewport_index < 0 || viewport_index >= ARRAY_SIZE(ttl->viewports)) {
        LOGEP("Viewport index %d is out of range.", viewport_index);
    } else {
        if (ttl->viewports[viewport_index].valid) {
            ttl->viewports[viewport_index].show = show;
        } else {
            LOGEP("Viewport data at index %d is invalid.", viewport_index);
        }
    }
}

void lwttl_set_viewport_view_scale(LWTTL* ttl, int viewport_index, int view_scale) {
    if (viewport_index < 0 || viewport_index >= ARRAY_SIZE(ttl->viewports)) {
        LOGEP("Viewport index %d is out of range.", viewport_index);
    } else {
        if (ttl->viewports[viewport_index].valid) {
            lwttl_update_viewport_data(ttl,
                                       &ttl->viewports[viewport_index],
                                       ttl->viewports[viewport_index].field_viewport_x,
                                       ttl->viewports[viewport_index].field_viewport_y,
                                       ttl->viewports[viewport_index].field_viewport_width,
                                       ttl->viewports[viewport_index].field_viewport_height,
                                       view_scale,
                                       &ttl->viewports[viewport_index].view_center,
                                       lwttl_half_lng_extent_in_degrees(view_scale),
                                       lwttl_half_lat_extent_in_degrees(view_scale));
        } else {
            LOGEP("Viewport data at index %d is invalid.", viewport_index);
        }
    }
}

const vec4* lwttl_viewport_view(const LWTTLFIELDVIEWPORT* vp) {
    return vp->view;
}

const vec4* lwttl_viewport_proj(const LWTTLFIELDVIEWPORT* vp) {
    return vp->proj;
}

const vec4* lwttl_viewport_ui_proj(const LWTTLFIELDVIEWPORT* vp) {
    return vp->ui_proj;
}

int lwttl_viewport_clamped_view_scale(const LWTTLFIELDVIEWPORT* vp) {
    return vp->clamped_view_scale;
}

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
                                    float* cell_h) {
    *x0 = 0;
    *y0 = 0;
    *cell_x0 = 0;
    *cell_y0 = 0;
    *cell_z0 = 0;
    *cell_w = 1;
    *cell_h = 1;
    const int x_scaled_offset_0 = bx - LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS / 2;
    const int y_scaled_offset_0 = by - LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS / 2;
    const int x_scaled_offset_1 = x_scaled_offset_0 + 1;
    const int y_scaled_offset_1 = y_scaled_offset_0 + 1;
    *x0 = (float)(bound_xc0 + vp->clamped_view_scale * x_scaled_offset_0);
    if (*x0 < vp->cell_bound_xc0 || *x0 >= vp->cell_bound_xc1) {
        // out of view range
        return -1;
    }
    *y0 = (float)(bound_yc0 + vp->clamped_view_scale * y_scaled_offset_0);
    if (*y0 < vp->cell_bound_yc0 || *y0 >= vp->cell_bound_yc1) {
        // out of view range
        return -2;
    }
    const float x1 = (float)(bound_xc0 + vp->clamped_view_scale * x_scaled_offset_1);
    const float y1 = (float)(bound_yc0 + vp->clamped_view_scale * y_scaled_offset_1);

    const float lng0_not_clamped = cell_fx_to_lng(*x0 - 0.5f * vp->clamped_view_scale);
    const float lat0_not_clamped = cell_fy_to_lat(*y0 - 0.5f * vp->clamped_view_scale);
    const float lng1_not_clamped = cell_fx_to_lng(x1 - 0.5f * vp->clamped_view_scale);
    const float lat1_not_clamped = cell_fy_to_lat(y1 - 0.5f * vp->clamped_view_scale);

    *cell_x0 = lng_to_render_coords(lng0_not_clamped, vp);
    *cell_y0 = lat_to_render_coords(lat0_not_clamped, vp);
    *cell_z0 = 0;// lwttl_is_selected_cell_intersect(ttl, (int)x0, (int)y0) ? 1.0f : 0.0f;
    const float cell_x1 = lng_to_render_coords(lng1_not_clamped, vp);
    const float cell_y1 = lat_to_render_coords(lat1_not_clamped, vp);
    *cell_w = cell_x1 - *cell_x0;
    // cell_y0 and cell_y1 are in OpenGL rendering coordinates (always cell_y0 > cell_y1)
    *cell_h = *cell_y0 - cell_y1;
    return 0;
}

float lwttl_viewport_aspect_ratio(const LWTTLFIELDVIEWPORT* vp) {
    return vp->field_viewport_aspect_ratio;
}

int lwttl_viewport_view_scale(const LWTTLFIELDVIEWPORT* vp) {
    return vp->view_scale;
}

float lwttl_viewport_waypoint_line_segment_thickness(const LWTTLFIELDVIEWPORT* vp) {
    return 0.1f / sqrtf((float)(vp->view_scale_msb + 1));
}

static float lwttl_viewport_icon_size_ratio(const LWTTLFIELDVIEWPORT* vp) {
    return 2.0f / sqrtf((float)(vp->clamped_view_scale_msb + 1));
}

float lwttl_viewport_icon_width(const LWTTLFIELDVIEWPORT* vp) {
    const float size_ratio = lwttl_viewport_icon_size_ratio(vp);
    return vp->cell_render_width * vp->clamped_view_scale * size_ratio;
}

float lwttl_viewport_icon_height(const LWTTLFIELDVIEWPORT* vp) {
    const float size_ratio = lwttl_viewport_icon_size_ratio(vp);
    return vp->cell_render_height * vp->clamped_view_scale * size_ratio;
}

int lwttl_viewport_icon_render_info(const LWTTL* ttl,
                                    const LWTTLFIELDVIEWPORT* vp,
                                    const int xc0,
                                    const int x_scaled_offset_0,
                                    const int yc0,
                                    const int y_scaled_offset_0,
                                    float* cell_x0,
                                    float* cell_y0,
                                    float* cell_z0) {
    *cell_x0 = 0;
    *cell_y0 = 0;
    *cell_z0 = 0;
    const float x0 = (float)(xc0 + vp->clamped_view_scale * x_scaled_offset_0);
    if (x0 < vp->cell_bound_xc0 || x0 >= vp->cell_bound_xc1) {
        // out of view range
        return -1;
    }
    const float y0 = (float)(yc0 + vp->clamped_view_scale * y_scaled_offset_0);
    if (y0 < vp->cell_bound_yc0 || y0 >= vp->cell_bound_yc1) {
        // out of view range
        return -2;
    }
    const float lng0_not_clamped = cell_fx_to_lng(x0 + 0.5f);
    const float lat0_not_clamped = cell_fy_to_lat(y0 + 0.5f);
    *cell_x0 = lng_to_render_coords(lng0_not_clamped, vp);
    *cell_y0 = lat_to_render_coords(lat0_not_clamped, vp);
    *cell_z0 = lwttl_is_selected_cell(ttl, (int)x0, (int)y0) ? lwttl_selected_cell_popup_height(ttl, vp) : 0.0f;
    return 0;
}

float lwttl_viewport_cell_render_width(const LWTTLFIELDVIEWPORT* vp) {
    return vp->cell_render_width;
}

float lwttl_viewport_cell_render_height(const LWTTLFIELDVIEWPORT* vp) {
    return vp->cell_render_height;
}

float lwttl_viewport_rt_x(const LWTTLFIELDVIEWPORT* vp) {
    return vp->field_viewport_rt_x;
}

float lwttl_viewport_rt_y(const LWTTLFIELDVIEWPORT* vp) {
    return vp->field_viewport_rt_y;
}

void lwttl_viewport_range(const LWTTLFIELDVIEWPORT* vp,
                          int* viewport_x,
                          int* viewport_y,
                          int* viewport_width,
                          int* viewport_height) {
    *viewport_x = vp->field_viewport_x;
    *viewport_y = vp->field_viewport_y;
    *viewport_width = vp->field_viewport_width;
    *viewport_height = vp->field_viewport_height;
}

void lwttl_set_window_size(LWTTL* ttl, int w, int h, float aspect_ratio) {
    lwttl_update_viewport_data(ttl,
                               &ttl->viewports[0],
                               ttl->viewports[0].field_viewport_x,
                               ttl->viewports[0].field_viewport_y,
                               w,
                               h,
                               ttl->viewports[0].view_scale,
                               &ttl->viewports[0].view_center,
                               ttl->viewports[0].half_lng_extent_in_deg,
                               ttl->viewports[0].half_lat_extent_in_deg);
    lwttl_update_viewport_data(ttl,
                               &ttl->viewports[1],
                               (int)((float)w / 2.0f - ((float)w / 1.5f) / 2.0f),
                               (int)((float)h - h / 3.75f),
                               (int)((float)w / 1.5f),
                               (int)((float)h / 4.0f),
                               ttl->viewports[1].view_scale,
                               &ttl->viewports[1].view_center,
                               ttl->viewports[1].half_lng_extent_in_deg,
                               ttl->viewports[1].half_lat_extent_in_deg);
}

int lwttl_viewport_render_flags(const LWTTLFIELDVIEWPORT* vp) {
    return vp->render_flags;
}

const LWTTLLNGLAT* lwttl_viewport_view_center(const LWTTLFIELDVIEWPORT* vp) {
    return &vp->view_center;
}

void lwttl_degrees_to_dms(int* d, int* m, float* s, const float degrees) {
    *d = (int)degrees;
    const float minutes = (degrees - *d) * 60;
    *m = (int)minutes;
    *s = (minutes - *m) * 60;
}

int lwttl_cell_box_count(const LWTTL* ttl) {
    return ttl->cell_box_count;
}

void lwttl_cell_box(const LWTTL* ttl, int index, int* xc0, int* yc0, int* xc1, int* yc1) {
    *xc0 = ttl->cell_box[index].xc0;
    *yc0 = ttl->cell_box[index].yc0;
    *xc1 = ttl->cell_box[index].xc1;
    *yc1 = ttl->cell_box[index].yc1;
}
