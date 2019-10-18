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

        Address (uint32_t localAddress, uint32_t remoteAddress, MessageType mt = MessageType::DIAGNOSTICS,
                 TargetAddressType tat = TargetAddressType::PHYSICAL)
            : localAddress (localAddress), remoteAddress (remoteAddress), messageType (mt), targetAddressType (tat)
        {
        }

        // Type addressType{};

        /// 5.3.2.2 N_SA encodes the network sender address. Valid both for DIAGNOSTICS and REMOTE_DIAGNOSTICS.
        // uint8_t sourceAddress{};

        /// 5.3.2.3 N_TA encodes the network target address. Valid both for DIAGNOSTICS and REMOTE_DIAGNOSTICS.
        // uint8_t targetAddress{};

        /// 5.3.2.5 N_AE Network address extension.
        // uint8_t networkAddressExtension{};

        uint32_t localAddress{};
        uint32_t remoteAddress{};

        /// 5.3.1
        MessageType messageType{MessageType::DIAGNOSTICS};

        /// 5.3.2.4
        TargetAddressType targetAddressType{TargetAddressType::PHYSICAL};
};

/****************************************************************************/

struct Normal11AddressEncoder {

        static constexpr uint32_t MAX_N = 0x7ff;

        /**
         * Create an address from a received CAN frame. This is
         * the address which the remote party used to send the frame to us.
         */
        template <typename CanFrameWrapper> static std::optional<Address> fromFrame (CanFrameWrapper const &f)
        {
                auto fId = f.getId ();

                if (f.isExtended () || fId > MAX_N) {
                        return {};
                }

                return Address (0x00, fId);
        }

        /**
         * Store address into a CAN frame. This is the address of the remote party we want the message to get to.
         */
        template <typename CanFrameWrapper> static bool toFrame (Address const &a, CanFrameWrapper &f)
        {
                if (a.remoteAddress > MAX_N) {
                        return false;
                }

                f.setId (a.remoteAddress);
                f.setExtended (false);
                return true;
        }
};

/****************************************************************************/

struct Normal29AddressEncoder {

        static constexpr uint32_t MAX_N = 0x1FFFFFFF;

        /**
         * Create an address from a received CAN frame. This is
         * the address which the remote party used to send the frame to us.
         */
        template <typename CanFrameWrapper> static std::optional<Address> fromFrame (CanFrameWrapper const &f)
        {
                auto fId = f.getId ();

                if (!f.isExtended () || fId > MAX_N) {
                        return {};
                }
                return Address (0x00, fId);
        }

        /**
         * Store address into a CAN frame. This is the address of the remote party we want the message to get to.
         */
        template <typename CanFrameWrapper> static bool toFrame (Address const &a, CanFrameWrapper &f)
        {
                if (a.remoteAddress > MAX_N) {
                        return false;
                }

                f.setId (a.remoteAddress);
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

                if (!f.isExtended () || !bool (fId & NORMAL_FIXED_29_MASK)) {
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
                if (a.localAddress > MAX_N || a.remoteAddress > MAX_N) {
                        return false;
                }

                f.setId (NORMAL_FIXED_29_MASK | (a.targetAddressType == Address::TargetAddressType::FUNCTIONAL)
                                 ? (N_TATYPE_MASK)
                                 : (0) | a.remoteAddress << 15 | a.localAddress);

                f.setExtended (true);

                return true;
        }
};

} // namespace tp
