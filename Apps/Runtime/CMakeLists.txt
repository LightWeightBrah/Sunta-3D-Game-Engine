# ========================= GAME EXECUTABLE (TEMPLATE) ========================= 

# Always called SuntaRuntime. The editor (Project > Build Game) copies it next to the game's resources,
# gives the copy the name and the icon of the game (see GameBuilder), so this program is only a template
# and has the default icon of the engine

project(SuntaRuntime)

include("${CMAKE_SOURCE_DIR}/cmake/SuntaAppIcon.cmake")

set(RUNTIME_APP_SOURCES "src/main.cpp")
sunta_add_app_icon(RUNTIME_APP_SOURCES)

add_executable(${PROJECT_NAME} ${RUNTIME_APP_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE Sunta)

# A Release game opens without a console window (its log goes to a file, see Log), Debug keeps the console
set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)

if(MSVC)
	# WIN32_EXECUTABLE makes the linker look for WinMain, this keeps the normal main()
	target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>)
endif()

# Only used when started from the IDE, a built game reads its own "res" folder
target_compile_definitions(${PROJECT_NAME} PRIVATE "SUNTA_DEFAULT_PROJECT_DIR=R\"(${SUNTA_PROJECT_DIR})\"")

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
