# ========================= SUNTA ENGINE CORE ========================= 

project(Sunta)

# --- ENGINE IMPLEMENTATIONS AND EXPLORER FILE TREE ---
file(GLOB_RECURSE ENGINE_SOURCES "src/*.cpp" "src/*.h")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${ENGINE_SOURCES})

# --- DEPENDENCIES IMPLEMENTATIONS ---
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

# ========================= PRECOMPILED HEADER ========================= 

add_library(${PROJECT_NAME} STATIC ${ENGINE_SOURCES})

# --- GLAD FIX TO HIDE PRECOMPILED HEADER ---
set_source_files_properties("${CMAKE_SOURCE_DIR}/Dependencies/GLAD/src/glad.c" 
PROPERTIES LANGUAGE C
SKIP_PRECOMPILE_HEADERS ON)

# --- PRECOMPILED HEADER IMPLEMENTATION ---
target_precompile_headers(${PROJECT_NAME} PRIVATE "src/Sunta/SuntaPreCompiled.h")

# ========================= MAIN API ACCESS ========================= 

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

# ========================= LINKING ========================= 

target_link_libraries(${PROJECT_NAME} PUBLIC 
glfw 
assimp 
opengl32.lib
)