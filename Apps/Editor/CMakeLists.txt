# ========================= SUNTA EDITOR (PROGRAM) ========================= 

project(SuntaEditor)

include("${CMAKE_SOURCE_DIR}/cmake/SuntaAppIcon.cmake")

set(EDITOR_APP_SOURCES "src/main.cpp")
sunta_add_app_icon(EDITOR_APP_SOURCES)

add_executable(${PROJECT_NAME} ${EDITOR_APP_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE SuntaEditorModule)

# The editor's Build menu copies SuntaRuntime.exe, so it has to be built together with the editor
add_dependencies(${PROJECT_NAME} SuntaRuntime)

# Project opened when no folder is given on the command line
target_compile_definitions(${PROJECT_NAME} PRIVATE "SUNTA_DEFAULT_PROJECT_DIR=R\"(${SUNTA_PROJECT_DIR})\"")

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# The Release editor opens without a console window (its log: the Console panel and SuntaEditor.log; started from a
# console, e.g. --build-game, it still writes there, see Terminal::Init). Debug keeps the console
set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
if(MSVC)
	# WIN32_EXECUTABLE makes the linker look for WinMain, this keeps the normal main()
	target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>)
endif()
