# ========================= GAME MODULE ========================= 

project(Game)

# ========================= SOURCES ========================= 

file(GLOB_RECURSE GAME_SOURCES "src/*.cpp" "src/*.h")
add_executable(${PROJECT_NAME} ${GAME_SOURCES})

# ========================= LINKING ========================= 

target_link_libraries(${PROJECT_NAME} PRIVATE Sunta)

# ========================= POST-BUILD ASSETS COPY ========================= 

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
"${CMAKE_SOURCE_DIR}/Sunta/res" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/res"
)
