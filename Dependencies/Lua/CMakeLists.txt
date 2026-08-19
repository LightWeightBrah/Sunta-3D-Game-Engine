cmake_minimum_required(VERSION 3.20)
project (Lua)

file(GLOB LUA_SOURCES "*.c" "*.h")

add_library(Lua STATIC ${LUA_SOURCES})

if(MSVC)
	target_compile_options(Lua PRIVATE /w) # ignore warnings
else()
	target_compile_options(Lua PRIVATE -w) # ignore warnings
endif()

target_include_directories(Lua PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")