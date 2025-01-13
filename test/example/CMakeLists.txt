CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
SET (CMAKE_VERBOSE_MAKEFILE OFF)

# hardcoded!
SET (CMAKE_BUILD_TYPE Debug)
SET(CMAKE_C_FLAGS "-std=gnu99 -Wall" CACHE INTERNAL "c compiler flags")
SET(CMAKE_CXX_FLAGS "-std=c++17 -Wall" CACHE INTERNAL "cxx compiler flags")
ADD_DEFINITIONS ("-DUNIT_TEST")
include_directories("./")

ADD_EXECUTABLE(example
    "main.cc"    
    "../../src/TransportProtocol.h"
)

