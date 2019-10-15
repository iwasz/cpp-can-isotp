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

TEST_CASE ("tx 1B", "[transmission]")
{
        bool called = false;
        auto tp = create ([] (auto &&) {},
                          [&called] (auto &&canFrame) {
                                  called = true;
                                  REQUIRE (canFrame.data[0] == 0x01); // SINGLE FRAME, 1B length
                                  REQUIRE (canFrame.data[1] == 0x55); // actual data
                                  REQUIRE (canFrame.id == 0x67);
                                  return true;
                          });

        tp.send (normal29Address (0x67, 0x89), {0x55});
        REQUIRE (called);
}

TEST_CASE ("tx 7B", "[transmission]")
{
        bool called = false;
        auto tp = create ([] (auto &&) {},
                          [&called] (auto &&canFrame) {
                                  called = true;
                                  REQUIRE (canFrame.data[0] == 0x07); // SINGLE FRAME, 7B length
                                  REQUIRE (canFrame.data[1] == 0);    // actual data
                                  REQUIRE (canFrame.data[2] == 1);
                                  REQUIRE (canFrame.data[3] == 2);
                                  REQUIRE (canFrame.data[4] == 3);
                                  REQUIRE (canFrame.data[5] == 4);
                                  REQUIRE (canFrame.data[6] == 5);
                                  REQUIRE (canFrame.data[7] == 6);
                                  return true;
                          });

        tp.send (normal29Address (0x00, 0x00), {0, 1, 2, 3, 4, 5, 6});
        REQUIRE (called);
}

/****************************************************************************/

TEST_CASE ("tx 8B", "[transmission]")
{
        int calledTimes = 0;
        auto tp = create ([] (auto &&) {},
                          [&calledTimes] (auto &&canFrame) {
                                  // 1. It should send a first frame
                                  if (calledTimes == 0) {
                                          REQUIRE (int (canFrame.data[0]) == 0x10); // FISRT FRAME
                                          REQUIRE (int (canFrame.data[1]) == 8);    // 8B length
                                          REQUIRE (int (canFrame.data[2]) == 0);    // First byte
                                          REQUIRE (int (canFrame.data[3]) == 1);
                                          REQUIRE (int (canFrame.data[4]) == 2);
                                          REQUIRE (int (canFrame.data[5]) == 3);
                                          REQUIRE (int (canFrame.data[6]) == 4);
                                          REQUIRE (int (canFrame.data[7]) == 5);
                                          ++calledTimes;
                                          return true;
                                  }

                                  // 1. It should send a first frame
                                  if (calledTimes == 1) {
                                          REQUIRE (int (canFrame.data[0]) == 0x21); // CONSECUTIVE FRAME, serial number 1
                                          REQUIRE (int (canFrame.data[1]) == 6);    // actual data - 7th byte
                                          REQUIRE (int (canFrame.data[2]) == 7);    // actual data - last, eight byte
                                          ++calledTimes;
                                          return true;
                                  }

                                  return false;
                          });

        tp.send (normal29Address (0x00, 0x00), {0, 1, 2, 3, 4, 5, 6, 7});
        tp.run ();                                      // IDLE -> SEND_FIRST_FRAME
        tp.run ();                                      // SEND_FIRST_FRAME -> RECEIVE_FLOW_FRAME
        tp.onCanNewFrame (CanFrame (0x00, true, 0x30)); // RECEIVE_FLOW_FRAME -> SEND_CONSECUTIVE_FRAME
        tp.run ();                                      // SEND_CONSECUTIVE_FRAME -> DONE

        while (tp.isSending ()) {
                tp.run ();
        }

        REQUIRE (calledTimes == 2);
}

/****************************************************************************/

