/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "Iso15765TransportProtocol.h"
#include "catch.hpp"

using namespace tp;

TEST_CASE ("cross half-duplex 1B", "[crosswise]")
{
        bool called = false;
        auto tpR = create (
                [&called](auto &&isoMessage) {
                        called = true;
                        REQUIRE (isoMessage.size () == 1);
                        REQUIRE (isoMessage[0] == 0x55);
                },
                [](auto &&canFrame) { return true; });

        auto tpT = create ([](auto &&) {},
                           [&tpR](auto &&canFrame) {
                                   tpR.onCanNewFrame (canFrame);
                                   return true;
                           });

        tpT.send ({ 0x89 }, { 0x55 });

        while (tpT.isSending ()) {
                tpT.run ();
        }
}

TEST_CASE ("cross half-duplex 16B", "[crosswise]")
{
        bool called = false;
        auto tpR = create (
                [&called](auto &&isoMessage) {
                        called = true;
                        REQUIRE (isoMessage.size () == 16);
                        REQUIRE (isoMessage[0] == 0);
                        REQUIRE (isoMessage[1] == 1);
                        REQUIRE (isoMessage[2] == 2);
                        REQUIRE (isoMessage[3] == 3);
                        REQUIRE (isoMessage[4] == 4);
                        REQUIRE (isoMessage[5] == 5);
                        REQUIRE (isoMessage[6] == 6);
                        REQUIRE (isoMessage[7] == 7);
                        REQUIRE (isoMessage[8] == 8);
                        REQUIRE (isoMessage[9] == 9);
                        REQUIRE (isoMessage[10] == 10);
                        REQUIRE (isoMessage[11] == 11);
                        REQUIRE (isoMessage[12] == 12);
                        REQUIRE (isoMessage[13] == 13);
                        REQUIRE (isoMessage[14] == 14);
                        REQUIRE (isoMessage[15] == 15);
                },
                [](auto &&canFrame) {
                        // TODO
                        tpT.onCanNewFrame (canFrame);
                        return true;
                });

        auto tpT = create ([](auto &&) {},
                           [&tpR](auto &&canFrame) {
                                   tpR.onCanNewFrame (canFrame);
                                   return true;
                           });

        // ????? round reference here.
        auto newFrame = [&tpT](CanFrame &&canFrame) { tpT.onCanNewFrame (canFrame); };
        decltype (newFrame) *nreFrameCallPtr = &newFrame;

        tpT.send ({ 0x89 }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });

        while (tpT.isSending ()) {
                tpT.run ();
        }
}
