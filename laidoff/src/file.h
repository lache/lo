#pragma once

#ifdef __cplusplus
extern "C" {;
#endif

typedef struct _LWUNIQUEID LWUNIQUEID;

char * create_string_from_file(const char * filename);
void release_string(char * d);
char* create_binary_from_file(const char* filename, size_t* size);
void release_binary(char * d);
int get_cached_user_id(const char* path_prefix, LWUNIQUEID* id);
int save_cached_user_id(const char* path_prefix, const LWUNIQUEID* id);
void concat_path(char* path, const char* path1, const char* path2);
const char* package_version();
int is_file_exist(const char* path_prefix, const char* filename);
void touch_file(const char* path_prefix, const char* filename);
void write_file_string(const char* path_prefix, const char* filename, const char* str);
void write_file_binary(const char* path_prefix, const char* filename, const char* dat, size_t dat_len);
void write_file(const char* path_prefix, const char* filename, const char* dat, size_t dat_len, const char* mode);
int read_file_string(const char* path_prefix, const char* filename, size_t str_out_len, char* str_out);
int read_file_binary(const char* path_prefix, const char* filename, size_t str_out_len, char* str_out);
int read_file(const char* path_prefix, const char* filename, size_t dat_out_len, char* dat_out, const char* mode);
int read_file_string_all(const char* path_prefix, const char* filename, const char** str_out);
int read_file_binary_all(const char* path_prefix, const char* filename, size_t* dat_out_len, const char** dat_out);
#ifdef __cplusplus
};
#endif
