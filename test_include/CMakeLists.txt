cmake_minimum_required(VERSION 3.10)
project(TestInclude)

# Define the absolute paths
set(BUILD_DIR "/home/wanglab/code/behavior_analysis/whisker_tracking/whisk/build")
set(GENERATED_DIR "/home/wanglab/code/behavior_analysis/whisker_tracking/whisk/src/generated/parameters")
set(BUILD_PARAMETERS_DIR "/home/wanglab/code/behavior_analysis/whisker_tracking/whisk/build/src/parameters")

# Debug prints
message(STATUS "Build directory: ${BUILD_DIR}")
message(STATUS "Generated parameters directory: ${GENERATED_DIR}")
message(STATUS "Build parameters directory: ${BUILD_PARAMETERS_DIR}")

# Include directories
include_directories("${BUILD_DIR}")
include_directories("${GENERATED_DIR}")
include_directories("${BUILD_PARAMETERS_DIR}")

add_executable(test_include main.c)
