#pragma once
#ifdef __cplusplus
extern "C" {;
#endif
void* remtex_new(const char* host);
int remtex_preload(void* r, const char* name);
GLuint remtex_load_tex(void* r, const char* name);
void remtex_destroy_render(void* r);
void remtex_destroy(void** r);
void remtex_update(void* r, double delta_time);
void remtex_render(void* r, void* htmlui);
void remtex_loading_str(void* r, char* str, size_t max_len);
unsigned long remtex_name_hash_from_url(const char* url, int* valid, char* name, int name_max_len);
int remtex_gpu_loaded(void* r, int id);
int remtex_width_height(void* r, int id, int* w, int* h);
#ifdef __cplusplus
}
#endif
