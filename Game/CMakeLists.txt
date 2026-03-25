# ========================= GAME MODULE ========================= 

project(Game)

# ========================= SOURCES ========================= 

file(GLOB_RECURSE GAME_SOURCES "src/*.cpp" "src/*.h")
add_executable(${PROJECT_NAME} ${GAME_SOURCES})

# ========================= LINKING ========================= 

target_link_libraries(${PROJECT_NAME} PRIVATE Sunta)

# ========================= POST-BUILD ASSETS COPY ========================= 

set(FINAL_RES_DIR "${SUNTA_BIN_DIR}/res")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    
    # Copying Engine sources to output directory for runtime access (e.g. shaders, textures, models)
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Sunta/res" "${FINAL_RES_DIR}/Sunta"

    # Copying Game sources to output directory
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Game/res" "${FINAL_RES_DIR}/Game"

    COMMENT "COPYING ASSETS TO: '${FINAL_RES_DIR}' FOR RUNTIME ACCESS"
    
)
