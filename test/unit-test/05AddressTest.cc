/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "TransportProtocol.h"
#include "catch.hpp"

using namespace tp;

TEST_CASE ("Normal11 encode", "[address]")
{
        {
                // Simple address
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                REQUIRE (Normal11AddressEncoder::toFrame (Address (0x12, 0x34), cfw));
                REQUIRE (cfw.getId () == 0x34);
                REQUIRE (!cfw.isExtended ());
        }

        {
                // Maximu value that can be stored into STD CAN frame id
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                REQUIRE (Normal11AddressEncoder::toFrame (Address (0x7fe, 0x7ff), cfw));
                REQUIRE (cfw.getId () == 0x7ff);
                REQUIRE (!cfw.isExtended ());
        }

        {
                // Value is too big to be stored, thus error has to be risen.
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                REQUIRE (!Normal11AddressEncoder::toFrame (Address (0x00, 0x800), cfw));
        }
}

TEST_CASE ("Normal11 decode", "[address]")
{
        {
                // Simple address
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x12);
                cfw.setExtended (false);
                auto a = Normal11AddressEncoder::fromFrame (cfw);
                REQUIRE (a);
                REQUIRE (a->remoteAddress == 0x12);
        }

        {
                // Big numer address (max tha can fit)
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x7ff);
                cfw.setExtended (false);
                auto a = Normal11AddressEncoder::fromFrame (cfw);
                REQUIRE (a);
                REQUIRE (a->remoteAddress == 0x7ff);
        }

        {
                // Too big value stored in the CAN ID somehow (it shouln'd be since STD ID is only 11 bits long).
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x800);
                cfw.setExtended (false);
                auto a = Normal11AddressEncoder::fromFrame (cfw);
                REQUIRE (!a);
        }

        {
                // Somebody tried to decode use 11 bit decoder on 29 bit frame -> fail.
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x12);
                cfw.setExtended (true);
                auto a = Normal11AddressEncoder::fromFrame (cfw);
                REQUIRE (!a);
        }
}

TEST_CASE ("Normal29 encode", "[address]")
{
        {
                // Simple address
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                REQUIRE (Normal29AddressEncoder::toFrame (Address (0x12, 0x34), cfw));
                REQUIRE (cfw.getId () == 0x34);
                REQUIRE (cfw.isExtended ());
        }

        {
                // Maximu value that can be stored into STD CAN frame id
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                REQUIRE (Normal29AddressEncoder::toFrame (Address (0x1FFFFFFe, 0x1FFFFFFF), cfw));
                REQUIRE (cfw.getId () == 0x1FFFFFFF);
                REQUIRE (cfw.isExtended ());
        }

        {
                // Value is too big to be stored, thus error has to be risen.
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                REQUIRE (!Normal29AddressEncoder::toFrame (Address (0x00, 0x20000000), cfw));
        }
}

TEST_CASE ("Normal29 decode", "[address]")
{
        {
                // Simple address
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x12);
                cfw.setExtended (true);
                auto a = Normal29AddressEncoder::fromFrame (cfw);
                REQUIRE (a);
                REQUIRE (a->remoteAddress == 0x12);
        }

        {
                // Big numer address (max tha can fit)
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x1FFFFFFF);
                cfw.setExtended (true);
                auto a = Normal29AddressEncoder::fromFrame (cfw);
                REQUIRE (a);
                REQUIRE (a->remoteAddress == 0x1FFFFFFF);
        }

        {
                // Too big value stored in the CAN ID somehow (it shouln'd be since STD ID is only 11 bits long).
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x20000000);
                cfw.setExtended (true);
                auto a = Normal29AddressEncoder::fromFrame (cfw);
                REQUIRE (!a);
        }

        {
                // Somebody tried to decode use 11 bit decoder on 29 bit frame -> fail.
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x12);
                cfw.setExtended (false);
                auto a = Normal29AddressEncoder::fromFrame (cfw);
                REQUIRE (!a);
        }
}