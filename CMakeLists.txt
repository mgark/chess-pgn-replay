cmake_minimum_required(VERSION 3.22)
set(TARGET_NAME chess_replay)
project(${TARGET_NAME})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CXX_STANDARD_REQUIRED 20)

set(SOURCE_FILES chess_replay.cpp)
set(HEADER_FILES 
        board.h 
        common.h 
        moves.h 
        scanner.h
        tokens.h)
add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE ${HEADER_FILES} ${SOURCE_FILES})
set(COMPILE_FLAGS ${CMAKE_CXX_FLAGS} -std=c++20)
target_compile_options(${TARGET_NAME} PRIVATE ${COMPILE_FLAGS})

set(TESTS_TARGET_NAME tests)
set(TESTS_SOURCE_FILES tests.cpp)
add_executable(${TESTS_TARGET_NAME})
target_sources(${TESTS_TARGET_NAME} PRIVATE ${TESTS_SOURCE_FILES})
set(COMPILE_FLAGS ${CMAKE_CXX_FLAGS} -std=c++20)
target_compile_options(${TESTS_TARGET_NAME} PRIVATE ${COMPILE_FLAGS})

