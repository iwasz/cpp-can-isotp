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
        MessageType messageType{};

        /// 5.3.2.4
        TargetAddressType targetAddressType{};
};

/****************************************************************************/

struct Normal11AddressEncoder {

        /**
         * Create an address from a received CAN frame. This is
         * the address which the remote party used to send the frame to us.
         */
        template <typename CanFrameWrapper> static std::optional<Address> fromFrame (CanFrameWrapper const &f)
        {
                if (!f.isExtended ()) {
                        return Address (0x00, f.getId ());
                }

                return {};
        }

        /**
         * Store address into a CAN frame. This is the address of the remote party we want the message to get to.
         */
        template <typename CanFrameWrapper> static void toFrame (Address const &a, CanFrameWrapper &f)
        {
                f.setId (a.remoteAddress);
                f.setExtended (false);
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
                if (f.isExtended ()) {
                        return Address (0x00, f.getId ());
                }

                return {};
        }

        /**
         * Store address into a CAN frame. This is the address of the remote party we want the message to get to.
         */
        template <typename CanFrameWrapper> static void toFrame (Address const &a, CanFrameWrapper &f)
        {
                f.setId (a.remoteAddress);
                f.setExtended (true);
        }
};

} // namespace tp
