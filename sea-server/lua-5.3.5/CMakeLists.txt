cmake_minimum_required(VERSION 3.5)
project(lua-5.3.5)

file(GLOB LUA_LIB "*.h" "*.c")

add_library(lua STATIC ${LUA_LIB})
