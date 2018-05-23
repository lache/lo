#pragma once

#include "platform_detection.h"

#if !LW_PLATFORM_ANDROID
char* load_software_decode_etc1_rgb(short extended_width, short extended_height, const char* d);
void release_software_decode_etc1_rgb(char* d);
#endif
