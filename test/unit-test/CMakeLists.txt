CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET (CMAKE_VERBOSE_MAKEFILE OFF)

SET (CMAKE_BUILD_TYPE Debug)
SET(CMAKE_CXX_FLAGS "-std=c++17 -Wall" CACHE INTERNAL "cxx compiler flags")
include_directories("./")

ENABLE_TESTING ()

ADD_DEFINITIONS ("-DUNIT_TEST=1")
ADD_EXECUTABLE(unit-test
    "../../src/TransportProtocol.h"
    "../../src/CanFrame.h"
    "../../src/Address.h"
    "../../src/MiscTypes.h"
    
    "etl_profile.h"
    "00CatchInit.cc"
    "01RecvTest.cc"
    "02SendTest.cc"
    "03CanFrameTest.cc"
    "04CrosswiseTest.cc"
    "05AddressTest.cc"
    "06BsAndStTest.cc"
    "07IsoMessageTest.cc"
    "08CallbackTest.cc"
)

ADD_TEST (unit-test unit-test)
