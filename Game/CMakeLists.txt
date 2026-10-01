# ========================= GAME MODULE ========================= 

project(Game)

# ========================= SOURCES ========================= 

file(GLOB_RECURSE GAME_SOURCES "src/*.cpp" "src/*.h")

# ========================= APP EXE ICON ========================= 

# --- PLATFORMS ---

# Windows
if(WIN32)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/res/App/resource.rc")
        message(STATUS "Game: Using custom Game executable icon")
        list(APPEND GAME_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/res/App/resource.rc")

    elseif(EXISTS "${CMAKE_SOURCE_DIR}/Sunta/res/App/resource.rc")
        message(STATUS "Game: Using default Sunta Engine executable icon")
        list(APPEND GAME_SOURCES "${CMAKE_SOURCE_DIR}/Sunta/res/App/resource.rc")

    endif()

# Linux
elseif(UNIX AND NOT APPLE)
	message(STATUS "Game: Linux platform detected (Icons handled via desktop files and runtime GLFW)")

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/res/App/app.desktop.in")
        set(DESKTOP_IN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/res/App/app.desktop.in")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/Sunta/res/App/app.desktop.in")
        set(DESKTOP_IN_PATH "${CMAKE_SOURCE_DIR}/Sunta/res/App/app.desktop.in")
    endif()

    if(DESKTOP_IN_PATH)
        configure_file(
            "${DESKTOP_IN_PATH}"
            "${SUNTA_BIN_DIR}/${PROJECT_NAME}.desktop"
            @ONLY
        )
    endif()

# Mac
elseif(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE engine_logo.icns)

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/res/App/engine_logo.icns")
        set(MAC_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/res/App/engine_logo.icns")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/Sunta/res/App/engine_logo.icns")
        set(MAC_ICON_PATH "${CMAKE_SOURCE_DIR}/Sunta/res/App/engine_logo.icns")
    endif()

    if(MAC_ICON_PATH)
        message(STATUS "Game: Using App Icon for MacOS Bundle")
        list(APPEND GAME_SOURCES ${MAC_ICON_PATH})
        set_source_files_properties(${MAC_ICON_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    endif()

endif()

add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${GAME_SOURCES})

# ========================= LINKING ========================= 

target_link_libraries(${PROJECT_NAME} PRIVATE Sunta)

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# ========================= POST-BUILD ASSETS COPY ========================= 

if(NOT SUNTA_DEV_MODE)
    set(FINAL_RES_DIR "${SUNTA_BIN_DIR}/res")

    add_custom_target(CopyAssets ALL
    
        # If there's no directory: create new. If there IS directory: don't do anything 
        COMMAND ${CMAKE_COMMAND} -E make_directory "${FINAL_RES_DIR}/Sunta"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${FINAL_RES_DIR}/Game"

        # Copying Engine sources to output directory for runtime access (e.g. shaders, textures, models)
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Sunta/res" "${FINAL_RES_DIR}/Sunta"

        # Copying Game sources to output directory
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Game/res" "${FINAL_RES_DIR}/Game"

        COMMENT "COPYING ASSETS TO: '${FINAL_RES_DIR}' FOR RUNTIME ACCESS"
    
    )
    add_dependencies(${PROJECT_NAME} CopyAssets)

endif()