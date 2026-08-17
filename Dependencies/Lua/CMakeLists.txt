project (Lua)

file(GLOB LUA_SOURCES "*.c" "*.h")

add_library(Lua STATIC ${LUA_SOURCES})

if(MSVC)
	target_compile_options(Lua PRIVATE /w)
else()
	target_compile_options(Lua PRIVATE -w)
endif()

target_include_directories(Lua PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")