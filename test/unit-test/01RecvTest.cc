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

TEST_CASE ("rx 1B", "[recv]")
{
        bool called = false;
        auto tp = create ({0, 0}, [&called] (auto &&tm) {
                called = true;
                REQUIRE (tm.size () == 1);
                REQUIRE (tm[0] == 0x67);
        });

        tp.onCanNewFrame (CanFrame (0x00, true, 0x01, 0x67));
        REQUIRE (called);
}

TEST_CASE ("rx 7B", "[recv]")
{
        bool called = false;
        auto tp = create ({0, 0}, [&called] (auto &&tm) {
                called = true;
                REQUIRE (tm.size () == 7);
                REQUIRE (tm[0] == 0);
                REQUIRE (tm[1] == 1);
                REQUIRE (tm[2] == 2);
                REQUIRE (tm[3] == 3);
                REQUIRE (tm[4] == 4);
                REQUIRE (tm[5] == 5);
                REQUIRE (tm[6] == 6);
        });

        tp.onCanNewFrame (CanFrame (0x00, true, 0x07, 0, 1, 2, 3, 4, 5, 6));
        REQUIRE (called);
}

TEST_CASE ("rx 8B", "[recv]")
{
        bool called = false;
        bool flow = false;
        auto tp = create (
                {0, 0},
                [&called] (auto &&tm) {
                        called = true;
                        REQUIRE (tm.size () == 8);
                        REQUIRE (tm[0] == 0);
                        REQUIRE (tm[1] == 1);
                        REQUIRE (tm[2] == 2);
                        REQUIRE (tm[3] == 3);
                        REQUIRE (tm[4] == 4);
                        REQUIRE (tm[5] == 5);
                        REQUIRE (tm[6] == 6);
                        REQUIRE (tm[7] == 7);
                },
                [&flow] (auto &&canFrame) {
                        flow = true;
                        REQUIRE (true);
                        return true;
                });

        tp.onCanNewFrame (CanFrame (0x00, true, 0x10, 8, 0, 1, 2, 3, 4, 5));
        tp.onCanNewFrame (CanFrame (0x00, true, 0x21, 6, 7));

        REQUIRE (called);
        REQUIRE (flow);
}

TEST_CASE ("rx 13B", "[recv]")
{
        bool called = false;
        bool flow = false;
        auto tp = create (
                {0, 0},
                [&called] (auto &&tm) {
                        called = true;
                        REQUIRE (tm.size () == 13);
                        REQUIRE (tm[0] == 0);
                        REQUIRE (tm[1] == 1);
                        REQUIRE (tm[2] == 2);
                        REQUIRE (tm[3] == 3);
                        REQUIRE (tm[4] == 4);
                        REQUIRE (tm[5] == 5);
                        REQUIRE (tm[6] == 6);
                        REQUIRE (tm[7] == 7);
                        REQUIRE (tm[8] == 8);
                        REQUIRE (tm[9] == 9);
                        REQUIRE (tm[10] == 10);
                        REQUIRE (tm[11] == 11);
                        REQUIRE (tm[12] == 12);
                },
                [&flow] (auto &&canFrame) {
                        flow = true;
                        REQUIRE (canFrame.data[0] == 0x30);
                        return true;
                });

        // 13B fits into 2 CanFrames
        tp.onCanNewFrame (CanFrame (0x00, true, 0x10, 13, 0, 1, 2, 3, 4, 5));
        tp.onCanNewFrame (CanFrame (0x00, true, 0x21, 6, 7, 8, 9, 10, 11, 12));

        REQUIRE (called);
        REQUIRE (flow);
}

