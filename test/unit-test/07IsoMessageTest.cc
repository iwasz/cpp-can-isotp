/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "LinuxTransportProtocol.h"
#include "catch.hpp"
#include <etl/vector.h>

using namespace tp;

/**
 *
 */
TEST_CASE ("Size constrained transmitter 16", "[isoMessage]")
{
        std::vector<CanFrame> framesFromR;
        std::vector<CanFrame> framesFromT;

        bool called = false;
        auto tpR = create (
                Address (0x89, 0x12), [&called] (auto const & /* isoMessage */) { called = true; },
                [&framesFromR] (auto const &canFrame) {
                        framesFromR.push_back (canFrame);
                        return true;
                });

        auto tpT = create<CanFrame, Normal29AddressEncoder, IsoMessage, 16> (
                Address (0x12, 0x89), [] (auto const & /*unused*/) {},
                [&framesFromT] (auto const &canFrame) {
                        framesFromT.push_back (canFrame);
                        return true;
                });

        REQUIRE (tpT.send ({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}));

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

TEST_CASE ("Size constrained transmitter 15", "[isoMessage]")
{
        std::vector<CanFrame> framesFromR;
        std::vector<CanFrame> framesFromT;

        bool called = false;
        auto tpR = create (
                Address (0x89, 0x12), [&called] (auto const & /* isoMessage */) { called = true; },
                [&framesFromR] (auto const &canFrame) {
                        framesFromR.push_back (canFrame);
                        return true;
                });

        auto tpT = create<CanFrame, Normal29AddressEncoder, IsoMessage, 15> (
                Address (0x12, 0x89), [] (auto const & /*unused*/) {},
                [&framesFromT] (auto const &canFrame) {
                        framesFromT.push_back (canFrame);
                        return true;
                });

        REQUIRE (!tpT.send ({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}));

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

        REQUIRE (!called);
}

TEST_CASE ("Size constrained receiver 16", "[isoMessage]")
{
        std::vector<CanFrame> framesFromR;
        std::vector<CanFrame> framesFromT;

        bool called = false;
        auto tpR = create<CanFrame, Normal29AddressEncoder, IsoMessage, 16> (
                Address (0x89, 0x12), [&called] (auto const & /* isoMessage */) { called = true; },
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

        REQUIRE (tpT.send ({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}));

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

TEST_CASE ("Size constrained receiver 15", "[isoMessage]")
{
        std::vector<CanFrame> framesFromR;
        std::vector<CanFrame> framesFromT;

        bool called = false;
        auto tpR = create<CanFrame, Normal29AddressEncoder, IsoMessage, 15> (
                Address (0x89, 0x12), [&called] (auto const & /* isoMessage */) { called = true; },
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

        REQUIRE (tpT.send ({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}));

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

        REQUIRE (!called);
        REQUIRE (tpT.transportMessagesMap.empty ());
}

TEST_CASE ("etl::vector 16B", "[isoMessage]")
{
        std::vector<CanFrame> framesFromR;
        std::vector<CanFrame> framesFromT;

        using EtlVec = etl::vector<uint8_t, 16>;
        int called = 0;

        auto tpR = create<CanFrame, Normal29AddressEncoder, EtlVec, 16> (
                Address (0x89, 0x12),
                [&called] (auto const &isoMessage) {
                        ++called;
                        REQUIRE (isoMessage.size () == 16);
                },
                [&framesFromR] (auto const &canFrame) {
                        framesFromR.push_back (canFrame);
                        return true;
                });

        auto tpT = create<CanFrame, Normal29AddressEncoder, EtlVec, 16> (
                Address (0x12, 0x89),
                [&called] (auto const &isoMessage) {
                        ++called;
                        REQUIRE (isoMessage.size () == 16);
                },
                [&framesFromT] (auto const &canFrame) {
                        framesFromT.push_back (canFrame);
                        return true;
                });

        EtlVec v (16);
        REQUIRE (tpT.send (std::ref (v)));

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
