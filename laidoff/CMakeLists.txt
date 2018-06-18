cmake_minimum_required(VERSION 3.5)
project(client)
# CMake의 Visual Studio Solution Folders 기능 켬
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# GLFW CMake 의존성 추가

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)
add_subdirectory(tinycthread)
if (NOT SERVER_ONLY)
    add_subdirectory(glfw)
    add_subdirectory(litehtml)
else (NOT SERVER_ONLY)

endif (NOT SERVER_ONLY)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
endif()

include_directories(${glfw_INCLUDE_DIRS})
include_directories("glfw/deps")

# 컴파일 플래그 설정

if (MSVC)
    # disable warning C4996: 'The POSIX name for this item is deprecated. Instead, use the ISO C and C++ conformant name'.
    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4996")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996")
	# disable warning C4819: 'The file contains a character that cannot be represented in the current code page (949).'
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4819")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4819")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_CRT_NONSTDC_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /WX")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_NONSTDC_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /WX")
endif ()

# ZeroMQ CMake 의존성 추가

# ZeroMQ 라이브러리 중 테스트 프로젝트는 빌드하지 않는다.
set(ZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(zeromq-4.2.1)

# ZeroMQ 라이브러리 중 사용할 것만 제외하고는 빌드되지 않도록 한다.

if (APPLE OR UNIX)
    # ZeroMQ 프로젝트는 ZeroMQ 솔루션 디렉토리에 모은다.
    set_target_properties(libzmq libzmq-static PROPERTIES FOLDER "ZeroMQ")
    set_target_properties(libzmq-static PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
else ()
    # ZeroMQ 프로젝트는 ZeroMQ 솔루션 디렉토리에 모은다.
    set_target_properties(libzmq libzmq-static local_lat local_thr remote_lat remote_thr inproc_lat inproc_thr PROPERTIES FOLDER "ZeroMQ")
    set_target_properties(libzmq-static local_lat local_thr remote_lat remote_thr inproc_lat inproc_thr PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
endif ()

include_directories(zeromq-4.2.1/include)

# CZMQ CMake 의존성 추가

add_subdirectory(czmq)
include_directories(czmq/include)

# CZMQ 라이브러리 중 사용할 것만 제외하고는 빌드되지 않도록 한다.
set_target_properties(czmq czmq_selftest zmakecert zsp PROPERTIES FOLDER "ZeroMQ")
# CZMQ 프로젝트는 ZeroMQ 솔루션 디렉토리에 모은다.
set_target_properties(czmq_selftest zmakecert zsp PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)

# lz4

add_subdirectory(lz4/contrib/cmake_unofficial)

include_directories(czmq/include)


include_directories("src")

include_directories("lua-5.3.4")

include_directories("ode/include")
include_directories("ode/ou/include")
include_directories("ode/ode/src")
include_directories("recastnavigation/Detour/Include")
include_directories("zmq/include")
include_directories("imgui")
include_directories("litehtml/include")
include_directories("lz4/lib")

#include_directories(libpng-1.6.28)

#link_directories(libpng-1.6.28/projects/vstudio/Release)

include_directories(/opt/vc/include /opt/vc/include/interface/vcos/pthreads /opt/vc/include/interface/vmcs_host/linux /opt/vc/src/hello_pi/libs/ilclient /opt/vc/src/hello_pi/libs/vgfont)

# pthread deps on Linux server build
find_package(Threads)


include(CheckIncludeFile)
CHECK_INCLUDE_FILE(/opt/vc/include/bcm_host.h BCM_HOST_AVAILABLE)

if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif ()

add_definitions(-DdDOUBLE)
add_definitions(-DLITEHTML_UTF8)
add_definitions(-DBASE_PATH="${CMAKE_CURRENT_SOURCE_DIR}")

if (WIN32)
    set(ICON win32/laidoff.rc)
    set(SERVERICON win32/server/laidoff-server.rc)
    set(WINCODEC Windowscodecs) # Windows Imaging Codecs
    set(WINDSHOW Strmiids) # DirectShow
    set(WINMM Winmm) # PlaySound
    set(WINXAUDIO2 Xaudio2) # XAudio2
    set(WINOPENGL32 Opengl32)
    set(WINSOCK2 ws2_32)
    set(IMGUI
            imgui/imgui_impl_glfw_gl3.cpp
            imgui/imgui_impl_glfw_gl3.h
            imgui/imgui.cpp
            imgui/imgui.h
            imgui/imgui_demo.cpp
            imgui/imgui_draw.cpp
            imgui/imgui_internal.h
            imgui/imconfig.h
            imgui/stb_rect_pack.h
            imgui/stb_textedit.h
            imgui/stb_truetype.h
            )
    set(WIN32_DEPS
            win32/etc1.cpp
            win32/file.c
            win32/image.cpp
            win32/key.c
            win32/main.c
            win32/mouse.c
            win32/sound.cpp
            win32/gl3w.c
            win32/gl3w.h
            win32/glcorearb.h
            win32/main_vulkan.cpp
            win32/lwxaudio2.cpp
            win32/lwxaudio2.h
            win32/net.c
            win32/scriptwatch.h
            win32/scriptwatch.c
            win32/lwimgui.cpp
            win32/lwimgui.h
            )
    set(SERVER_WIN32_DEPS

            )
elseif (APPLE)
    set(ICON osx/laidoff.icns)
    set_source_files_properties(osx/laidoff.icns PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources")
    set_source_files_properties(assets PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources")

    set(OSX_DEPS
            win32/etc1.cpp
            win32/file.c
            win32/image.cpp
            win32/key.c
            win32/main.c
            win32/mouse.c
            win32/sound.cpp
            win32/net.c
            osx/osx_image.m
            osx/osx_sound.m
            assets # mac os executable bundles its external resources
            )
elseif (RPI)


    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DRPI -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi -std=gnu11")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRPI")

    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/opt/vc/lib/ -lbrcmGLESv2 -lbrcmEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm -L/opt/vc/src/hello_pi/libs/ilclient -L/opt/vc/src/hello_pi/libs/vgfont")

    set(RPI_DEPS
            win32/etc1.cpp
            win32/file.c
            win32/image.cpp
            rpi/key.c
            rpi/main.c
            rpi/mouse.c
            win32/sound.cpp
            rpi/rpi_image.c
            ios/laidoff/net.c
            )
elseif (UNIX)

    set(UNIX_DEPS
            win32/etc1.cpp
            win32/file.c
            win32/image.cpp
            win32/key.c
            win32/main.c
            win32/mouse.c
            win32/sound.cpp
            ios/laidoff/net.c
            rpi/rpi_image.c
            )
endif ()

set(GLAD "${GLFW_SOURCE_DIR}/deps/glad/glad.h"
        "${GLFW_SOURCE_DIR}/deps/glad.c")
set(GETOPT "${GLFW_SOURCE_DIR}/deps/getopt.h"
        "${GLFW_SOURCE_DIR}/deps/getopt.c")
set(TINYCTHREAD "${GLFW_SOURCE_DIR}/deps/tinycthread.h"
        "${GLFW_SOURCE_DIR}/deps/tinycthread.c")

file(GLOB LUA_LIB "lua-5.3.4/*.h" "lua-5.3.4/*.c")
file(GLOB ODE_LIB "ode/ode/src/*.h" "ode/ode/src/*.c" "ode/ode/src/*.cpp" "ode/ode/src/joints/*.h" "ode/ode/src/joints/*.cpp" "ode/ou/src/ou/*.cpp" "ode/include/ode/*.h" "ode/ou/include/ou/*.h")
file(GLOB LAIDOFF_LIB "src/*.h" "src/*.c" "src/*.cpp")
file(GLOB LAIDOFF_SERVER_LIB "serversrc/*.h" "serversrc/*.c" "serversrc/*.cpp")
file(GLOB DETOUR_LIB "recastnavigation/Detour/Include/*.h" "recastnavigation/Detour/Source/*.cpp")
file(GLOB ZMQ_LIB "zmq/src/*.c" "zmq/src/*.hpp" "zmq/src/*.cpp" "zmq/include/*.h")

source_group(Lua FILES ${LUA_LIB})
source_group(Ode FILES ${ODE_LIB})
source_group(Detour FILES ${DETOUR_LIB})
source_group(Zmq FILES ${ZMQ_LIB})
source_group(Win32 FILES ${WIN32_DEPS} ${ICON})
source_group(Imgui FILES ${IMGUI})

if (NOT SERVER_ONLY)
    #add_executable(laidoff WIN32 MACOSX_BUNDLE laidoff.c ${ICON} ${GLAD})
    add_executable(laidoff
            MACOSX_BUNDLE
            ${LAIDOFF_LIB}
            ${ODE_LIB}
            ${LUA_LIB}
            ${DETOUR_LIB}
            #${ZMQ_LIB}
            ${IMGUI}
            ${WIN32_DEPS}
            ${OSX_DEPS}
            ${RPI_DEPS}
            ${UNIX_DEPS}
            ${ICON}
            ${GLAD})
    target_compile_definitions(laidoff PRIVATE _WINSOCK_DEPRECATED_NO_WARNINGS)

    target_link_libraries(laidoff
            glfw
            libzmq
            czmq
			litehtml
            ${WINCODEC}
            ${WINDSHOW}
            ${WINOPENGL32}
            ${WINMM}
            ${WINXAUDIO2}
            ${WINSOCK2}
		lz4_static
            )
endif (NOT SERVER_ONLY)

add_executable(laidoff-server
        src/puckgame.c
        src/puckgame.h
        src/pcg_basic.c
        src/pcg_basic.h
        src/lwtcp.c
        src/lwtcp.h
        src/numcomp.c
        src/numcomp.h
        src/numcomp_puck_game.c
        src/numcomp_puck_game.h
        ${LAIDOFF_SERVER_LIB}
        ${SERVER_WIN32_DEPS}
        ${ODE_LIB}
        ${SERVERICON})
target_compile_definitions(laidoff-server PRIVATE LW_IS_SERVER=1 _WINSOCK_DEPRECATED_NO_WARNINGS)

target_link_libraries(laidoff-server
        ${WINSOCK2}
        tinycthread
        ${CMAKE_THREAD_LIBS_INIT} # pthread deps on Linux server build
        lz4_static
        )

if (NOT SERVER_ONLY)
    set(WINDOWS_BINARIES laidoff laidoff-server)
else (NOT SERVER_ONLY)
    set(WINDOWS_BINARIES laidoff-server)
endif (NOT SERVER_ONLY)

if (MSVC)# Tell MSVC to use 'main' instead of 'WinMain' for Windows subsystem executables
    set_target_properties(${WINDOWS_BINARIES} PROPERTIES
            LINK_FLAGS "/ENTRY:mainCRTStartup")
elseif (APPLE)

endif ()

# 빌드 결과를 한곳으로 모은다. (DLL, EXE)
if (NOT SERVER_ONLY)
    set(OUTPUT_BINARIES laidoff libzmq czmq laidoff-server)
else (NOT SERVER_ONLY)
    set(OUTPUT_BINARIES libzmq czmq laidoff-server)
endif (NOT SERVER_ONLY)

set_target_properties(${OUTPUT_BINARIES}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )
