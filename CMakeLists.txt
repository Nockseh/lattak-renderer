cmake_minimum_required(VERSION 3.25)
project(raylib_test C CXX)
set(CMAKE_CXX_STANDARD 17)

# raylib
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
if (EMSCRIPTEN)
    set(PLATFORM Web CACHE STRING "" FORCE)
endif()
FetchContent_Declare(raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/6.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(raylib)

# game
add_executable(game src/main.cpp)
target_link_libraries(game PRIVATE raylib)

# assets: everything in assets/ is available at runtime as "assets/..."
set(ASSETS_DIR ${CMAKE_SOURCE_DIR}/assets)
file(GLOB_RECURSE ASSET_FILES CONFIGURE_DEPENDS ${ASSETS_DIR}/*)

if (EMSCRIPTEN)
    # packed into game.data at link time; relink whenever an asset changes
    target_link_options(game PRIVATE "--preload-file=${ASSETS_DIR}@assets")
    set_property(TARGET game APPEND PROPERTY LINK_DEPENDS ${ASSET_FILES})
else()
    # copied next to the exe on every build (only changed files)
    add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                ${ASSETS_DIR} $<TARGET_FILE_DIR:game>/assets)
    add_dependencies(game copy_assets)
endif()

if (EMSCRIPTEN)
    set_target_properties(game PROPERTIES SUFFIX ".html")
    target_link_options(game PRIVATE
        -sUSE_GLFW=3
        -sALLOW_MEMORY_GROWTH=1
        "--shell-file=${raylib_SOURCE_DIR}/src/minshell.html")

    find_program(EMRUN emrun HINTS "${EMSCRIPTEN_ROOT_PATH}" REQUIRED)
    add_custom_target(serve
        COMMAND ${EMRUN} --no_browser --port 8080 game.html
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS game
        USES_TERMINAL)
else()
    target_link_options(game PRIVATE -static)
endif()