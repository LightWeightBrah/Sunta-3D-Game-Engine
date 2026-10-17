# ========================= GLOBAL SETTINGS ========================= 

cmake_minimum_required(VERSION 3.20)
project(SuntaEngine VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(SUNTA_DEV_MODE "Development mode: the game build reads assets from the source folders when there is no 'res' next to it" ON)

if(SUNTA_DEV_MODE)
	message(STATUS "Sunta Engine: DEVELOPMENT mode (game build falls back to the source folders when it has no 'res' folder)")
else()
	message(STATUS "Sunta Engine: RELEASE mode (game build needs the 'res' folder next to it, build target 'Package')")
endif()

# The game project the editor opens by default and the game executable is built from
# A project is a folder with "res/" inside (and "res/game.config"), see Projects/
set(SUNTA_PROJECT "Projects/MyGame" CACHE STRING "Game project (folder with 'res'), relative to the repository or absolute")
cmake_path(ABSOLUTE_PATH SUNTA_PROJECT BASE_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE SUNTA_PROJECT_DIR NORMALIZE)
message(STATUS "Sunta Engine: Game project: ${SUNTA_PROJECT_DIR}")

# Lua version
add_compile_definitions(SOL_LUA_VERSION=50400)

# ========================= OUTPUT SETTINGS ========================= 

set(CONFIG_NAME "$<CONFIG>-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

set(SUNTA_BIN_DIR "${CMAKE_SOURCE_DIR}/bin/${CONFIG_NAME}")
set(SUNTA_INT_DIR "${CMAKE_SOURCE_DIR}/bin-intermediates/${CONFIG_NAME}")
set(SUNTA_PACKAGE_DIR "${CMAKE_SOURCE_DIR}/distribution")

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
set(ASSIMP_BUILD_COLLADA_IMPORTER ON CACHE BOOL "" FORCE)
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

# Sunta            - engine core (static library, no editor)
# SuntaEditor      - editor program: core + editor module
# SuntaRuntime     - game program: core only. Content of the game lies in <project>/res
add_subdirectory(Sunta)
add_subdirectory(Apps/Editor)
add_subdirectory(Apps/Runtime)
