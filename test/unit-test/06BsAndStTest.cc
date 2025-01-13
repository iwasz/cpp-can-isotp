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

/**
 * This is simmilar to one of 04Crosswisetests, but this time block size is set
 * to something greater than 0. BS is set to 1, which means that for every consecutive
 * frame there is one flow frame with continue to send status
 */
TEST_CASE ("BlockSize test", "[address]")
{
        std::vector<CanFrame> framesFromR;
        std::vector<CanFrame> framesFromT;

        int called = 0;

        auto tpR = create (
                Address (0x12, 0x89),
                [&called] (auto const &isoMessage) {
                        ++called;
                        REQUIRE (isoMessage.size () == 4095);
                },
                [&framesFromR] (auto const &canFrame) {
                        framesFromR.push_back (canFrame);
                        return true;
                });

        auto tpT = create (
                Address (0x89, 0x12),
                [&called] (auto const &isoMessage) {
                        ++called;
                        REQUIRE (isoMessage.size () == 4095);
                },

                [&framesFromT] (auto const &canFrame) {
                        framesFromT.push_back (canFrame);
                        return true;
                });

        tpT.setBlockSize (1);
        tpT.send (std::vector<uint8_t> (4095));
        tpR.setBlockSize (1);
        tpR.send (std::vector<uint8_t> (4095));

        while (tpT.isSending () || tpR.isSending ()) {
                tpT.run ();
                for (CanFrame &f : framesFromT) {
                        tpR.onCanNewFrame (f);
                }
                framesFromT.clear ();

                tpR.run ();
                for (CanFrame &f : framesFromR) {
                        tpT.onCanNewFrame (f);
                }
                framesFromR.clear ();
        }

        REQUIRE (called == 2);
}

TEST_CASE ("SeparationTime test", "[address]")
{
        std::vector<CanFrame> framesFromR;
        std::vector<CanFrame> framesFromT;

        int called = 0;

        auto tpR = create (
                Address (0x12, 0x89),
                [&called] (auto const &isoMessage) {
                        ++called;
                        REQUIRE (isoMessage.size () == 4095);
                },
                [&framesFromR] (auto const &canFrame) {
                        framesFromR.push_back (canFrame);
                        return true;
                });

        auto tpT = create (
                Address (0x89, 0x12),
                [&called] (auto const &isoMessage) {
                        ++called;
                        REQUIRE (isoMessage.size () == 4095);
                },

                [&framesFromT] (auto const &canFrame) {
                        framesFromT.push_back (canFrame);
                        return true;
                });

        tpT.setSeparationTime (0x3);
        tpT.send (std::vector<uint8_t> (4095));
        tpR.setSeparationTime (0x3);
        tpR.send (std::vector<uint8_t> (4095));

        while (tpT.isSending () || tpR.isSending ()) {
                tpT.run ();
                for (CanFrame &f : framesFromT) {
                        tpR.onCanNewFrame (f);
                }
                framesFromT.clear ();

                tpR.run ();
                for (CanFrame &f : framesFromR) {
                        tpT.onCanNewFrame (f);
                }
                framesFromR.clear ();
        }

        REQUIRE (called == 2);
}
