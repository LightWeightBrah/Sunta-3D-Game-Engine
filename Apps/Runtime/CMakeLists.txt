# ========================= GAME EXECUTABLE (TEMPLATE) ========================= 

# Always called SuntaRuntime. The editor (Project > Build Game) copies it next to the game's resources,
# gives the copy the name and the icon of the game (see GameBuilder), so this program is only a template
# and has the default icon of the engine

project(SuntaRuntime)

include("${CMAKE_SOURCE_DIR}/cmake/SuntaAppIcon.cmake")

set(RUNTIME_APP_SOURCES "src/main.cpp")
sunta_add_app_icon(RUNTIME_APP_SOURCES "")

add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${RUNTIME_APP_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE Sunta)

# Only used when started from the IDE, a built game reads its own "res" folder
if(SUNTA_DEV_MODE)
	target_compile_definitions(${PROJECT_NAME} PRIVATE "SUNTA_DEFAULT_PROJECT_DIR=R\"(${SUNTA_PROJECT_DIR})\"")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
