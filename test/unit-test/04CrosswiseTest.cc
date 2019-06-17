/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "Iso15765TransportProtocol.h"
#include "catch.hpp"
#include <vector>

using namespace tp;

// TEST_CASE ("cross half-duplex 1B", "[crosswise]")
//{
//        bool called = false;
//        auto tpR = create (
//                [&called](auto &&isoMessage) {
//                        called = true;
//                        REQUIRE (isoMessage.size () == 1);
//                        REQUIRE (isoMessage[0] == 0x55);
//                },
//                [](auto &&canFrame) { return true; });

//        auto tpT = create ([](auto &&) {},
//                           [&tpR](auto &&canFrame) {
//                                   tpR.onCanNewFrame (canFrame);
//                                   return true;
//                           });

//        tpT.send ({ 0x89 }, { 0x55 });

//        while (tpT.isSending ()) {
//                tpT.run ();
//        }
//}

struct ICrossroads {

        virtual ~ICrossroads () = default;
        virtual void onCanFrame2R (CanFrame const &) = 0;
        virtual void onCanFrame2T (CanFrame const &) = 0;
};

template <typename TR, typename TT> struct Crossroads : public ICrossroads {
        Crossroads (TR &tr, TT &tt) : tr (tr), tt (tt) {}

        virtual ~Crossroads () = default;
        void onCanFrame2R (CanFrame const &f) { tr.onCanNewFrame (f); }
        void onCanFrame2T (CanFrame const &f) { tt.onCanNewFrame (f); }

private:
        TR &tr;
        TT &tt;
};

// TEST_CASE ("cross half-duplex 16B", "[crosswise]")
//{
//        ICrossroads *crossroads = nullptr;
//        bool called = false;
//        auto tpR = create (
//                [&called](auto &&isoMessage) {
//                        called = true;
//                        REQUIRE (isoMessage.size () == 16);
//                        REQUIRE (isoMessage[0] == 0);
//                        REQUIRE (isoMessage[1] == 1);
//                        REQUIRE (isoMessage[2] == 2);
//                        REQUIRE (isoMessage[3] == 3);
//                        REQUIRE (isoMessage[4] == 4);
//                        REQUIRE (isoMessage[5] == 5);
//                        REQUIRE (isoMessage[6] == 6);
//                        REQUIRE (isoMessage[7] == 7);
//                        REQUIRE (isoMessage[8] == 8);
//                        REQUIRE (isoMessage[9] == 9);
//                        REQUIRE (isoMessage[10] == 10);
//                        REQUIRE (isoMessage[11] == 11);
//                        REQUIRE (isoMessage[12] == 12);
//                        REQUIRE (isoMessage[13] == 13);
//                        REQUIRE (isoMessage[14] == 14);
//                        REQUIRE (isoMessage[15] == 15);
//                },
//                [&crossroads](auto &&canFrame) {
//                        crossroads->onCanFrame2T (canFrame);
//                        return true;
//                });

//        auto tpT = create ([](auto &&) {},
//                           [&crossroads](auto &&canFrame) {
//                                   crossroads->onCanFrame2R (canFrame);
//                                   return true;
//                           });

//        Crossroads cr{ tpR, tpT };
//        crossroads = &cr;

//        tpT.send ({ 0x89 }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });

//        while (tpT.isSending ()) {
//                tpT.run ();
//                //tpR.run ();
//        }
//}

TEST_CASE ("cross half-duplex 16B", "[crosswise]")
{
        std::vector<CanFrame> framesFromR;
        std::vector<CanFrame> framesFromT;

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
                [&framesFromR](auto &&canFrame) {
                        // crossroads->onCanFrame2T (canFrame);
                        framesFromR.push_back (canFrame);
                        return true;
                });

        // Target address is 0x12, source address is 0x89.
        tpR.setDefaultAddress (Address{ 0x12, 0x89 });

        auto tpT = create ([](auto &&) {},
                           [&framesFromT](auto &&canFrame) {
                                   // crossroads->onCanFrame2R (canFrame);
                                   framesFromT.push_back (canFrame);
                                   return true;
                           });

        // Target address is 0x89, source address is 0x12. We expect responses (FCs) with 0x12 address.
        tpT.send (Address{ 0x89, 0x12 }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });

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
}
