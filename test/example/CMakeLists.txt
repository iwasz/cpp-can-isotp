CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET (CMAKE_VERBOSE_MAKEFILE OFF)

# hardcoded!
SET (CMAKE_BUILD_TYPE Debug)
SET(CMAKE_C_FLAGS "-std=gnu99 -Wall" CACHE INTERNAL "c compiler flags")
SET(CMAKE_CXX_FLAGS "-std=c++17 -Wall" CACHE INTERNAL "cxx compiler flags")
ADD_DEFINITIONS ("-DUNIT_TEST")

INCLUDE_DIRECTORIES("../../src/")

ADD_EXECUTABLE(example
    "main.cc"    
    "../../src/Iso15765TransportProtocol.h"
)

