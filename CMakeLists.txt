# ========================= GLOBAL SETTINGS ========================= 

cmake_minimum_required(VERSION 3.20)
project(SuntaEngine VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(SUNTA_COPY_ASSETS "Copy assets to bin directory for distribution" OFF)

# ========================= OUTPUT SETTINGS ========================= 

set(CONFIG_NAME "$<CONFIG>-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

set(SUNTA_BIN_DIR "${CMAKE_SOURCE_DIR}/bin/${CONFIG_NAME}")
set(SUNTA_INT_DIR "${CMAKE_SOURCE_DIR}/bin-intermediates/${CONFIG_NAME}")

# for single-config generators (e.g. Makefiles)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SUNTA_BIN_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SUNTA_INT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${SUNTA_INT_DIR})
set(CMAKE_PDB_OUTPUT_DIRECTORY ${SUNTA_INT_DIR})


# for multi-config generators (e.g. Visual Studio)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES}) # Debug, Release, ...
	string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER) # e.g. change "Debug" to "DEBUG"

	# so we don't have bin/Debug/Game.exe, instead we have bin/Game.exe
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${SUNTA_BIN_DIR}) # Windows: .exe, .dll | Linux: .exe
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${SUNTA_INT_DIR}) # Windows:     -      | Linux: .so
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${SUNTA_INT_DIR}) # Windows:   .lib     | Linux: .a
	set(CMAKE_PDB_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER}     ${SUNTA_INT_DIR}) # Windows:   .pdb     | Linux:  -
endforeach()

# ========================= DEPENDENCIES ============================ 

# --- GLFW ---
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE) #This generates GLFW install files in global OS, so we can turn it off
add_subdirectory(Dependencies/GLFW)

# --- Assimp ---
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) #global flag but needed for Assimp 

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
add_subdirectory(Dependencies/Assimp)

# --- Native File Dialog Extended (nfd) ---
add_subdirectory(Dependencies/nfd)

# --- Lua integration ---
add_subdirectory(Dependencies/Lua)

#  ========================= SUNTA MODULES ========================= 

# --- PLATFORMS ---

# Windows
if(WIN32)
	add_definitions(-DSUNTA_PLATFORM_WINDOWS)
	message(STATUS "Sunta: Windows platform detected")

# Linux
elseif(UNIX AND NOT APPLE)
	add_definitions(-DSUNTA_PLATFORM_LINUX)
	message(STATUS "Sunta: Linux platform detected")

# Mac
elseif(APPLE)
	add_definitions(-DSUNTA_PLATFORM_MAC)
	message(STATUS "Sunta: MacOS platform detected")

endif()

# --- PROJECTS ---

add_subdirectory(Sunta)
add_subdirectory(Game)

