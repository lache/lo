#pragma once

typedef struct _LWCONTEXT LWCONTEXT;

typedef struct _LWDIRPAD {
    // 1 if dir pad is dragged, 0 if otherwise
    int dragging;
    // dir_pad_dragging pointer index
    int pointer_id;
    // Current dir pad x coordinate (screen coordinate)
    float x;
    // Current dir pad y coordinate (screen coordinate)
    float y;
    // Touch start x coordinate (screen coordinate) -- could be FOLLOWED while dragging
    float start_x;
    // Touch start y coordinate (screen coordinate) -- could be FOLLOWED while dragging
    float start_y;
    // Touch start x coordinate (screen coordinate) -- DETERMINED at touch began, CONSTANT while dragging
    float touch_began_x;
    // Touch start y coordinate (screen coordinate) -- DETERMINED at touch began, CONSTANT while dragging
    float touch_began_y;
    // Original center x coordinate (screen coordinate) -- CONSTANT during runtime
    float origin_x;
    // Original center y coordinate (screen coordinate) -- CONSTANT during runtime
    float origin_y;
    // Max distance between '(x,y)' and '(start_x, start_y)' -- CONSTANT during runtime
    float max_follow_distance;
    // Max distance between '(start_x, start_y)' and '(touch_began_x, touch_began_y)' -- CONSTANT during runtime
    float max_began_distance;
} LWDIRPAD;

void dir_pad_init(LWDIRPAD* dir_pad,
                  float origin_x,
                  float origin_y,
                  float max_follow_distance,
                  float max_began_distance);
void reset_dir_pad_position(LWDIRPAD* dir_pad);
int lw_get_normalized_dir_pad_input(const LWCONTEXT* pLwc, const LWDIRPAD* dir_pad, float *dx, float *dy, float *dlen);
void get_right_dir_pad_original_center(const float aspect_ratio, float *x, float *y);
void get_left_dir_pad_original_center(const float aspect_ratio, float *x, float *y);
float get_dir_pad_size_radius();
int dir_pad_press(LWDIRPAD* dir_pad, float x, float y, int pointer_id,
                  float dir_pad_center_x, float dir_pad_center_y, float sr);
void dir_pad_move(LWDIRPAD* dir_pad, float x, float y, int pointer_id,
                  float dir_pad_center_x, float dir_pad_center_y, float sr);
int dir_pad_release(LWDIRPAD* dir_pad, int pointer_id);

void render_dir_pad(const LWCONTEXT* pLwc, float x, float y);
void render_dir_pad_with_start(const LWCONTEXT* pLwc, const LWDIRPAD* dir_pad);
void dir_pad_follow_start_position(LWDIRPAD* dir_pad);
void render_dir_pad_joystick_area(const LWCONTEXT* pLwc, float x, float y, float ui_alpha);
void render_dir_pad_joystick(const LWCONTEXT* pLwc, float x, float y, float ui_alpha);
void render_dir_pad_with_start_joystick(const LWCONTEXT* pLwc, const LWDIRPAD* dir_pad, float ui_alpha);