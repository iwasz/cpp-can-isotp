CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET (CMAKE_VERBOSE_MAKEFILE OFF)

SET (CMAKE_BUILD_TYPE Debug)
SET(CMAKE_CXX_FLAGS "-std=c++17 -Wall" CACHE INTERNAL "cxx compiler flags")

ADD_EXECUTABLE(socket-test
    "../../src/TransportProtocol.h"
    "../../src/CanFrame.h"
    "../../src/Address.h"
    "../../src/MiscTypes.h"
    
    "main.cc"

    "../../deps/fmt/src/format.cc"
    "../../deps/fmt/src/posix.cc"
)

