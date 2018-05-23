#pragma once

#if defined __cplusplus
extern "C" {;
#endif  /* defined __cplusplus */

#define MAX_PATHQUERY_SMOOTH (2048)

typedef struct _LWPATHQUERY LWPATHQUERY;
typedef struct _LWFIELD LWFIELD;
typedef struct _LWNAV LWNAV;

void* load_nav(const char* filename);
void unload_nav(LWNAV* nav);
void nav_query(LWNAV* nav, LWPATHQUERY* pq);
void set_random_start_end_pos(LWNAV* nav, LWPATHQUERY* pq);
void start_new_path_query_test(LWNAV* nav, LWPATHQUERY* pq);
void start_new_path_query_continue_test(LWNAV* nav, LWPATHQUERY* pq);
int nav_update_path_query(LWNAV* nav, LWPATHQUERY* pq, float move_speed, float delta_time, float* out_pos, float* out_rot);
void pathquery_set_start_to_end(LWPATHQUERY* pq);
int nav_new_path_query(LWNAV* nav);
void nav_clear_all_path_queries(LWNAV* nav);
int nav_update_output_path_query(LWNAV* nav, int idx, int val);
int nav_bind_path_query_output_location(LWNAV* nav, int idx, LWFIELD* field, int field_object_idx);
void* nav_path_query_test(LWNAV* nav);
void* nav_path_query(LWNAV* nav, int idx);
void nav_update(LWNAV* nav, float move_speed, float delta_time);
void nav_path_query_spos(const LWNAV* nav, float* p);
void nav_path_query_epos(const LWNAV* nav, float* p);
void nav_set_path_query_spos(LWNAV* nav, float x, float y, float z);
void nav_set_path_query_epos(LWNAV* nav, float x, float y, float z);
int nav_path_query_n_smooth_path(const LWNAV* nav);
void nav_reset_deterministic_seed(LWNAV* nav);
void reset_nav_context(LWNAV* nav);
#if defined __cplusplus
}
#endif  /* defined __cplusplus */
