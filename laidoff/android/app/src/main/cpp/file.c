#include <stdio.h>
#include <stdlib.h>
#include <android/log.h>
#include <errno.h>
#include "file.h"
#include "laidoff.h"

#define  LOG_TAG    "And9"

#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

typedef struct _NativeAsset {
    unsigned long asset_hash;
    int start_offset;
    int length;
} NativeAsset;

static NativeAsset nativeAssetArray[512];
static int nativeAssetLength;
static char apkPath[2048];
static char filesPath[2048];
int lw_download_assets();

void set_apk_path(const char* apk_path) {
    strcpy(apkPath, apk_path);
}

void set_files_path(const char* files_path) {
    strcpy(filesPath, files_path);
}

void release_string(char* d) {
    free(d);
}

char* create_asset_file(const char* filename, size_t* size, int binary) {
    unsigned long asset_hash = hash((const unsigned char*) filename);

    for (int i = 0; i < nativeAssetLength; i++) {
        if (nativeAssetArray[i].asset_hash == asset_hash) {
            FILE* f = fopen(apkPath, binary ? "rb" : "r");

            char* p = malloc(nativeAssetArray[i].length + (binary ? 0 : 1));

            int fseekResult = fseek(f, nativeAssetArray[i].start_offset, SEEK_SET);

            int freadResult = fread(p, nativeAssetArray[i].length, 1, f);

            if (!binary) {
                p[nativeAssetArray[i].length] = '\0'; // null termination for string data needed
            }

            LOGI("create_asset_file: %s size: %d, fseekResult: %d, errno: %d, binary: %d, test: %s",
                 filename,
                 nativeAssetArray[i].length,
                 fseekResult,
                 errno,
                 binary,
                 p
            );

            fclose(f);
            *size = nativeAssetArray[i].length;
            return p;
        }
    }

    *size = 0;
    return 0;
}

const char* get_filename_only(const char* filename) {
    size_t l = strlen(filename);
    for (int i = l - 1; i >= 0; i--) {
        if (filename[i] == '/') {
            return &filename[i + 1];
        }
    }

    return filename;
}

char* create_asset_file_from_downloaded(const char* filename, size_t* size, int binary) {
    char download_path[1024] = {0,};
    strcat(download_path, filesPath);
    strcat(download_path, "/");
    strcat(download_path, get_filename_only(filename));

    FILE* f = fopen(download_path, binary ? "rb" : "r");
    if (f == 0) {
        LOGE("create_asset_file_from_downloaded fopen failed! %s", download_path);
        return 0;
    }
    int fseekResult = fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* p = malloc(*size + (binary ? 0 : 1));

    int freadResult = fread(p, *size, 1, f);

    if (!binary) {
        p[*size] = '\0'; // null termination for string data needed
    }

    LOGI("create_asset_file_from_downloaded: %s size: %d, fseekResult: %d, errno: %d, binary: %d, test: %s",
         filename,
         *size,
         fseekResult,
         errno,
         binary,
         p
    );

    fclose(f);
    return p;
}

char* create_binary_from_file(const char* filename, size_t* size) {
    LOGI("create_binary_from_file: %s", filename);
    if (lw_download_assets()) {
        return create_asset_file_from_downloaded(filename, size, 1);
    } else {
        return create_asset_file(filename, size, 1);
    }
}

char* create_string_from_file(const char* filename) {
    LOGI("create_string_from_file: %s", filename);
    size_t size;
    if (lw_download_assets()) {
        return create_asset_file_from_downloaded(filename, &size, 0);
    } else {
        // check if downloaded one (or runtime created one) exists for conf.json
        if (strstr(filename, "conf/conf.json")) {
            char* downloaded = create_asset_file_from_downloaded(filename, &size, 0);
            if (downloaded) {
                return downloaded;
            }
        }
        return create_asset_file(filename, &size, 0);
    }
}

void release_binary(char* d) {
    free(d);
}

void init_register_asset() {
    nativeAssetLength = 0;
}

void register_asset(const char* asset_path, int start_offset, int length) {
    if (nativeAssetLength >= ARRAY_SIZE(nativeAssetArray)) {
        LOGE("nativeAssetArray exceeded! asset %s cannot be registered.", asset_path);
        // cannot proceed!
        abort();
    }
    nativeAssetArray[nativeAssetLength].asset_hash = hash((unsigned char*) asset_path);
    nativeAssetArray[nativeAssetLength].start_offset = start_offset;
    nativeAssetArray[nativeAssetLength].length = length;

    LOGI("Asset #%d (hash=%lu) registered: %s, start_offset=%d, length=%d",
         nativeAssetLength, nativeAssetArray[nativeAssetLength].asset_hash, asset_path,
         start_offset, length);

    nativeAssetLength++;
}
