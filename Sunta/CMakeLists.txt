# ========================= SUNTA ENGINE CORE ============================  

project(Sunta)

# ========================= ENGINE SOURCES ===============================  

file(GLOB_RECURSE ENGINE_SOURCES "src/*.cpp" "src/*.h")

#								   REGEX "Regular Expressions"
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "src/Sunta/Platform/.*") # . means any sign (letter, number, / etc.); * means repeat previous sign n times

# ====================== RENDERER API SOURCES ============================      

file(GLOB_RECURSE OPENGL_SOURCES  "src/Sunta/Platform/OpenGL/*.cpp" "src/Sunta/Platform/OpenGL/*.h")
list(APPEND ENGINE_SOURCES ${OPENGL_SOURCES})

# ========================= PLATFORM DETECTION ===========================  

# Windows
if(WIN32)
	file(GLOB_RECURSE PLATFORM_SOURCES "src/Sunta/Platform/Windows/*.cpp" "src/Sunta/Platform/Windows/*.h")
	set(PLATFORM_LIBS opengl32.lib)

# Linux
elseif(UNIX AND NOT APPLE)
	file(GLOB_RECURSE PLATFORM_SOURCES "src/Sunta/Platform/Linux/*.cpp" "src/Sunta/Platform/Linux/*.h")
	set(PLATFORM_LIBS GL X11 pthread dl)

# Mac
elseif(APPLE)
	file(GLOB_RECURSE PLATFORM_SOURCES "src/Sunta/Platform/Mac/*.cpp" "src/Sunta/Platform/Mac/*.h")
	set(PLATFORM_LIBS 
	"-framework OpenGL" 
	"-framework Cocoa" 
	"-framework IOKit"
	"-framework CoreVideo"
    "-framework QuartzCore")
endif()

list(APPEND ENGINE_SOURCES ${PLATFORM_SOURCES})

# ========================= DEPENDENCIES SOURCES ========================= 

list(APPEND ENGINE_SOURCES
"${CMAKE_SOURCE_DIR}/Dependencies/GLAD/src/glad.c"
"${CMAKE_SOURCE_DIR}/Dependencies/imgui_wrapper/imgui/imgui.cpp"
"${CMAKE_SOURCE_DIR}/Dependencies/imgui_wrapper/imgui/imgui_draw.cpp"
"${CMAKE_SOURCE_DIR}/Dependencies/imgui_wrapper/imgui/imgui_widgets.cpp"
"${CMAKE_SOURCE_DIR}/Dependencies/imgui_wrapper/imgui/imgui_tables.cpp"
"${CMAKE_SOURCE_DIR}/Dependencies/imgui_wrapper/imgui/backends/imgui_impl_glfw.cpp"
"${CMAKE_SOURCE_DIR}/Dependencies/imgui_wrapper/imgui/backends/imgui_impl_opengl3.cpp"
"${CMAKE_SOURCE_DIR}/Dependencies/stb_wrapper/stb/stb_image.cpp"
)

# ========================= IDE ORGANIZATION =============================  

set(SUNTA_INTERNAL_FILES ${ENGINE_SOURCES})
list(FILTER SUNTA_INTERNAL_FILES INCLUDE REGEX "${CMAKE_CURRENT_SOURCE_DIR}/src/.*")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${SUNTA_INTERNAL_FILES})

set(SUNTA_EXTERNAL_FILES ${ENGINE_SOURCES})
list(FILTER SUNTA_EXTERNAL_FILES EXCLUDE REGEX "${CMAKE_CURRENT_SOURCE_DIR}/src/.*")
source_group("Dependencies" FILES ${SUNTA_EXTERNAL_FILES})

# ========================= PRECOMPILED HEADER ===========================   

add_library(${PROJECT_NAME} STATIC ${ENGINE_SOURCES})

# --- GLAD FIX TO HIDE PRECOMPILED HEADER ---
set_source_files_properties("${CMAKE_SOURCE_DIR}/Dependencies/GLAD/src/glad.c" 
PROPERTIES LANGUAGE C
SKIP_PRECOMPILE_HEADERS ON)

# --- PRECOMPILED HEADER IMPLEMENTATION ---
target_precompile_headers(${PROJECT_NAME} PRIVATE "src/Sunta/Core/SuntaPreCompiled.h")

# ========================= MAIN API ACCESS ============================== 

# --- PUBLIC API (ACCESS: GAME & ENGINE) ---
target_include_directories(${PROJECT_NAME} PUBLIC
"${CMAKE_CURRENT_SOURCE_DIR}/src"
"${CMAKE_SOURCE_DIR}/Dependencies/glm_wrapper"
"${CMAKE_SOURCE_DIR}/Dependencies/GLAD/include"
)

# --- PRIVATE API (ACCESS: ENGINE) ---
target_include_directories(${PROJECT_NAME} PRIVATE
"${CMAKE_CURRENT_SOURCE_DIR}/src/Sunta"
"${CMAKE_SOURCE_DIR}/Dependencies/imgui_wrapper"					# for #include <imgui/imgui.h>
"${CMAKE_SOURCE_DIR}/Dependencies/imgui_wrapper/imgui"				# for internal imgui implementation fixes (searching imgui_internal.h)
"${CMAKE_SOURCE_DIR}/Dependencies/imgui_wrapper/imgui/backends"		# for internal imgui implementation (files from folder backends can see folder imgui)
"${CMAKE_SOURCE_DIR}/Dependencies/stb_wrapper"
)

# =========================== LINKING ====================================  

target_link_libraries(${PROJECT_NAME} PUBLIC 
glfw 
assimp 
${PLATFORM_LIBS}
)