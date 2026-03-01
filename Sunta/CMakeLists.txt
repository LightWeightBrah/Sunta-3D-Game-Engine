project(Sunta)

file(GLOB_RECURSE ENGINE_SOURCES "src/*.cpp" "src/*.h")

list(APPEND ENGINE_SOURCES
"${CMAKE_SOURCE_DIR}/Dependencies/GLAD/src/glad.c"
"${CMAKE_SOURCE_DIR}/Dependencies/imgui/imgui.cpp"
"${CMAKE_SOURCE_DIR}/Dependencies/imgui/imgui_draw.cpp"
"${CMAKE_SOURCE_DIR}/Dependencies/imgui/imgui_widgets.cpp"
"${CMAKE_SOURCE_DIR}/Dependencies/imgui/imgui_tables.cpp"
"${CMAKE_SOURCE_DIR}/Dependencies/imgui/imgui_impl_glfw.cpp"
"${CMAKE_SOURCE_DIR}/Dependencies/imgui/imgui_impl_opengl3.cpp"
"${CMAKE_SOURCE_DIR}/Dependencies/stb/stb_image.cpp"
)

add_library(${PROJECT_NAME} STATIC ${ENGINE_SOURCES})
target_precompile_headers(${PROJECT_NAME} PUBLIC "src/SuntaPreCompiled.h")

target_include_directories(${PROJECT_NAME} PUBLIC src
"${CMAKE_SOURCE_DIR}/Dependencies/GLAD"
"${CMAKE_SOURCE_DIR}/Dependencies/glm"
"${CMAKE_SOURCE_DIR}/Dependencies/imgui"
"${CMAKE_SOURCE_DIR}/Dependencies/stb"
)

target_link_libraries(${PROJECT_NAME} PUBLIC glfw assimp opengl32.lib)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${ENGINE_SOURCES})