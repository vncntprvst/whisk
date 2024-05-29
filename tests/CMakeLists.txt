cmake_minimum_required(VERSION 3.10)
project(ffmpeg_tests)

set(CMAKE_C_STANDARD 99)

include_directories(
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/../build
    ${CMAKE_SOURCE_DIR}/../build/src/parameters
    ${CMAKE_SOURCE_DIR}/../src/generated/parameters
)

add_library(ffmpeg_adapt_lib STATIC ../src/ffmpeg_adapt.c)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil libavdevice libswscale)

# Add the test executables
add_executable(simple_test simple_test.c)
target_link_libraries(simple_test ffmpeg_adapt_lib PkgConfig::FFMPEG)

add_executable(pixel_format_test pixel_format_test.c)
target_link_libraries(pixel_format_test ffmpeg_adapt_lib PkgConfig::FFMPEG)

add_executable(ffmpeg_test ffmpeg_test.c)
target_link_libraries(ffmpeg_test ffmpeg_adapt_lib PkgConfig::FFMPEG)

add_executable(ffmpeg_image_alloc_test ffmpeg_image_alloc_test.c)
target_link_libraries(ffmpeg_image_alloc_test ffmpeg_adapt_lib PkgConfig::FFMPEG)

add_executable(ffmpeg_video_init_test ffmpeg_video_init_test.c)
target_link_libraries(ffmpeg_video_init_test ffmpeg_adapt_lib PkgConfig::FFMPEG)
