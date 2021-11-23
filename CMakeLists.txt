cmake_minimum_required(VERSION 3.10)
project(MonoInjector)

file(GLOB_RECURSE SOURCE_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")

file(
    GLOB_RECURSE HEADER_FILES "${CMAKE_SOURCE_DIR}/src/*.hpp"
                              "${CMAKE_SOURCE_DIR}/src/*.h"
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
