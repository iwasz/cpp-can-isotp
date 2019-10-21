/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "MiscTypes.h"
#include <array>
#include <cstdint>
#include <gsl/gsl>

namespace tp {

static constexpr uint32_t MAX_11_ID = 0x7ff;
static constexpr uint32_t MAX_29_ID = 0x1FFFFFFF;

/**
 * This is the address class which represents ISO TP addresses. It lives in
 * slihtly higher level of abstraction than addresses describved in the ISO
 * document (N_AI) as it use the same variables for all 7 address types (localAddress
 * and remoteAddress). This are "address encoders" which further convert this
 * "Address" to apropriate data inside Can Frames.
 */
struct Address {

        // enum class Type : uint8_t { NORMAL_11, NORMAL_29, FIXED_29, EXTENDED_11, EXTENDED_29, MIXED_11, MIXED_29 };

        /// 5.3.1 Mtype
        enum class MessageType : uint8_t {
                DIAGNOSTICS,       /// N_SA, N_TA, N_TAtype are used
                REMOTE_DIAGNOSTICS /// N_SA, N_TA, N_TAtype and N_AE are used
        };

        /// N_TAtype Network Target Address Type
        enum class TargetAddressType : uint8_t {
                PHYSICAL,  /// 1 to 1 communiaction supported for multiple and single frame communications
                FUNCTIONAL /// 1 to n communication (like broadcast?) is allowed only for single frame comms.
        };

        Address () = default;

        Address (uint32_t rxId, uint32_t txId, MessageType mt = MessageType::DIAGNOSTICS, TargetAddressType tat = TargetAddressType::PHYSICAL)
            : rxId (rxId), txId (txId), messageType (mt), targetAddressType (tat)
        {
        }

        Address (uint32_t rxId, uint32_t txId, uint8_t sourceAddress, uint8_t targetAddress, MessageType mt = MessageType::DIAGNOSTICS,
                 TargetAddressType tat = TargetAddressType::PHYSICAL)
            : rxId (rxId), txId (txId), sourceAddress (sourceAddress), targetAddress (targetAddress), messageType (mt), targetAddressType (tat)
        {
        }

        Address (uint32_t rxId, uint32_t txId, uint8_t sourceAddress, uint8_t targetAddress, uint8_t networkAddressExtension,
                 MessageType mt = MessageType::DIAGNOSTICS, TargetAddressType tat = TargetAddressType::PHYSICAL)
            : rxId (rxId),
              txId (txId),
              sourceAddress (sourceAddress),
              targetAddress (targetAddress),
              networkAddressExtension (networkAddressExtension),
              messageType (mt),
              targetAddressType (tat)
        {
        }

        uint32_t rxId{};
        uint32_t txId{};

        /// 5.3.2.2 N_SA encodes the network sender address. Valid both for DIAGNOSTICS and REMOTE_DIAGNOSTICS.
        uint8_t sourceAddress{};

        /// 5.3.2.3 N_TA encodes the network target address. Valid both for DIAGNOSTICS and REMOTE_DIAGNOSTICS.
        uint8_t targetAddress{};

        /// 5.3.2.5 N_AE Network address extension.
        uint8_t networkAddressExtension{};

        /// 5.3.1
        MessageType messageType{MessageType::DIAGNOSTICS};

        /// 5.3.2.4
        TargetAddressType targetAddressType{TargetAddressType::PHYSICAL};
};

/****************************************************************************/

struct Normal11AddressEncoder {

        /**
         * Create an address from a received CAN frame. This is
         * the address which the remote party used to send the frame to us.
         */
        template <typename CanFrameWrapper> static std::optional<Address> fromFrame (CanFrameWrapper const &f)
        {
                auto fId = f.getId ();

                if (f.isExtended () || fId > MAX_11_ID) {
                        return {};
                }

                return Address (0x00, fId);
        }

        /**
         * Store address into a CAN frame. This is the address of the remote party we want the message to get to.
         */
        template <typename CanFrameWrapper> static bool toFrame (Address const &a, CanFrameWrapper &f)
        {
                if (a.txId > MAX_11_ID) {
                        return false;
                }

                f.setId (a.txId);
                f.setExtended (false);
                return true;
        }
};

/****************************************************************************/

struct Normal29AddressEncoder {

        /**
         * Create an address from a received CAN frame. This is
         * the address which the remote party used to send the frame to us.
         */
        template <typename CanFrameWrapper> static std::optional<Address> fromFrame (CanFrameWrapper const &f)
        {
                auto fId = f.getId ();

                if (!f.isExtended () || fId > MAX_29_ID) {
                        return {};
                }
                return Address (0x00, fId);
        }

        /**
         * Store address into a CAN frame. This is the address of the remote party we want the message to get to.
         */
        template <typename CanFrameWrapper> static bool toFrame (Address const &a, CanFrameWrapper &f)
        {
                if (a.txId > MAX_29_ID) {
                        return false;
                }

                f.setId (a.txId);
                f.setExtended (true);
                return true;
        }
};

/****************************************************************************/

struct NormalFixed29AddressEncoder {

        static constexpr uint32_t NORMAL_FIXED_29_MASK = 0x018DA0000;
        static constexpr uint32_t N_TA_MASK = 0x00000ff00;
        static constexpr uint32_t N_TATYPE_MASK = 0x10000;
        static constexpr uint32_t N_SA_MASK = 0x0000000ff;
        static constexpr uint32_t MAX_N = 0xff; /// Maximum value that can be stored in either N_TA, N_SA.

