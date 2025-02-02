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
                REQUIRE (a->getTxId () == 0x12);
        }

        {
                // Big numer address (max tha can fit)
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x7ff);
                cfw.setExtended (false);
                auto a = Normal11AddressEncoder::fromFrame (cfw);
                REQUIRE (a);
                REQUIRE (a->getTxId () == 0x7ff);
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
                REQUIRE (a->getTxId () == 0x12);
        }

        {
                // Big numer address (max tha can fit)
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x1FFFFFFF);
                cfw.setExtended (true);
                auto a = Normal29AddressEncoder::fromFrame (cfw);
                REQUIRE (a);
                REQUIRE (a->getTxId () == 0x1FFFFFFF);
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
                        Address (0, 0, 0x12, 0x34, Address::MessageType::DIAGNOSTICS, Address::TargetAddressType::PHYSICAL), cfw));

                REQUIRE (cfw.getId () == ((0b110 << 26) | (218 << 16) | (0x34 << 8) | 0x12));
                REQUIRE (cfw.isExtended ());
        }

        {
                // Max
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                REQUIRE (NormalFixed29AddressEncoder::toFrame (
                        Address (0, 0, 0xfe, 0xff, Address::MessageType::DIAGNOSTICS, Address::TargetAddressType::PHYSICAL), cfw));

                REQUIRE (cfw.getId () == ((0b110 << 26) | (218 << 16) | (0xff << 8) | 0xfe));
                REQUIRE (cfw.isExtended ());
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
                REQUIRE (a->getSourceAddress () == 0x12);
                REQUIRE (a->getTargetAddress () == 0x34);
                REQUIRE (a->getTargetAddressType () == Address::TargetAddressType::PHYSICAL);
        }

        {
                // Simple address (Functional)
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId ((0b110 << 26) | (219 << 16) | (0x34 << 8) | 0x12);
                cfw.setExtended (true);
                auto a = NormalFixed29AddressEncoder::fromFrame (cfw);
                REQUIRE (a);
                REQUIRE (a->getSourceAddress () == 0x12);
                REQUIRE (a->getTargetAddress () == 0x34);
                REQUIRE (a->getTargetAddressType () == Address::TargetAddressType::FUNCTIONAL);
        }

        {
                // Max
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId ((0b110 << 26) | (218 << 16) | (0xff << 8) | 0xfe);
                cfw.setExtended (true);
                auto a = NormalFixed29AddressEncoder::fromFrame (cfw);
                REQUIRE (a);
                REQUIRE (a->getSourceAddress () == 0xfe);
                REQUIRE (a->getTargetAddress () == 0xff);
                REQUIRE (a->getTargetAddressType () == Address::TargetAddressType::PHYSICAL);
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
                // malformed 3
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0xffffffff);
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

TEST_CASE ("Extended11bits encode", "[address]")
{
        {
                // Simple address
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                REQUIRE (Extended11AddressEncoder::toFrame (Address (0x123, 0x456, 0x55, 0xaa), cfw));

                REQUIRE (cfw.getId () == 0x456);
                REQUIRE (cfw.getDlc () == 1);
                REQUIRE (cfw.get (0) == 0xaa);
                REQUIRE (!cfw.isExtended ());
        }

        {
                // Too big value
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                REQUIRE (!Extended11AddressEncoder::toFrame (Address (0x1123, 0x2456, 0x55, 0xaa), cfw));
        }
}

TEST_CASE ("Extended11bits decode", "[address]")
{
        {
                // Simple address
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x456);
                cfw.setExtended (false);
                cfw.setDlc (1);
                cfw.set (0, 0xaa);
                auto a = Extended11AddressEncoder::fromFrame (cfw);
                REQUIRE (a);
                REQUIRE (a->getTxId () == 0x456);
                REQUIRE (a->getTargetAddress () == 0xaa);
                REQUIRE (a->getTargetAddressType () == Address::TargetAddressType::PHYSICAL);
        }

        {
                // Too big value in ID
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x1456);
                cfw.setExtended (false);
                cfw.setDlc (1);
                cfw.set (0, 0xaa);
                auto a = Extended11AddressEncoder::fromFrame (cfw);
                REQUIRE (!a);
        }

        {
                // Extended frame instead of STD
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x456);
                cfw.setExtended (true);
                cfw.setDlc (1);
                cfw.set (0, 0xaa);
                auto a = Extended11AddressEncoder::fromFrame (cfw);
                REQUIRE (!a);
        }

        {
                // No data
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x456);
                cfw.setExtended (false);
                cfw.setDlc (0);
                auto a = Extended11AddressEncoder::fromFrame (cfw);
                REQUIRE (!a);
        }
}

