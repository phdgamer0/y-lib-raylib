cmake_minimum_required(VERSION 3.16)
project(y-lib-raylib)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/919ad68c.zip
)
FetchContent_MakeAvailable(raylib)
# y-- source tree to compile against; yenv passes its installed toolchain
set(YLANG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../y---lang" CACHE PATH "y-- source directory")
add_library(y-raylib SHARED module.cpp)
# symbols come from the ymm process at load time
if(APPLE)
    target_link_options(y-raylib PRIVATE -undefined dynamic_lookup)
endif()
target_include_directories(y-raylib PRIVATE ${YLANG_DIR}/include ${YLANG_DIR}/include/ylang ${YLANG_DIR}/src)
target_link_libraries(y-raylib PRIVATE raylib)
if(NOT WIN32)
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
    target_link_libraries(y-raylib PRIVATE OpenGL::GL X11::X11 m pthread dl rt)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(y-raylib PRIVATE OpenGL::GL ws2_32 crypt32)
endif()
