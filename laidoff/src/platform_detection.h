#pragma once

#ifdef WIN32
#	define LW_PLATFORM_WIN32 1
#elif __APPLE__
#	include <TargetConditionals.h>
#	if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
#		define LW_PLATFORM_IOS 1
#		define LW_PLATFORM_IOS_SIMULATOR 1
#	elif TARGET_OS_IPHONE
#		define LW_PLATFORM_IOS 1
#	else
#		define LW_PLATFORM_OSX 1
#	endif
#elif RPI
#	define LW_PLATFORM_RPI 1
#elif __ANDROID__
#	define LW_PLATFORM_ANDROID 1
#elif __linux__
#	define LW_PLATFORM_LINUX 1
#else
#	define LW_PLATFORM_ANDROID 1
#endif