TEST_CASE ("tx 12B", "[transmission]")
{
        int calledTimes = 0;
        auto tp = create ([] (auto &&) {},
                          [&calledTimes] (auto &&canFrame) {
                                  // 1. It should send a first frame
                                  if (calledTimes == 0) {
                                          REQUIRE (int (canFrame.data[0]) == 0x10); // FISRT FRAME
                                          REQUIRE (int (canFrame.data[1]) == 13);   // 13B length
                                          REQUIRE (int (canFrame.data[2]) == 0);    // First byte
                                          REQUIRE (int (canFrame.data[3]) == 1);
                                          REQUIRE (int (canFrame.data[4]) == 2);
                                          REQUIRE (int (canFrame.data[5]) == 3);
                                          REQUIRE (int (canFrame.data[6]) == 4);
                                          REQUIRE (int (canFrame.data[7]) == 5);
                                          ++calledTimes;
                                          return true;
                                  }

                                  // 1. It should send a first frame
                                  if (calledTimes == 1) {
                                          REQUIRE (int (canFrame.data[0]) == 0x21); // CONSECUTIVE FRAME, serial number 1
                                          REQUIRE (int (canFrame.data[1]) == 6);
                                          REQUIRE (int (canFrame.data[2]) == 7);
                                          REQUIRE (int (canFrame.data[3]) == 8);
                                          REQUIRE (int (canFrame.data[4]) == 9);
                                          REQUIRE (int (canFrame.data[5]) == 10);
                                          REQUIRE (int (canFrame.data[6]) == 11);
                                          REQUIRE (int (canFrame.data[7]) == 12);
                                          ++calledTimes;
                                          return true;
                                  }

                                  return false;
                          });

        tp.send (normal29Address (0x00, 0x00), {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
        tp.run ();                                      // IDLE -> SEND_FIRST_FRAME
        tp.run ();                                      // SEND_FIRST_FRAME -> RECEIVE_FLOW_FRAME
        tp.onCanNewFrame (CanFrame (0x00, true, 0x30)); // RECEIVE_FLOW_FRAME -> SEND_CONSECUTIVE_FRAME
        tp.run ();                                      // SEND_CONSECUTIVE_FRAME -> DONE

        while (tp.isSending ()) {
                tp.run ();
        }

        REQUIRE (calledTimes == 2);
}

TEST_CASE ("tx 4095B", "[transmission]")
{
        int calledTimes = 0;
        auto tp = create ([] (auto &&) {},
                          [&calledTimes] (auto &&canFrame) {
                                  if (calledTimes == 0) {
                                          REQUIRE (int (canFrame.dlc) == 8);
                                          REQUIRE (int (canFrame.data[0]) == 0x1f); // FISRT FRAME
                                          REQUIRE (int (canFrame.data[1]) == 0xff); // 13B length
                                          REQUIRE (int (canFrame.data[2]) == 0);    // First byte
                                          REQUIRE (int (canFrame.data[3]) == 1);
                                          REQUIRE (int (canFrame.data[4]) == 2);
                                          REQUIRE (int (canFrame.data[5]) == 3);
                                          REQUIRE (int (canFrame.data[6]) == 4);
                                          REQUIRE (int (canFrame.data[7]) == 5);
                                  }

                                  if (calledTimes == 1) {
                                          REQUIRE (int (canFrame.dlc) == 8);
                                          REQUIRE (int (canFrame.data[0]) == 0x21); // CONSECUTIVE FRAME, serial number 1
                                          REQUIRE (int (canFrame.data[1]) == 6);
                                          REQUIRE (int (canFrame.data[2]) == 7);
                                          REQUIRE (int (canFrame.data[3]) == 8);
                                          REQUIRE (int (canFrame.data[4]) == 9);
                                          REQUIRE (int (canFrame.data[5]) == 10);
                                          REQUIRE (int (canFrame.data[6]) == 11);
                                          REQUIRE (int (canFrame.data[7]) == 12);
                                  }

                                  if (calledTimes == 2) {
                                          REQUIRE (int (canFrame.dlc) == 8);
                                          REQUIRE (int (canFrame.data[0]) == 0x22);
                                          REQUIRE (int (canFrame.data[1]) == 13);
                                          REQUIRE (int (canFrame.data[2]) == 14);
                                          REQUIRE (int (canFrame.data[3]) == 15);
                                          REQUIRE (int (canFrame.data[4]) == 16);
                                          REQUIRE (int (canFrame.data[5]) == 17);
                                          REQUIRE (int (canFrame.data[6]) == 18);
                                          REQUIRE (int (canFrame.data[7]) == 19);
                                  }

                                  //...

                                  if (calledTimes == 15) {
                                          REQUIRE (int (canFrame.dlc) == 8);
                                          REQUIRE (int (canFrame.data[0]) == 0x2f);
                                          REQUIRE (int (canFrame.data[1]) == 104);
                                          REQUIRE (int (canFrame.data[2]) == 105);
                                          REQUIRE (int (canFrame.data[3]) == 106);
                                          REQUIRE (int (canFrame.data[4]) == 107);
                                          REQUIRE (int (canFrame.data[5]) == 108);
                                          REQUIRE (int (canFrame.data[6]) == 109);
                                          REQUIRE (int (canFrame.data[7]) == 110);
                                  }

                                  // ...

                                  if (calledTimes == 31) {
                                          REQUIRE (int (canFrame.dlc) == 8);
                                          REQUIRE (int (canFrame.data[0]) == 0x2f);
                                          REQUIRE (int (canFrame.data[1]) == 216);
                                          REQUIRE (int (canFrame.data[2]) == 217);
                                          REQUIRE (int (canFrame.data[3]) == 218);
                                          REQUIRE (int (canFrame.data[4]) == 219);
                                          REQUIRE (int (canFrame.data[5]) == 220);
                                          REQUIRE (int (canFrame.data[6]) == 221);
                                          REQUIRE (int (canFrame.data[7]) == 222);
                                  }

                                  // ...

                                  if (calledTimes == 585) { // last CAN frame to send
                                          REQUIRE (int (canFrame.data[0]) == 0x29);
                                          REQUIRE (int (canFrame.data[1]) == 254);
                                          REQUIRE (int (canFrame.dlc) == 2);
                                  }

                                  ++calledTimes;
                                  return true;
                          });

        std::vector<uint8_t> message (4095);
        uint8_t v = 0;

        for (uint8_t &b : message) {
                b = v++;
        }

        tp.send (normal29Address (0x00, 0x00), message);
        tp.run ();                                                  // IDLE -> SEND_FIRST_FRAME
        tp.run ();                                                  // SEND_FIRST_FRAME -> RECEIVE_FLOW_FRAME
        tp.onCanNewFrame (CanFrame (0x00, true, 0x30, 0x00, 0x00)); // RECEIVE_FLOW_FRAME -> SEND_CONSECUTIVE_FRAME
        tp.run ();                                                  // SEND_CONSECUTIVE_FRAME -> more consecutive frames

        while (tp.isSending ()) {
                tp.run ();
        }

        REQUIRE (calledTimes == 586);
}

TEST_CASE ("tx 4096B", "[transmission]")
{
        auto tp = create ([] (auto &&) {});
        REQUIRE (tp.send (normal29Address (0x67, 0x89), std::vector<uint8_t> (4096)) == false);
}