TEST_CASE ("Extended29bits encode", "[address]")
{
        {
                // Simple address
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                REQUIRE (Extended29AddressEncoder::toFrame (Address (0x123456, 0x789abc, 0x55, 0xaa), cfw));

                REQUIRE (cfw.getId () == 0x789abc);
                REQUIRE (cfw.getDlc () == 1);
                REQUIRE (cfw.get (0) == 0xaa);
                REQUIRE (cfw.isExtended ());
        }

        {
                // Too big value
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                REQUIRE (!Extended29AddressEncoder::toFrame (Address (0xffffffff, 0xffffffff, 0x55, 0xaa), cfw));
        }
}

TEST_CASE ("Extended29bits decode", "[address]")
{
        {
                // Simple address
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x789abc);
                cfw.setExtended (true);
                cfw.setDlc (1);
                cfw.set (0, 0xaa);
                auto a = Extended29AddressEncoder::fromFrame (cfw);
                REQUIRE (a);
                REQUIRE (a->getTxId () == 0x789abc);
                REQUIRE (a->getTargetAddress () == 0xaa);
        }

        {
                // Too big value in ID
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0xffffffff);
                cfw.setExtended (true);
                cfw.setDlc (1);
                cfw.set (0, 0xaa);
                auto a = Extended29AddressEncoder::fromFrame (cfw);
                REQUIRE (!a);
        }

        {
                // STD frame instead of ext
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x789abc);
                cfw.setExtended (false);
                cfw.setDlc (1);
                cfw.set (0, 0xaa);
                auto a = Extended29AddressEncoder::fromFrame (cfw);
                REQUIRE (!a);
        }

        {
                // No data
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x789abc);
                cfw.setExtended (true);
                cfw.setDlc (0);
                auto a = Extended29AddressEncoder::fromFrame (cfw);
                REQUIRE (!a);
        }
}

TEST_CASE ("Mixed11bits encode", "[address]")
{
        {
                // Simple address
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                REQUIRE (Mixed11AddressEncoder::toFrame (Address (0x123, 0x456, 0, 0, 0xaa), cfw));

                REQUIRE (cfw.getId () == 0x456);
                REQUIRE (cfw.getDlc () == 1);
                REQUIRE (cfw.get (0) == 0xaa);
                REQUIRE (!cfw.isExtended ());
        }

        {
                // Too big value
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                REQUIRE (!Mixed11AddressEncoder::toFrame (Address (0xffffffff, 0xffffffff, 0, 0, 0xaa), cfw));
        }
}

TEST_CASE ("Mixed11bits decode", "[address]")
{
        {
                // Simple address
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x456);
                cfw.setExtended (false);
                cfw.setDlc (1);
                cfw.set (0, 0xaa);
                auto a = Mixed11AddressEncoder::fromFrame (cfw);
                REQUIRE (a);
                REQUIRE (a->getTxId () == 0x456);
                REQUIRE (a->getNetworkAddressExtension () == 0xaa);
                REQUIRE (a->getTargetAddressType () == Address::TargetAddressType::PHYSICAL);
        }

        {
                // Too big value in ID
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x1456);
                cfw.setExtended (false);
                cfw.setDlc (1);
                cfw.set (0, 0xaa);
                auto a = Mixed11AddressEncoder::fromFrame (cfw);
                REQUIRE (!a);
        }

        {
                // Extended frame instead of STD
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x456);
                cfw.setExtended (true);
                cfw.setDlc (1);
                cfw.set (0, 0xaa);
                auto a = Mixed11AddressEncoder::fromFrame (cfw);
                REQUIRE (!a);
        }

        {
                // No data
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0x456);
                cfw.setExtended (false);
                cfw.setDlc (0);
                auto a = Mixed11AddressEncoder::fromFrame (cfw);
                REQUIRE (!a);
        }
}

