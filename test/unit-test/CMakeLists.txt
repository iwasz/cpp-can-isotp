CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET (CMAKE_VERBOSE_MAKEFILE OFF)

# hardcoded!
SET (CMAKE_BUILD_TYPE Debug)
SET(CMAKE_C_FLAGS "-std=gnu99 -Wall" CACHE INTERNAL "c compiler flags")
SET(CMAKE_CXX_FLAGS "-std=c++14 -Wall" CACHE INTERNAL "cxx compiler flags")
ADD_DEFINITIONS ("-DUNIT_TEST")

PROJECT (libmicrotest)
ENABLE_TESTING ()
INCLUDE_DIRECTORIES("../src/")


ADD_EXECUTABLE(libmicrotest
    "../src/storage/IRandomAccessStorage.h"
    "../src/storage/ICircullarQueueStorage.h"
    "../src/storage/FlashEepromStorage.h"
    "../src/storage/FileRandomAccessStorage.h"
    "../src/ErrorHandler.h"
    "../src/ErrorHandler.c"
    "../src/Timer.h"
    "../src/TimerUnit.cc"
    "../src/collection/CircularQueue.h"
    "../src/collection/Queue.h"
    "../src/collection/CircularBuffer.h"
    "../src/collection/CircularBuffer.cc"
    "../src/numeric/IncrementalAverage.h"
    "../src/numeric/IncrementalAverage.cc"
    "../src/numeric/RollingAverage.h"
    "../src/numeric/RollingAverage.cc"
    "00CatchInit.cc"
    "01StorageTest.cc"
    "02CcqTest.cc"
    "03QueueTest.cc"
    "04TimerTest.cc"
    "05NumericTest.cc"
    "06SourceSinkTest.cc"
    "07CircularBufferTest.cc"
)

ADD_TEST (libmicrotest libmicrotest)
