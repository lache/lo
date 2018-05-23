cmake_minimum_required(VERSION 3.5)
project(sea-server)
set(Boost_USE_STATIC_LIBS ON)
FIND_PACKAGE(Boost 1.58 COMPONENTS system date_time regex filesystem atomic REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
if (MSVC)
    add_definitions(
            -D_WIN32_WINNT=0x0501
            -D_CRT_NONSTDC_NO_WARNINGS
            -D_CRT_SECURE_NO_WARNINGS
            -D_SCL_SECURE_NO_WARNINGS
            -DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)
endif ()

if (MSVC)
    #/wd4819: disable warning C4819: 'The file contains a character that cannot be represented in the current code page (949).'
    # /WX: treat warning as error
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4819 /WX")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4819 /WX")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif ()

MACRO(ADD_MSVC_PRECOMPILED_HEADER PrecompiledHeader PrecompiledSource SourcesVar)
    IF (MSVC)
        GET_FILENAME_COMPONENT(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
        SET(PrecompiledBinary "$(IntDir)/${PrecompiledBasename}.pch")
        SET(Sources ${${SourcesVar}})

        SET_SOURCE_FILES_PROPERTIES(${PrecompiledSource}
                PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                OBJECT_OUTPUTS "${PrecompiledBinary}")
        SET_SOURCE_FILES_PROPERTIES(${Sources}
                PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\" /FI\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                OBJECT_DEPENDS "${PrecompiledBinary}")
        # Add precompiled header to SourcesVar
        LIST(APPEND ${SourcesVar} ${PrecompiledSource})
    ENDIF (MSVC)
ENDMACRO(ADD_MSVC_PRECOMPILED_HEADER)

# lz4

add_subdirectory(lz4/contrib/cmake_unofficial)
include_directories("lz4/lib")

# pthread deps on Linux server build
find_package(Threads)

SET(ServerSources
        sea_server.cpp
        udp_server.cpp
        udp_server.hpp
        udp_admin_server.cpp
        udp_admin_server.hpp
        tcp_server.cpp
        tcp_server.hpp
        tcp_connection.cpp
        tcp_connection.hpp
        sea.cpp
        sea.hpp
        sea_object.cpp
        sea_object.hpp
        sea_static.cpp
        sea_static.hpp
        sea_static_object.cpp
        sea_static_object.hpp
        seaport_object.cpp
        seaport_object.hpp
        seaport.cpp
        seaport.hpp
        astarrtree.cpp
        astarrtree.hpp
        xy.hpp
        route.hpp
        route.cpp
        region.hpp
        region.cpp
        seaarea.hpp
        seaarea.cpp
        sealog.hpp
        spinlock.hpp
        city.cpp
        city.hpp
        city_object.cpp
        city_object.hpp
        packet.h
        cargo.cpp
		cargo.h)

ADD_MSVC_PRECOMPILED_HEADER("precompiled.hpp" "precompiled.cpp" ServerSources)
ADD_EXECUTABLE(sea-server ${ServerSources} AStar.c AStar.h CohenSutherland.c CohenSutherland.h)

TARGET_LINK_LIBRARIES(sea-server LINK_PUBLIC ${Boost_LIBRARIES} lz4_static ${CMAKE_THREAD_LIBS_INIT})
