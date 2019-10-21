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
                REQUIRE (a->txId == 0x12);
        }

        {
                // Big numer address (max tha can fit)
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x7ff);
                cfw.setExtended (false);
                auto a = Normal11AddressEncoder::fromFrame (cfw);
                REQUIRE (a);
                REQUIRE (a->txId == 0x7ff);
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
                REQUIRE (a->txId == 0x12);
        }

        {
                // Big numer address (max tha can fit)
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x1FFFFFFF);
                cfw.setExtended (true);
                auto a = Normal29AddressEncoder::fromFrame (cfw);
                REQUIRE (a);
                REQUIRE (a->txId == 0x1FFFFFFF);
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

TEST_CASE ("NormalFixed29 encode", "[address]")
{
        {
                // Simple address
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                REQUIRE (NormalFixed29AddressEncoder::toFrame (
                        Address (0x12, 0x34, Address::MessageType::DIAGNOSTICS, Address::TargetAddressType::PHYSICAL), cfw));

                REQUIRE (cfw.getId () == ((0b110 << 26) | (218 << 16) | (0x34 << 8) | 0x12));
                REQUIRE (cfw.isExtended ());
        }

        {
                // Max
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                REQUIRE (NormalFixed29AddressEncoder::toFrame (
                        Address (0xfe, 0xff, Address::MessageType::DIAGNOSTICS, Address::TargetAddressType::PHYSICAL), cfw));

                REQUIRE (cfw.getId () == ((0b110 << 26) | (218 << 16) | (0xff << 8) | 0xfe));
                REQUIRE (cfw.isExtended ());
        }

        {
                // Too big
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                REQUIRE (!NormalFixed29AddressEncoder::toFrame (
                        Address (0xfe, 0x100, Address::MessageType::DIAGNOSTICS, Address::TargetAddressType::PHYSICAL), cfw));
        }
}

TEST_CASE ("NormalFixed29 decode", "[address]")
{
        {
                // Simple address (Physical)
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId ((0b110 << 26) | (218 << 16) | (0x34 << 8) | 0x12);
                cfw.setExtended (true);
                auto a = NormalFixed29AddressEncoder::fromFrame (cfw);
                REQUIRE (a);
                REQUIRE (a->rxId == 0x12);
                REQUIRE (a->txId == 0x34);
                REQUIRE (a->targetAddressType == Address::TargetAddressType::PHYSICAL);
        }

        {
                // Simple address (Functional)
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId ((0b110 << 26) | (219 << 16) | (0x34 << 8) | 0x12);
                cfw.setExtended (true);
                auto a = NormalFixed29AddressEncoder::fromFrame (cfw);
                REQUIRE (a);
                REQUIRE (a->rxId == 0x12);
                REQUIRE (a->txId == 0x34);
                REQUIRE (a->targetAddressType == Address::TargetAddressType::FUNCTIONAL);
        }

        {
                // Max
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId ((0b110 << 26) | (218 << 16) | (0xff << 8) | 0xfe);
                cfw.setExtended (true);
                auto a = NormalFixed29AddressEncoder::fromFrame (cfw);
                REQUIRE (a);
                REQUIRE (a->rxId == 0xfe);
                REQUIRE (a->txId == 0xff);
                REQUIRE (a->targetAddressType == Address::TargetAddressType::PHYSICAL);
        }

        {
                // malformed
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId ((0b100 << 26) | (218 << 16) | (0xff << 8) | 0xfe);
                cfw.setExtended (true);
                REQUIRE (!NormalFixed29AddressEncoder::fromFrame (cfw));
        }

        {
                // malformed 2
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId ((0b110 << 26) | (228 << 16) | (0xff << 8) | 0xfe);
                cfw.setExtended (true);
                REQUIRE (!NormalFixed29AddressEncoder::fromFrame (cfw));
        }

        {
                // 11 instead of 29
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId ((0b1010 << 26) | (218 << 16) | (0xff << 8) | 0xfe);
                cfw.setExtended (false);
                REQUIRE (!NormalFixed29AddressEncoder::fromFrame (cfw));
        }
}
