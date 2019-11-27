CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET (CMAKE_VERBOSE_MAKEFILE OFF)

SET (CMAKE_BUILD_TYPE Debug)

# TODO can't get it to work with ninja...
SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -fsanitize=address -fsanitize=leak")
SET (CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -fsanitize=address -fsanitize=leak")
SET (CMAKE_EXE_LINKER_FLAGS:STRING "-fsanitize=address  -fsanitize=leak")
SET (CMAKE_MODULE_LINKER_FLAGS:STRING "-fsanitize=address  -fsanitize=leak")

include_directories("./")

ENABLE_TESTING ()

ADD_DEFINITIONS ("-DUNIT_TEST=1")
ADD_EXECUTABLE(unit-test
    "../../src/Address.h"
    "../../src/CanFrame.h"
    "../../src/CppCompat.h"
    "../../src/LinuxCanFrame.h"
    "../../src/LinuxTransportProtocol.h"
    "../../src/MiscTypes.h"
    "../../src/StlTypes.h"
    "../../src/TransportProtocol.h"

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
