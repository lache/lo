#include <stdio.h>
#include <stdlib.h>

#include "file.h"
#include "lwlog.h"
#include "platform_detection.h"

#if LW_PLATFORM_OSX
char* get_assets_path(const char* filename);
#endif

char* create_string_from_file(const char* filename)
{
#if LW_PLATFORM_OSX
    char* filename_should_free = get_assets_path(filename);
    filename = filename_should_free;
#endif
	FILE* f;
	f = fopen(filename, "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        const int f_size = (int)ftell(f);
        fseek(f, 0, SEEK_SET);
        char* d = (char*)malloc(f_size + 1);
        const size_t last_byte = fread(d, 1, f_size, f);
        d[last_byte] = '\0';
        fclose(f);
        LOGI("create_string_from_file: %s (%d bytes) loaded to memory.", filename, (int)last_byte);
#if LW_PLATFORM_OSX
        free(filename_should_free);
#endif
        return d;
    }
    LOGE("create_string_from_file: %s [ERROR] FILE NOT FOUND.", filename);
#if LW_PLATFORM_OSX
    free(filename_should_free);
#endif
    return 0;
}

void release_string(char* d)
{
	free(d);
}

char* create_binary_from_file(const char* filename, size_t* size)
{
#if LW_PLATFORM_OSX
    char* filename_should_free = get_assets_path(filename);
    filename = filename_should_free;
#endif
	FILE* f;
	f = fopen(filename, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        const int f_size = (int)ftell(f);
        fseek(f, 0, SEEK_SET);
        char* d = (char*)malloc(f_size);
        fread(d, 1, f_size, f);
        *size = f_size;
        fclose(f);
        LOGI("create_binary_from_file: %s (%d bytes) loaded to memory.", filename, f_size);
#if LW_PLATFORM_OSX
        free(filename_should_free);
#endif
        return d;
    }
    LOGE("create_binary_from_file: %s [ERROR] FILE NOT FOUND.", filename);
#if LW_PLATFORM_OSX
    free(filename_should_free);
#endif
    return 0;
}

void release_binary(char* d)
{
	free(d);
}
