CMAKE_MINIMUM_REQUIRED (VERSION 3.5 FATAL_ERROR)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0048 NEW)
endif(COMMAND cmake_policy)

PROJECT (cpp-can-isotp)
SET (CMAKE_CXX_FLAGS "-std=c++17 -Wall -fsanitize=address -fno-omit-frame-pointer" CACHE INTERNAL "cxx compiler flags")
INCLUDE_DIRECTORIES("src" "deps/GSL/include" "deps/etl/include" "deps/fmt/include")
add_subdirectory (test/example)
add_subdirectory (test/unit-test)
add_subdirectory (test/socket-test)

