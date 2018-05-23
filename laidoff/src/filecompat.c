#include "platform_detection.h"
#include <stdio.h>
#include "lwlog.h"
#include "lwuniqueid.h"
#include "lwmacro.h"
#include <string.h>
#include <stdlib.h>
#include "file.h"
// needed for 'mkdir()' call
#if !LW_PLATFORM_WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#else
#include <io.h>
#define F_OK (0)
#endif

void concat_path(char* path, const char* path1, const char* path2) {
	if (path1) {
		strcat(path, path1);
	}
	if (strlen(path) > 0) {
		if (strncmp(&path[strlen(path) - 1], PATH_SEPARATOR, 1) != 0) {
			strcat(path, PATH_SEPARATOR);
		}
	}
	strcat(path, path2);
}

int get_cached_user_id(const char* path_prefix, LWUNIQUEID* id) {
	FILE* f;
	char path[1024] = { 0, };
	concat_path(path, path_prefix, LW_USER_ID_CACHE_FILE);
    LOGI("Open cached user id file: %s", path);
	f = fopen(path, "rb");
	if (f == 0) {
		// no cached user id exists
		LOGI("No cached user id exists");
		return -1;
	}
	fseek(f, 0, SEEK_END);
	const int f_size = (int)ftell(f);
	fseek(f, 0, SEEK_SET);
	const int expected_size = sizeof(unsigned int) * 4;
	if (f_size != expected_size) {
		// no cached user id exists
		LOGI("Invalid cached user id size: %d (%d expected)", f_size, expected_size);
		return -2;
	}
	fread((void*)id, expected_size, 1, f);
	fclose(f);
	return 0;
}

int save_cached_user_id(const char* path_prefix, const LWUNIQUEID* id) {
#if !LW_PLATFORM_WIN32
    struct stat st = {0};
    if (stat(path_prefix, &st) == -1) {
        mkdir(path_prefix, 0700);
    }
#endif
	FILE* f;
	char path[1024] = { 0, };
	concat_path(path, path_prefix, LW_USER_ID_CACHE_FILE);
	f = fopen(path, "wb");
	if (f == 0) {
		// no cached user id exists
        LOGEP("CRITICAL ERROR: Cannot open user id file cache for writing...");
        exit(-99);
	}
	const int expected_size = sizeof(unsigned int) * 4;
	size_t elements_written = fwrite((const void*)id, expected_size, 1, f);
	if (elements_written != 1) {
		// no cached user id exists
		LOGEP("fwrite bytes written failed!");
		return -2;
	}
	fclose(f);
	return 0;
}

int is_file_exist(const char* path_prefix, const char* filename) {
    char path[1024] = { 0, };
    concat_path(path, path_prefix, filename);
    return access(path, F_OK) != -1;
}

void touch_file(const char* path_prefix, const char* filename) {
    char path[1024] = { 0, };
    concat_path(path, path_prefix, filename);
    FILE* f = fopen(path, "wb");
    if (f == 0) {
        // no cached user id exists
        LOGEP("CRITICAL ERROR: Cannot open file '%s' for writing...", path);
        exit(-99);
    }
    fclose(f);
    LOGIP("File '%s' touched.", path);
}

void write_file_string(const char* path_prefix, const char* filename, const char* str) {
    write_file(path_prefix, filename, str, -1/*not used*/, "w");
}

void write_file_binary(const char* path_prefix, const char* filename, const char* dat, size_t dat_len) {
    write_file(path_prefix, filename, dat, dat_len, "wb");
}

void write_file(const char* path_prefix, const char* filename, const char* dat, size_t dat_len, const char* mode) {
    char path[1024] = { 0, };
    concat_path(path, path_prefix, filename);
    FILE* f = fopen(path, mode);
    if (f == 0) {
        // no cached user id exists
        LOGEP("CRITICAL ERROR: Cannot open file '%s' for writing...", path);
        exit(-99);
    }
    if (strcmp(mode, "w") == 0) {
        fprintf(f, "%s", dat);
    } else {
        fwrite(dat, dat_len, 1, f);
    }
    fclose(f);
    LOGIP("File '%s' written.", path);
}

int read_file_string(const char* path_prefix, const char* filename, size_t str_out_len, char* str_out) {
    return read_file(path_prefix, filename, str_out_len, str_out, "r");
}

int read_file_binary(const char* path_prefix, const char* filename, size_t str_out_len, char* str_out) {
    return read_file(path_prefix, filename, str_out_len, str_out, "rb");
}

int read_file(const char* path_prefix, const char* filename, size_t dat_out_len, char* dat_out, const char* mode) {
    char path[1024] = { 0, };
    concat_path(path, path_prefix, filename);
    FILE* f = fopen(path, mode);
    if (f == 0) {
        // no cached user id exists
        LOGEP("CRITICAL ERROR: Cannot open file '%s' for reading...", path);
        return -1;
    }
    if (strcmp(mode, "r") == 0) {
        char format[16];
        snprintf(format, sizeof(format), "%%%zus", dat_out_len - 1);
        int fscanf_result = fscanf(f, format, dat_out);
        if (fscanf_result != 1) {
            LOGEP("Cannot read all string of '%s'. fscanf returned %d",
                  path,
                  fscanf_result);
            return -2;
        }
    } else {
        fseek(f, 0, SEEK_END);
        long total_size = ftell(f);
        if (total_size <= 0) {
            LOGEP("Total size of '%s' is %d!",
                  path,
                  total_size);
            return -3;
        }
        fseek(f, 0, SEEK_SET);
        size_t read_size = LWMIN((size_t)total_size, dat_out_len);
        fread(dat_out, 1, read_size, f);
        if (read_size != total_size) {
            LOGEP("Cannot read all data of '%s'. Buffer size = %zu, Total size = %d",
                  path,
                  dat_out_len,
                  total_size);
            return -4;
        }
    }
    fclose(f);
    LOGIP("File '%s' read.", path);
    return 0;
}
