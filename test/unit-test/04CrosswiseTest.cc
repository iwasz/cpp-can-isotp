/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "LinuxTransportProtocol.h"
#include <catch2/catch.hpp>
#include <vector>

using namespace tp;

TEST_CASE ("cross half-duplex 1B", "[crosswise]")
{
        bool called = false;
        auto tpR = create (
                Address (0x89, 0x67),
                [&called] (auto const &isoMessage) {
                        called = true;
                        REQUIRE (isoMessage.size () == 1);
                        REQUIRE (isoMessage[0] == 0x55);
                },
                [] (auto const & /* canFrame */) { return true; });

        auto tpT = create (
                Address (0x67, 0x89), [] (auto const & /*unused*/) {},
                [&tpR] (auto const &canFrame) {
                        tpR.onCanNewFrame (canFrame);
                        return true;
                });

        tpT.send ({0x55});

        while (tpT.isSending ()) {
                tpT.run ();
        }

        REQUIRE (called);
}

TEST_CASE ("cross half-duplex 16B", "[crosswise]")
{
        std::vector<CanFrame> framesFromR;
        std::vector<CanFrame> framesFromT;

        bool called = false;
        auto tpR = create (
                Address (0x89, 0x12),
                [&called] (auto const &isoMessage) {
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
                [&framesFromR] (auto const &canFrame) {
                        framesFromR.push_back (canFrame);
                        return true;
                });

        auto tpT = create (
                Address (0x12, 0x89), [] (auto const & /*unused*/) {},
                [&framesFromT] (auto const &canFrame) {
                        framesFromT.push_back (canFrame);
                        return true;
                });

        tpT.send ({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15});

        while (tpT.isSending ()) {
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

        REQUIRE (called);
}

TEST_CASE ("cross half-duplex 16B wrong source", "[crosswise]")
{
        std::vector<CanFrame> framesFromR;
        std::vector<CanFrame> framesFromT;

        bool called = false;
        // Target address is 0x12, source address is 0x89.
        auto tpR = create (
                Address (0x12, 0x89), [&called] (auto const & /* isoMessage */) { called = true; },
                [&framesFromR] (auto const &canFrame) {
                        framesFromR.push_back (canFrame);
                        return true;
                });

        // Target address is 0x89, source address is WRONGLY set to 0x13!
        auto tpT = create (
                Address (0x89, 0x13), [] (auto const &) {},
                [&framesFromT] (auto const &canFrame) {
                        framesFromT.push_back (canFrame);
                        return true;
                });

        tpT.send ({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15});

        while (tpT.isSending ()) {
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

        // Not called because of wrong source address
        REQUIRE (!called);
}

TEST_CASE ("cross full-duplex 16B", "[crosswise]")
{
        std::vector<CanFrame> framesFromR;
        std::vector<CanFrame> framesFromT;

        int called = 0;

        // Target address is 0x12, source address is 0x89.
        auto tpR = create (
                Address (0x12, 0x89),
                [&called] (auto const &isoMessage) {
                        ++called;
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
                [&framesFromR] (auto const &canFrame) {
                        framesFromR.push_back (canFrame);
                        return true;
                });

        // Target address is 0x89, source address is 0x12.
        auto tpT = create (
                Address (0x89, 0x12),
                [&called] (auto const &isoMessage) {
                        ++called;
                        REQUIRE (isoMessage.size () == 16);
                        REQUIRE (isoMessage[0] == 15);
                        REQUIRE (isoMessage[1] == 14);
                        REQUIRE (isoMessage[2] == 13);
                        REQUIRE (isoMessage[3] == 12);
                        REQUIRE (isoMessage[4] == 11);
                        REQUIRE (isoMessage[5] == 10);
                        REQUIRE (isoMessage[6] == 9);
                        REQUIRE (isoMessage[7] == 8);
                        REQUIRE (isoMessage[8] == 7);
                        REQUIRE (isoMessage[9] == 6);
                        REQUIRE (isoMessage[10] == 5);
                        REQUIRE (isoMessage[11] == 4);
                        REQUIRE (isoMessage[12] == 3);
                        REQUIRE (isoMessage[13] == 2);
                        REQUIRE (isoMessage[14] == 1);
                        REQUIRE (isoMessage[15] == 0);
                },

                [&framesFromT] (auto const &canFrame) {
                        framesFromT.push_back (canFrame);
                        return true;
                });

        tpT.send ({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15});
        tpR.send ({15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});

        while (tpT.isSending ()) {
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

TEST_CASE ("cross full-duplex 4095B", "[crosswise]")
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

        tpT.send (std::vector<uint8_t> (4095));
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
