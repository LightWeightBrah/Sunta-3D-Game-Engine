# ========================= GAME (PROGRAM WITHOUT EDITOR) ========================= 

project(SuntaRuntime)

include("${CMAKE_SOURCE_DIR}/cmake/SuntaAppIcon.cmake")

# ========================= GAME NAME ========================= 

# The name of the executable and of the package folder comes from "name" in <project>/res/game.config
set(GAME_CONFIG_PATH "${SUNTA_PROJECT_DIR}/res/game.config")
set(GAME_NAME "Game")

if(EXISTS "${GAME_CONFIG_PATH}")
	file(READ "${GAME_CONFIG_PATH}" GAME_CONFIG_CONTENT)
	string(JSON CONFIG_GAME_NAME ERROR_VARIABLE CONFIG_ERROR GET "${GAME_CONFIG_CONTENT}" name)

	if(NOT CONFIG_ERROR)
		set(GAME_NAME "${CONFIG_GAME_NAME}")
	endif()

	# re-run CMake when the config changes, so the name stays up to date
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${GAME_CONFIG_PATH}")
endif()

message(STATUS "Game: '${GAME_NAME}'")

# ========================= EXECUTABLE ========================= 

set(RUNTIME_APP_SOURCES "src/main.cpp")
sunta_add_app_icon(RUNTIME_APP_SOURCES "${SUNTA_PROJECT_DIR}/res")

add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${RUNTIME_APP_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE Sunta)

# Only used when started from the IDE, a packaged game reads its own "res" folder
if(SUNTA_DEV_MODE)
	target_compile_definitions(${PROJECT_NAME} PRIVATE "SUNTA_DEFAULT_PROJECT_DIR=R\"(${SUNTA_PROJECT_DIR})\"")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
	OUTPUT_NAME "${GAME_NAME}"
	VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# ========================= PACKAGE ========================= 

# Result: distribution/<GameName>/ with the executable and res/Sunta + res/Game
# Resources used only by the editor (icons, UI backgrounds) are left out
set(EDITOR_ONLY_RESOURCES_REGEX "/Textures/(Icons|ui)(/|$)")

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION . BUNDLE DESTINATION .)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/Sunta/res/" DESTINATION res/Sunta
	REGEX "${EDITOR_ONLY_RESOURCES_REGEX}" EXCLUDE)

install(DIRECTORY "${SUNTA_PROJECT_DIR}/res/" DESTINATION res/Game)

# Build it with:  cmake --build <build dir> --config Release --target Package
add_custom_target(Package
	COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}" --config $<CONFIG> --prefix "${SUNTA_PACKAGE_DIR}/${GAME_NAME}"
	COMMENT "PACKAGING '${GAME_NAME}' TO: '${SUNTA_PACKAGE_DIR}/${GAME_NAME}'"
)
add_dependencies(Package ${PROJECT_NAME})