TEST_CASE ("rx 4095B", "[recv]")
{
        bool called = false;
        bool flow = false;

        auto tp = create (
                {0, 0},
                [&called] (auto &&tm) {
                        called = true;
                        REQUIRE (tm.size () == 4095);
                        REQUIRE (tm[0] == 0);
                        REQUIRE (tm[1] == 1);
                        REQUIRE (tm[2] == 2);
                        REQUIRE (tm[3] == 3);
                        REQUIRE (tm[4] == 4);
                        REQUIRE (tm[5] == 5);
                        REQUIRE (tm[6] == 6);
                        REQUIRE (tm[7] == 7);
                        REQUIRE (tm[8] == 8);
                        REQUIRE (tm[9] == 9);
                        REQUIRE (tm[10] == 10);
                        REQUIRE (tm[11] == 11);
                        REQUIRE (tm[12] == 12);
                        REQUIRE (tm[13] == 13);
                        //...
                        REQUIRE (tm[4087] == 247);
                        REQUIRE (tm[4088] == 248);
                        REQUIRE (tm[4089] == 249);
                        REQUIRE (tm[4090] == 250);
                        REQUIRE (tm[4091] == 251);
                        REQUIRE (tm[4092] == 252);
                        REQUIRE (tm[4093] == 253);
                        REQUIRE (tm[4094] == 254);
                },
                [&flow] (auto &&canFrame) {
                        flow = true;
                        REQUIRE (canFrame.data[0] == 0x30);
                        return true;
                });

        tp.onCanNewFrame (CanFrame (0x00, true, 0x1f, 0xff, 0, 1, 2, 3, 4, 5));

        // It happens so 4095 / 7 == 585
        int j = 6; // We stoped at 5 in the FIRST_FRAME, so we continue starting from 6
        int sn = 1;
        for (int i = 0; i < 4095 / 7 - 1; ++i, j += 7, ++sn) {
                j %= 256;
                sn %= 16;
                tp.onCanNewFrame (CanFrame (0x00, true, 0x20 | sn, j, j + 1, j + 2, j + 3, j + 4, j + 5, j + 6));
        }

        j %= 256;
        sn %= 16;
        // First frame had 6B, then in the loop we add 584*7 = 4088, and then only 1. 6+4088+1 == 4095 == 0xfff
        tp.onCanNewFrame (CanFrame (0x00, true, 0x20 | sn, j));

        REQUIRE (called);
        REQUIRE (flow);
}

/**
 * This tests one particular problem I've had.
 * MessagePack data :
 * [131, 163, 114, 101, 113, 1, 164, 97, 100, 100, 114, 0, 163, 118, 97, 108, 1]
 *
 * means:
 * {
 *   "req": 1,
 *   "addr": 0,
 *   "val": 1
 * }
 *
 * In the CAN layer it should look like this:
 *
 * slcan0  18DA2211   [8]  10 11 83 A3 72 65 71 01
 * slcan0  18DA1122   [3]  30 00 01
 * slcan0  18DA2211   [8]  21 A4 61 64 64 72 00 A3
 * slcan0  18DA2211   [5]  22 76 61 6C 01
 */
TEST_CASE ("rx MessagePack", "[recv]")
{
        bool called = false;
        bool flow = false;

        static constexpr size_t ISO_MESSAGE_SIZE = 512;
        using MyIsoMessage = etl::vector<uint8_t, ISO_MESSAGE_SIZE>;
        auto tp = create<CanFrame, NormalFixed29AddressEncoder, MyIsoMessage, ISO_MESSAGE_SIZE> (
                Address{0x00, 0x00, 0x22, 0x11},
                [&called] (auto const &tm) {
                        called = true;
                        REQUIRE (tm.size () == 17);
                        REQUIRE (tm[0] == 131);
                        REQUIRE (tm[1] == 163);
                        REQUIRE (tm[2] == 114);
                        REQUIRE (tm[3] == 101);
                        REQUIRE (tm[4] == 113);
                        REQUIRE (tm[5] == 1);
                        REQUIRE (tm[6] == 164);
                        REQUIRE (tm[7] == 97);
                        REQUIRE (tm[8] == 100);
                        REQUIRE (tm[9] == 100);
                        REQUIRE (tm[10] == 114);
                        REQUIRE (tm[11] == 0);
                        REQUIRE (tm[12] == 163);
                        REQUIRE (tm[13] == 118);
                        REQUIRE (tm[14] == 97);
                        REQUIRE (tm[15] == 108);
                        REQUIRE (tm[16] == 1);
                },
                [&flow] (auto &&canFrame) {
                        flow = true;
                        REQUIRE (canFrame.data[0] == 0x30);
                        return true;
                });

        tp.onCanNewFrame (CanFrame (0x18DA2211, true, 0x10, 0x11, 0x83, 0xA3, 0x72, 0x65, 0x71, 0x01));
        // Here it responds with (it should respond) 18DA1122   [3]  30 00 01
        tp.onCanNewFrame (CanFrame (0x18DA2211, true, 0x21, 0xA4, 0x61, 0x64, 0x64, 0x72, 0x00, 0xA3));
        tp.onCanNewFrame (CanFrame (0x18DA2211, true, 0x22, 0x76, 0x61, 0x6C, 0x01));

        REQUIRE (called);
        REQUIRE (flow);
}