TEST_CASE ("Mixed29bits encode", "[address]")
{
        {
                // Simple address
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                REQUIRE (Mixed29AddressEncoder::toFrame (
                        Address (0, 0, 0x55, 0xaa, 0x77, tp::Address::MessageType::REMOTE_DIAGNOSTICS, tp::Address::TargetAddressType::PHYSICAL),
                        cfw));

                REQUIRE (cfw.getId () == ((0b110 << 26) | (206 << 16) | (0xaa << 8) | 0x55));
                REQUIRE (cfw.getDlc () == 1);
                REQUIRE (cfw.get (0) == 0x77);
                REQUIRE (cfw.isExtended ());
        }

        {
                // Simple address
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                REQUIRE (Mixed29AddressEncoder::toFrame (Address (0, 0, 0x55, 0xaa, 0x77, tp::Address::MessageType::REMOTE_DIAGNOSTICS,
                                                                  tp::Address::TargetAddressType::FUNCTIONAL),
                                                         cfw));

                REQUIRE (cfw.getId () == ((0b110 << 26) | (205 << 16) | (0xaa << 8) | 0x55));
                REQUIRE (cfw.getDlc () == 1);
                REQUIRE (cfw.get (0) == 0x77);
                REQUIRE (cfw.isExtended ());
        }
}

TEST_CASE ("Mixed29bits decode", "[address]")
{
        {
                // Simple address (physical)_
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (((0b110 << 26) | (206 << 16) | (0xaa << 8) | 0x55));
                cfw.setExtended (true);
                cfw.setDlc (1);
                cfw.set (0, 0x77);
                auto a = Mixed29AddressEncoder::fromFrame (cfw);
                REQUIRE (a);
                REQUIRE (a->getSourceAddress () == 0x55);
                REQUIRE (a->getTargetAddress () == 0xaa);
                REQUIRE (a->getNetworkAddressExtension () == 0x77);
                REQUIRE (a->getMessageType () == tp::Address::MessageType::REMOTE_DIAGNOSTICS);
                REQUIRE (a->getTargetAddressType () == tp::Address::TargetAddressType::PHYSICAL);
        }

        {
                // Simple address (functional)_
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (((0b110 << 26) | (205 << 16) | (0xaa << 8) | 0x55));
                cfw.setExtended (true);
                cfw.setDlc (1);
                cfw.set (0, 0x77);
                auto a = Mixed29AddressEncoder::fromFrame (cfw);
                REQUIRE (a);
                REQUIRE (a->getSourceAddress () == 0x55);
                REQUIRE (a->getTargetAddress () == 0xaa);
                REQUIRE (a->getNetworkAddressExtension () == 0x77);
                REQUIRE (a->getMessageType () == tp::Address::MessageType::REMOTE_DIAGNOSTICS);
                REQUIRE (a->getTargetAddressType () == tp::Address::TargetAddressType::FUNCTIONAL);
        }

        {
                // Malformwed ID
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (0xffffffff);
                cfw.setExtended (true);
                cfw.setDlc (1);
                cfw.set (0, 0xaa);
                auto a = Mixed29AddressEncoder::fromFrame (cfw);
                REQUIRE (!a);
        }

        {
                // STD frame instead of ext
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (((0b110 << 26) | (205 << 16) | (0xaa << 8) | 0x55));
                cfw.setExtended (false);
                cfw.setDlc (1);
                cfw.set (0, 0xaa);
                auto a = Mixed29AddressEncoder::fromFrame (cfw);
                REQUIRE (!a);
        }

        {
                // No data
                CanFrameWrapper<CanFrame> cfw{CanFrame ()};
                cfw.setId (((0b110 << 26) | (205 << 16) | (0xaa << 8) | 0x55));
                cfw.setExtended (true);
                cfw.setDlc (0);
                auto a = Mixed29AddressEncoder::fromFrame (cfw);
                REQUIRE (!a);
        }
}
