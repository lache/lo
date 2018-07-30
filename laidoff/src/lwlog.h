#pragma once

#include "platform_detection.h"

static inline void lw_null_printf(const char* x, ...) {}
// Use this instead of LOGI if you want to suppress logging temporarily
#define LOGIx(...) (void)lw_null_printf(__VA_ARGS__);
#define LOGIPx(...) (void)lw_null_printf(__VA_ARGS__);

#if LW_PLATFORM_WIN32
#include <stdio.h>
#include <windows.h>
#define LOGV(...) //((void)printf(__VA_ARGS__))
#define LOGD(...) lwlog_lock();(void)printf(__VA_ARGS__);(void)printf("\n");lwlog_unlock()
#define LOGI(...) lwlog_lock();(void)printf(__VA_ARGS__);(void)printf("\n");lwlog_unlock()
#define LOGW(...) lwlog_lock();(void)printf(__VA_ARGS__);(void)printf("\n");lwlog_unlock()
#define LOGE(...) { \
lwlog_lock(); \
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); \
CONSOLE_SCREEN_BUFFER_INFO consoleInfo; \
WORD saved_attributes; \
GetConsoleScreenBufferInfo(hConsole, &consoleInfo); \
saved_attributes = consoleInfo.wAttributes; \
SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY); \
(void)printf(__VA_ARGS__); \
(void)printf("\n"); \
SetConsoleTextAttribute(hConsole, saved_attributes); \
lwlog_unlock(); \
}
#define LOGF(...) LOGE(__VA_ARGS__);abort()
#define LOGA(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#elif LW_PLATFORM_IOS || LW_PLATFORM_OSX
#include <stdio.h>
#define LOGV(...) //((void)printf(__VA_ARGS__))
#define LOGD(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#define LOGI(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#define LOGW(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#define LOGE(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#define LOGF(...) (void)printf(__VA_ARGS__);(void)printf("\n");abort()
#define LOGA(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#elif LW_PLATFORM_IOS_SIMULATOR || LW_PLATFORM_RPI || LW_PLATFORM_LINUX
#include <stdio.h>
#define LOGV(...) //((void)printf(__VA_ARGS__))
#define LOGD(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#define LOGI(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#define LOGW(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#define LOGE(...) printf("\x1b[33m");(void)printf(__VA_ARGS__);(void)printf("\n");printf("\x1b[0m")
#define LOGF(...) (void)printf(__VA_ARGS__);(void)printf("\n");abort()
#define LOGA(...) (void)printf(__VA_ARGS__);(void)printf("\n")
#elif LW_PLATFORM_ANDROID
#include <android/log.h>
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, "native-activity", __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "native-activity", __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "native-activity", __VA_ARGS__))
#define LOGF(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "native-activity", __VA_ARGS__));abort()
#define LOGA(...) ((void)__android_log_print(ANDROID_LOG_ASSERT, "native-activity", __VA_ARGS__))
#endif

#define LW_STRINGIFY_(x) #x
#define LW_STRINGIFY(x) LW_STRINGIFY_(x)
#define __LW_STRING_LINE__ LW_STRINGIFY(__LINE__)
#define LWLOGPOS __FILE__ "(" __LW_STRING_LINE__ "): "

#define LOGVP(...) LOGV(LWLOGPOS __VA_ARGS__)
#define LOGDP(...) LOGD(LWLOGPOS __VA_ARGS__)
#define LOGIP(...) LOGI(LWLOGPOS __VA_ARGS__)
#define LOGWP(...) LOGW(LWLOGPOS __VA_ARGS__)
#define LOGEP(...) LOGE(LWLOGPOS __VA_ARGS__)
#define LOGFP(...) LOGF(LWLOGPOS __VA_ARGS__)
#define LOGAP(...) LOGA(LWLOGPOS __VA_ARGS__)

#ifdef __cplusplus
extern "C" {;
#endif
void lwlog_init();
void lwlog_destroy();
void lwlog_lock();
void lwlog_unlock();
#ifdef __cplusplus
}
#endif
