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
#include <gsl/gsl>

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

/**
 * Another usecase from my work that failed.
 * Two consecutive transactions (request + response, request2 + response2). The
 * problem is response2 is not getting through.
 */
TEST_CASE ("MessagePack transaction", "[recv]")
{
        using Vec = std::vector<uint8_t>;

        int rxStage = 0;
        int txStage = 0;

        static constexpr size_t ISO_MESSAGE_SIZE = 128;
        using MyIsoMessage = etl::vector<uint8_t, ISO_MESSAGE_SIZE>;
        auto tp = create<CanFrame, NormalFixed29AddressEncoder, MyIsoMessage, ISO_MESSAGE_SIZE> (
                Address{0x00, 0x00, 0x22, 0x11},
                [&rxStage] (auto const &tm) {
                        if (rxStage == 0) {
                                ++rxStage;
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
                        }
                        if (rxStage == 1) {
                                ++rxStage;
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
                        }
                },
                [&txStage] (auto &&canFrame) {
                        if (txStage == 0) {
                                ++txStage;
                                REQUIRE (canFrame.data[0] == 0x30);
                                REQUIRE (canFrame.data[1] == 0x00);
                                REQUIRE (canFrame.data[2] == 0x00);
                        }
                        else if (txStage == 1) {
                                ++txStage;
                                REQUIRE (canFrame.data[0] == 0x10);
                                REQUIRE (canFrame.data[1] == 0x24);
                                REQUIRE (canFrame.data[2] == 0x83);
                                REQUIRE (canFrame.data[3] == 0xa3);
                                REQUIRE (canFrame.data[4] == 0x72);
                                REQUIRE (canFrame.data[5] == 0x73);
                                REQUIRE (canFrame.data[6] == 0x70);
                                REQUIRE (canFrame.data[7] == 0x01);
                        }
                        else if (txStage == 2) {
                                ++txStage;
                                REQUIRE (Vec (canFrame.data.cbegin (), canFrame.data.cend ())
                                         == Vec{0x21, 0xA3, 0x65, 0x72, 0x72, 0x02, 0xA3, 0x6D});
                        }
                        else if (txStage == 3) {
                                ++txStage;
                                REQUIRE (Vec (canFrame.data.cbegin (), canFrame.data.cend ())
                                         == Vec{0x22, 0x73, 0x67, 0xB4, 0x43, 0x6F, 0x6E, 0x6E});
                        }
                        else if (txStage == 4) {
                                ++txStage;
                                REQUIRE (Vec (canFrame.data.cbegin (), canFrame.data.cend ())
                                         == Vec{0x23, 0x65, 0x63, 0x74, 0x69, 0x6F, 0x6E, 0x20});
                        }
                        else if (txStage == 5) {
                                ++txStage;
                                REQUIRE (Vec (canFrame.data.cbegin (), canFrame.data.cend ())
                                         == Vec{0x24, 0x74, 0x69, 0x6D, 0x65, 0x64, 0x20, 0x6F});
                        }
                        else if (txStage == 6) {
                                ++txStage;

                                REQUIRE (canFrame.data[0] == 0x25);
                                REQUIRE (canFrame.data[1] == 0x75);
                                REQUIRE (canFrame.data[2] == 0x74);
                        }

                        return true;
                });

        // Request 1 (turn on the relay)
        txStage = 0;
        tp.onCanNewFrame (CanFrame (0x18DA2211, true, 0x10, 0x11, 0x83, 0xA3, 0x72, 0x65, 0x71, 0x01));
        // Here it responds with 18DA1122   [3]  30 00 01
        tp.onCanNewFrame (CanFrame (0x18DA2211, true, 0x21, 0xA4, 0x61, 0x64, 0x64, 0x72, 0x00, 0xA3));
        tp.onCanNewFrame (CanFrame (0x18DA2211, true, 0x22, 0x76, 0x61, 0x6C, 0x01));

        // Response 1 (Modbus connection timeout)
        tp.send ({0x83, 0xA3, 0x72, 0x73, 0x70, 0x01, 0xA3, 0x65, 0x72, 0x72, 0x02, 0xA3, 0x6D, 0x73, 0x67, 0xB4, 0x43, 0x6F,
                  0x6E, 0x6E, 0x65, 0x63, 0x74, 0x69, 0x6F, 0x6E, 0x20, 0x74, 0x69, 0x6D, 0x65, 0x64, 0x20, 0x6F, 0x75, 0x74});

        tp.run ();
        tp.run ();
        tp.onCanNewFrame (CanFrame (0x18DA2211, true, 0x30, 0x10, 0x00));

        while (tp.isSending ()) {
                tp.run ();
        }

        // Request 2 (turn off the relay)
        txStage = 0;
        tp.onCanNewFrame (CanFrame (0x18DA2211, true, 0x10, 0x11, 0x83, 0xA3, 0x72, 0x65, 0x71, 0x01));
        tp.onCanNewFrame (CanFrame (0x18DA2211, true, 0x21, 0xA4, 0x61, 0x64, 0x64, 0x72, 0x00, 0xA3));
        tp.onCanNewFrame (CanFrame (0x18DA2211, true, 0x22, 0x76, 0x61, 0x6C, 0x00));

        // Response 2 (Modbus connection timeout) - same as response 2
        tp.send ({0x83, 0xA3, 0x72, 0x73, 0x70, 0x01, 0xA3, 0x65, 0x72, 0x72, 0x02, 0xA3, 0x6D, 0x73, 0x67, 0xB4, 0x43, 0x6F,
                  0x6E, 0x6E, 0x65, 0x63, 0x74, 0x69, 0x6F, 0x6E, 0x20, 0x74, 0x69, 0x6D, 0x65, 0x64, 0x20, 0x6F, 0x75, 0x74});

        tp.run ();
        tp.run ();
        tp.onCanNewFrame (CanFrame (0x18DA2211, true, 0x30, 0x10, 0x00));

        while (tp.isSending ()) {
                tp.run ();
        }
}
