CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET (CMAKE_VERBOSE_MAKEFILE OFF)

# hardcoded!
SET (CMAKE_BUILD_TYPE Debug)
SET(CMAKE_C_FLAGS "-std=gnu99 -Wall" CACHE INTERNAL "c compiler flags")
SET(CMAKE_CXX_FLAGS "-std=c++17 -Wall" CACHE INTERNAL "cxx compiler flags")
ADD_DEFINITIONS ("-DUNIT_TEST")

PROJECT (cpp-iso-tp-example)
INCLUDE_DIRECTORIES("../../src/")


ADD_EXECUTABLE(${CMAKE_PROJECT_NAME}
    "main.cc"    
    "../../src/CanFrame.cc"
    "../../src/CanFrame.h"
#    "../../src/Iso15765TransportProtocol.cc"
    "../../src/Iso15765TransportProtocol.h"
    "../../src/ITransportProtocolCallback.h"
    "../../src/Types.h"
    "../../src/TransportMessage.h"
    "../../src/Timer.h"
    "../../src/TimerUnit.cc"
)

