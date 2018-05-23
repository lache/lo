#pragma once

#include "platform_detection.h"

#if LW_PLATFORM_WIN32 || LW_PLATFORM_ANDROID
#define LWU(x) u8##x
#else
#define LWU(x) x
#endif

int u8_toucs(unsigned int* dest, int sz, const char *src, int srcsz);
