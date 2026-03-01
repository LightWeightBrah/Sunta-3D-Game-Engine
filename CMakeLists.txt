# ============ GLOBAL SETTINGS ============

cmake_minimum_required(VERSION 3.20)
project(SuntaEngine VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ============ DEPENDENCIES ============

# --- GLFW ---
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE) #This generates GLFW install files in global OS, so we can turn it off
add_subdirectory(Dependencies/GLFW)

# --- Assimp ---
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) #global flag but needed for Assimp 

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
add_subdirectory(Dependencies/Assimp)

#  ============ SUNTA MODULES ============

# --- PLATFORMS ---
if(WIN32)
	add_definitions(-DSUNTA_PLATFORM_WINDOWS)
endif()

# --- PROJECTS ---

add_subdirectory(Sunta)
add_subdirectory(Game)

