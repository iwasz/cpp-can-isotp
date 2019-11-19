/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "MiscTypes.h"

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

        uint32_t getRxId () const { return this->rxId; }
        void setRxId (uint32_t rxId) { this->rxId = rxId; }

        uint32_t getTxId () const { return this->txId; }
        void setTxId (uint32_t txId) { this->txId = txId; }

        uint8_t getSourceAddress () const { return this->sourceAddress; }
        void setSourceAddress (uint8_t sourceAddress) { this->sourceAddress = sourceAddress; }

        uint8_t getTargetAddress () const { return this->targetAddress; }
        void setTargetAddress (uint8_t targetAddress) { this->targetAddress = targetAddress; }

        uint8_t getNetworkAddressExtension () const { return this->networkAddressExtension; }
        void setNetworkAddressExtension (uint8_t networkAddressExtension) { this->networkAddressExtension = networkAddressExtension; }

        MessageType getMessageType () const { return this->messageType; }
        void setMessageType (MessageType messageType) { this->messageType = messageType; }

        TargetAddressType getTargetAddressType () const { return this->targetAddressType; }
        void setTargetAddressType (TargetAddressType targetAddressType) { this->targetAddressType = targetAddressType; }

private:
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
}; // namespace tp

inline bool operator< (Address const &a, Address const &b)
{
        return a.getTxId () < b.getTxId () && a.getTargetAddress () < b.getTargetAddress ()
                && a.getNetworkAddressExtension () < b.getNetworkAddressExtension ();
}

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
                if (a.getTxId () > MAX_11_ID) {
                        return false;
                }

                f.setId (a.getTxId ());
                f.setExtended (false);
                return true;
        }

        /// Implements address matching for this type of addressing.
        static bool matches (Address const &theirs, Address const &ours) { return theirs.getTxId () == ours.getRxId (); }
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
                if (a.getTxId () > MAX_29_ID) {
                        return false;
                }

                f.setId (a.getTxId ());
                f.setExtended (true);
                return true;
        }

        /// Implements address matching for this type of addressing.
        static bool matches (Address const &theirs, Address const &ours) { return theirs.getTxId () == ours.getRxId (); }
};

/****************************************************************************/

struct NormalFixed29AddressEncoder {

        static constexpr uint32_t NORMAL_FIXED_29 = 0x018DA0000;
        static constexpr uint32_t NORMAL_FIXED_29_MASK = 0x1FFE0000;
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

                if (!f.isExtended () || bool ((fId & NORMAL_FIXED_29_MASK) != NORMAL_FIXED_29)) {
                        return {};
                }

                return Address (0x00, 0x00, fId & N_SA_MASK, (fId & N_TA_MASK) >> 8, Address::MessageType::DIAGNOSTICS,
                                (bool (fId & N_TATYPE_MASK)) ? (Address::TargetAddressType::FUNCTIONAL)
                                                             : (Address::TargetAddressType::PHYSICAL));
        }

        /**
         * Store address into a CAN frame. This is the address of the remote party we want the message to get to.
         */
        template <typename CanFrameWrapper> static bool toFrame (Address const &a, CanFrameWrapper &f)
        {
                f.setId (NORMAL_FIXED_29 | ((a.getTargetAddressType () == Address::TargetAddressType::FUNCTIONAL) ? (N_TATYPE_MASK) : (0))
                         | a.getTargetAddress () << 8 | a.getSourceAddress ());

                f.setExtended (true);

                return true;
        }

        /// Implements address matching for this type of addressing.
        static bool matches (Address const &theirs, Address const &ours) { return theirs.getTargetAddress () == ours.getSourceAddress (); }
};

/****************************************************************************/

struct Extended11AddressEncoder {

        static constexpr uint32_t MAX_TA = 0xff;

        template <typename CanFrameWrapper> static std::optional<Address> fromFrame (CanFrameWrapper const &f)
        {
                auto fId = f.getId ();

                if (fId > MAX_11_ID) {
                        return {};
                }

                if (f.isExtended () || fId > MAX_11_ID || f.getDlc () < 1) {
                        return {};
                }

                return Address (0x00, fId, 0x00, f.get (0));
        }

        template <typename CanFrameWrapper> static bool toFrame (Address const &a, CanFrameWrapper &f)
        {
                if (a.getTxId () > MAX_11_ID) {
                        return false;
                }

                f.setId (a.getTxId ());
                f.setExtended (false);

                if (f.getDlc () < 1) {
                        f.setDlc (1);
                }

                f.set (0, a.getTargetAddress ());
                return true;
        }

        /// Implements address matching for this type of addressing.
        static bool matches (Address const &theirs, Address const &ours)
        {
                return theirs.getTxId () == ours.getRxId () && theirs.getTargetAddress () == ours.getSourceAddress ();
        }
};

/****************************************************************************/

struct Extended29AddressEncoder {

        static constexpr uint32_t MAX_TA = 0xff;

        template <typename CanFrameWrapper> static std::optional<Address> fromFrame (CanFrameWrapper const &f)
        {
                auto fId = f.getId ();

                if (fId > MAX_29_ID) {
                        return {};
                }

                if (!f.isExtended () || fId > MAX_29_ID || f.getDlc () < 1) {
                        return {};
                }

                return Address (0x00, fId, 0x00, f.get (0));
        }