        /**
         * Create an address from a received CAN frame. This is
         * the address which the remote party used to send the frame to us.
         */
        template <typename CanFrameWrapper> static std::optional<Address> fromFrame (CanFrameWrapper const &f)
        {
                auto fId = f.getId ();

                if (!f.isExtended () || bool ((fId & NORMAL_FIXED_29_MASK) != NORMAL_FIXED_29_MASK)) {
                        return {};
                }

                return Address (fId & N_SA_MASK, (fId & N_TA_MASK) >> 8, Address::MessageType::DIAGNOSTICS,
                                (bool (fId & N_TATYPE_MASK)) ? (Address::TargetAddressType::FUNCTIONAL)
                                                             : (Address::TargetAddressType::PHYSICAL));
        }

        /**
         * Store address into a CAN frame. This is the address of the remote party we want the message to get to.
         */
        template <typename CanFrameWrapper> static bool toFrame (Address const &a, CanFrameWrapper &f)
        {
                if (a.rxId > MAX_N || a.txId > MAX_N) {
                        return false;
                }

                f.setId (NORMAL_FIXED_29_MASK | ((a.targetAddressType == Address::TargetAddressType::FUNCTIONAL) ? (N_TATYPE_MASK) : (0))
                         | a.txId << 8 | a.rxId);

                f.setExtended (true);

                return true;
        }
};

/****************************************************************************/

struct Extended11AddressEncoder {

        static constexpr uint32_t MAX_TA = 0xff;

        template <typename CanFrameWrapper> static std::optional<Address> fromFrame (CanFrameWrapper const &f)
        {
                auto fId = f.getId ();

                if (f.isExtended () || fId > MAX_11_ID || f.getDlc () < 1) {
                        return {};
                }

                return Address (0x00, fId, 0x00, f.get (0));
        }

        template <typename CanFrameWrapper> static bool toFrame (Address const &a, CanFrameWrapper &f)
        {
                if (a.txId > MAX_11_ID) {
                        return false;
                }

                f.setId (a.txId);
                f.setExtended (false);

                if (f.getDlc () < 1) {
                        f.setDlc (1);
                }

                f.set (0, a.targetAddress);
                return true;
        }
};

struct Extended29AddressEncoder {

        static constexpr uint32_t MAX_TA = 0xff;

        template <typename CanFrameWrapper> static std::optional<Address> fromFrame (CanFrameWrapper const &f)
        {
                auto fId = f.getId ();

                if (!f.isExtended () || fId > MAX_29_ID || f.getDlc () < 1) {
                        return {};
                }

                return Address (0x00, fId, 0x00, f.get (0));
        }

        template <typename CanFrameWrapper> static bool toFrame (Address const &a, CanFrameWrapper &f)
        {
                if (a.txId > MAX_29_ID) {
                        return false;
                }

                f.setId (a.txId);
                f.setExtended (true);

                if (f.getDlc () < 1) {
                        f.setDlc (1);
                }

                f.set (0, a.targetAddress);
                return true;
        }
};

/**
 * Helper class
 */
template <typename T> struct AddressTraitsBase {

        static constexpr uint8_t N_PCI_OFSET = (T::USING_EXTENDED) ? (1) : (0);

        template <typename CFWrapper> static uint8_t getNPciByte (CFWrapper const &f) { return f.get (N_PCI_OFSET); }
        template <typename CFWrapper> static IsoNPduType getType (CFWrapper const &f) { return IsoNPduType ((getNPciByte (f) & 0xF0) >> 4); }

        /*---------------------------------------------------------------------------*/

        template <typename CFWrapper> static uint8_t getDataLengthS (CFWrapper const &f) { return getNPciByte (f) & 0x0f; }
        template <typename CFWrapper> static uint16_t getDataLengthF (CFWrapper const &f)
        {
                return ((getNPciByte (f) & 0x0f) << 8) | f.get (N_PCI_OFSET + 1);
        }
        template <typename CFWrapper> static uint8_t getSerialNumber (CFWrapper const &f) { return getNPciByte (f) & 0x0f; }
        template <typename CFWrapper> static FlowStatus getFlowStatus (CFWrapper const &f) { return FlowStatus (getNPciByte (f) & 0x0f); }

        // TODO. What these are for?
        // static uint8_t getBlockSize (CFWrapper const &f) { return f.get (N_PCI_OFSET) + 1); }
        // static uint8_t getSeparationTime (CFWrapper const &f) { return f.get (N_PCI_OFSET) + 2); }

        // template <typename CFWrapper> static uint8_t get (CFWrapper const &f, size_t i) { return gsl::at (f.data, i + N_PCI_OFSET); }
        // template <typename CFWrapper> static void set (CFWrapper const &f, size_t i, uint8_t b) { gsl::at (f.data, i + N_PCI_OFSET) = b; }
};

template <typename AddressEncoder> struct AddressTraits : public AddressTraitsBase<AddressTraits<AddressEncoder>> {
        static constexpr bool USING_EXTENDED = false;
};

template <> struct AddressTraits<Extended11AddressEncoder> : public AddressTraitsBase<AddressTraits<Extended11AddressEncoder>> {
        static constexpr bool USING_EXTENDED = true;
};

template <> struct AddressTraits<Extended29AddressEncoder> : public AddressTraitsBase<AddressTraits<Extended11AddressEncoder>> {
        static constexpr bool USING_EXTENDED = true;
};

} // namespace tp
