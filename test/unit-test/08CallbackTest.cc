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
 * Tests N_USData.indication
 */
TEST_CASE ("Advanced callback", "[callbacks]")
{
        bool called = false;

        auto tpR = create (
                Address (0x89, 0x67),
                [&called] (auto const &address, auto const &isoMessage, tp::Result result) {
                        called = true;
                        REQUIRE (isoMessage.size () == 1);
                        REQUIRE (isoMessage[0] == 0x55);
                        REQUIRE (result == Result::N_OK);
                        REQUIRE (address.getTxId () == 0x89);
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

TEST_CASE ("AdvancedMethod callback", "[callbacks]")
{
        bool called = false;

        class FullCallback {
        public:
                explicit FullCallback (bool &b) : called{b} {}

                void confirm (Address const &a, Result r) {}
                // void firstFrameIndication (Address const &a, uint16_t len) {}
                void indication (Address const &address, std::vector<uint8_t> const &isoMessage, Result result)
                {
                        called = true;
                        REQUIRE (isoMessage.size () == 1);
                        REQUIRE (isoMessage[0] == 0x55);
                        REQUIRE (result == Result::N_OK);
                        REQUIRE (address.getTxId () == 0x89);
                }

        private:
                bool &called;
        };

        auto tpR = create (Address (0x89, 0x67), FullCallback (called), [] (auto const & /* canFrame */) { return true; });

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

/**
 * Test M_USData.confirm (5.2.2)
 */
TEST_CASE ("confirm callback", "[callbacks]")
{
        /*
         * Successful transmission.
         */
        {
                int called = 0;

                class FullCallbackR {
                public:
                        explicit FullCallbackR (int &b) : called{b} {}

                        void indication (Address const &address, std::vector<uint8_t> const &isoMessage, Result result)
                        {
                                ++called;
                                REQUIRE (isoMessage.size () == 1);
                                REQUIRE (isoMessage[0] == 0x55);
                                REQUIRE (result == Result::N_OK);
                                REQUIRE (address.getTxId () == 0x89);
                        }

                private:
                        int &called;
                };

                auto tpR = create (Address (0x89, 0x67), FullCallbackR (called), [] (auto const & /* canFrame */) { return true; });

                class FullCallbackT {
                public:
                        explicit FullCallbackT (int &b) : called{b} {}

                        void confirm (Address const &address, Result result)
                        {
                                ++called;
                                REQUIRE (address.getTxId () == 0x89);
                                REQUIRE (result == Result::N_OK);
                        }

                        void indication (Address const & /* address */, std::vector<uint8_t> const & /* isoMessage */, Result /* result */) {}

                private:
                        int &called;
                };

                auto tpT = create (Address (0x67, 0x89), FullCallbackT (called), [&tpR] (auto const &canFrame) {
                        tpR.onCanNewFrame (canFrame);
                        return true;
                });

                tpT.send ({0x55});

                while (tpT.isSending ()) {
                        tpT.run ();
                }

                REQUIRE (called == 2);
        }

        /*
         * UNsuccessful transmission.
         */
        {
                int called = 0;

                class FullCallback {
                public:
                        explicit FullCallback (int &b) : called{b} {}

                        void confirm (Address const &address, Result result)
                        {
                                ++called;
                                REQUIRE (address.getTxId () == 0x89);
                                REQUIRE (result != Result::N_OK);
                        }

                        void indication (Address const & /* address */, std::vector<uint8_t> const & /* isoMessage */, Result /* result */)
                        {
                                ++called;
                        }

                private:
                        int &called;
                };

                auto tpR = create (
                        Address (0x89, 0x67), [] (auto /*iso*/) {}, [] (auto const & /* canFrame */) { return true; });

                auto tpT = create (Address (0x67, 0x89), FullCallback (called), [] (auto const & /* canFrame */) {
                        // An error occureed during CAN frame sending.
                        return false;
                });

                tpT.send ({0x55});

                while (tpT.isSending ()) {
                        tpT.run ();
                }

                REQUIRE (called == 1);
                REQUIRE (tpT.transportMessagesMap.empty ());
        }
}

TEST_CASE ("First Frame callback", "[callbacks]")
{
        std::vector<CanFrame> framesFromR;
        std::vector<CanFrame> framesFromT;

        int called = 0;

        class FullCallbackR {
        public:
                explicit FullCallbackR (int &b) : called{b} {}

                void indication (Address const &address, std::vector<uint8_t> const &isoMessage, Result result)
                {
                        ++called;
                        REQUIRE (isoMessage.size () == 11);
                        REQUIRE (isoMessage[0] == 1);
                        REQUIRE (result == Result::N_OK);
                        REQUIRE (address.getTxId () == 0x89);
                }

                void firstFrameIndication (Address const &address, uint16_t len)
                {
                        REQUIRE (address.getTxId () == 0x89);
                        called += len;
                }

        private:
                int &called;
        };

        auto tpR = create (Address (0x89, 0x67), FullCallbackR (called), [&framesFromR] (auto const &canFrame) {
                framesFromR.push_back (canFrame);
                return true;
        });

        auto tpT = create (
                Address (0x67, 0x89), [] (auto /* a */) {},
                [&framesFromT] (auto const &canFrame) {
                        framesFromT.push_back (canFrame);
                        return true;
                });

        // Has to be long enough to be split between first and consecutive frames
        tpT.send ({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});

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

        REQUIRE (called == 12);
}
