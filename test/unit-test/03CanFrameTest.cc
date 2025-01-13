/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "LinuxTransportProtocol.h"
#include <catch2/catch.hpp>

using namespace tp;

TEST_CASE ("instantiate", "[canFrame]")
{
        CanFrame cf{0x00, true};
        REQUIRE (cf.dlc == 0);
}
