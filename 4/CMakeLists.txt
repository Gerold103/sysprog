cmake_minimum_required(VERSION 3.0)
project(ThreadPool C CXX)

set(COMMON_FLAGS
    -Wextra
    -Werror
    -Wall
    -Wno-gnu-folding-constant
    -g
)
add_compile_options(${COMMON_FLAGS})

option(ENABLE_LEAK_CHECKS
    "Enable memory leak checks with heap_help"
    OFF)

option(ENABLE_GLOB_SEARCH
    "Enable compilation of all the files, not just the preselected ones"
    OFF)

set(UTILS_DIR ${CMAKE_SOURCE_DIR}/../utils)
set(UTILS_SOURCES ${UTILS_DIR}/unit.c)

include_directories(${UTILS_DIR})

if(ENABLE_LEAK_CHECKS)
    list(APPEND UTILS_SOURCES ${UTILS_DIR}/heap_help/heap_help.c)
    include_directories(${UTILS_DIR}/heap_help)
endif()

if(NOT ENABLE_GLOB_SEARCH)
    set(TEST_SOURCES
        thread_pool.c
        test.c
        ${UTILS_SOURCES}
    )
    add_executable(test ${TEST_SOURCES})
else()
    file(GLOB TEST_SOURCES *.c *.cpp *.cc)
    list(APPEND TEST_SOURCES ${UTILS_SOURCES})
    add_executable(test ${TEST_SOURCES})
endif()

target_link_libraries(test pthread)