        template <typename CanFrameWrapper> static bool toFrame (Address const &a, CanFrameWrapper &f)
        {
                if (a.getTxId () > MAX_29_ID) {
                        return false;
                }

                f.setId (a.getTxId ());
                f.setExtended (true);

                if (f.getDlc () < 1) {
                        f.setDlc (1);
                }

                f.set (0, a.getTargetAddress ());
                return true;
        }

        /// Implements address matching for this type of addressing.
        static bool matches (Address const &theirs, Address const &ours)
        {
                return theirs.getTxId () == ours.getRxId () && theirs.getTargetAddress () == ours.getSourceAddress ();
        }
};

/****************************************************************************/

struct Mixed11AddressEncoder {

        /**
         * Create an address from a received CAN frame. This is
         * the address which the remote party used to send the frame to us.
         */
        template <typename CanFrameWrapper> static std::optional<Address> fromFrame (CanFrameWrapper const &f)
        {
                if (f.isExtended () || f.getDlc () < 1 || f.getId () > MAX_11_ID) {
                        return {};
                }

                return Address (0x00, f.getId (), 0x00, 0x00, f.get (0), Address::MessageType::REMOTE_DIAGNOSTICS);
        }

        /**
         * Store address into a CAN frame. This is the address of the remote party we want the message to get to.
         */
        template <typename CanFrameWrapper> static bool toFrame (Address const &a, CanFrameWrapper &f)
        {
                // if (a.messageType != Address::MessageType::REMOTE_DIAGNOSTICS) {
                //         return false;
                // }
                if (a.getTxId () > MAX_11_ID) {
                        return false;
                }

                f.setId (a.getTxId ());

                if (f.getDlc () < 1) {
                        f.setDlc (1);
                }

                f.set (0, a.getNetworkAddressExtension ());
                f.setExtended (false);
                return true;
        }

        /// Implements address matching for this type of addressing.
        static bool matches (Address const &theirs, Address const &ours)
        {
                return theirs.getTxId () == ours.getRxId () && theirs.getNetworkAddressExtension () == ours.getNetworkAddressExtension ();
        }
};

/****************************************************************************/

struct Mixed29AddressEncoder {

        static constexpr uint32_t PHYS_29 = 0x018Ce0000;
        static constexpr uint32_t FUNC_29 = 0x018Cd0000;
        static constexpr uint32_t PHYS_FUNC_29_MASK = 0x1FFf0000;
        static constexpr uint32_t N_TA_MASK = 0x00000ff00;
        static constexpr uint32_t N_SA_MASK = 0x0000000ff;

        /**
         * Create an address from a received CAN frame. This is
         * the address which the remote party used to send the frame to us.
         */
        template <typename CanFrameWrapper> static std::optional<Address> fromFrame (CanFrameWrapper const &f)
        {
                auto fId = f.getId ();

                if (!f.isExtended () || f.getDlc () < 1) {
                        return {};
                }

                if (bool ((fId & PHYS_FUNC_29_MASK) == PHYS_29)) {
                        return Address (0x00, 0x00, fId & N_SA_MASK, (fId & N_TA_MASK) >> 8, f.get (0), Address::MessageType::REMOTE_DIAGNOSTICS,
                                        Address::TargetAddressType::PHYSICAL);
                }

                if (bool ((fId & PHYS_FUNC_29_MASK) == FUNC_29)) {
                        return Address (0x00, 0x00, fId & N_SA_MASK, (fId & N_TA_MASK) >> 8, f.get (0), Address::MessageType::REMOTE_DIAGNOSTICS,
                                        Address::TargetAddressType::FUNCTIONAL);
                }

                return {};
        }

        /**
         * Store address into a CAN frame. This is the address of the remote party we want the message to get to.
         */
        template <typename CanFrameWrapper> static bool toFrame (Address const &a, CanFrameWrapper &f)
        {
                // if (a.messageType != Address::MessageType::REMOTE_DIAGNOSTICS) {
                //         return false;
                // }

                f.setId (((a.getTargetAddressType () == Address::TargetAddressType::PHYSICAL) ? (PHYS_29) : (FUNC_29))
                         | a.getTargetAddress () << 8 | a.getSourceAddress ());

                if (f.getDlc () < 1) {
                        f.setDlc (1);
                }

                f.set (0, a.getNetworkAddressExtension ());
                f.setExtended (true);
                return true;
        }

        /// Implements address matching for this type of addressing.
        static bool matches (Address const &theirs, Address const &ours)
        {
                return theirs.getTargetAddress () == ours.getSourceAddress ()
                        && theirs.getNetworkAddressExtension () == ours.getNetworkAddressExtension ();
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
};

template <typename AddressEncoder> struct AddressTraits : public AddressTraitsBase<AddressTraits<AddressEncoder>> {
        static constexpr bool USING_EXTENDED = false;
};

template <> struct AddressTraits<Extended11AddressEncoder> : public AddressTraitsBase<AddressTraits<Extended11AddressEncoder>> {
        static constexpr bool USING_EXTENDED = true;
};

template <> struct AddressTraits<Extended29AddressEncoder> : public AddressTraitsBase<AddressTraits<Extended29AddressEncoder>> {
        static constexpr bool USING_EXTENDED = true;
};

template <> struct AddressTraits<Mixed11AddressEncoder> : public AddressTraitsBase<AddressTraits<Mixed11AddressEncoder>> {
        static constexpr bool USING_EXTENDED = true;
};

template <> struct AddressTraits<Mixed29AddressEncoder> : public AddressTraitsBase<AddressTraits<Mixed29AddressEncoder>> {
        static constexpr bool USING_EXTENDED = true;
};

} // namespace tp
