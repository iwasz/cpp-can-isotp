/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
//#include "ArduinoCanFrame.h"
//#include "ArduinoTypes.h"
#include "CanFrame.h"
#include "TransportProtocol.h"
#include <mcp2515.h>

namespace tp {

// struct can_frame {
//        canid_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
//        __u8    can_dlc; /* frame payload length in byte (0 .. CAN_MAX_DLEN) */
//        __u8    data[CAN_MAX_DLEN] __attribute__((aligned(8)));
//};

/**
 * Wrapper for particular can frame type we are using ini this program (namely
 * can_frame in this case).
 */
template <> class CanFrameWrapper<can_frame> {
public:
        CanFrameWrapper () = default;
        explicit CanFrameWrapper (can_frame const &cf) : frame (cf) {} /// Construct from underlying implementation type.

        template <typename A, typename... B> void setData (int totalElements, A c1, B... cr)
        {
                static_assert (sizeof...(cr) <= 7);
                set (totalElements - 1 - sizeof...(cr), c1);

                if constexpr (sizeof...(cr) > 0) {
                        setData (totalElements, cr...);
                }
        }

        template <typename... T> CanFrameWrapper (uint32_t id, bool extended, T... data)
        {
                setId (id);
                setExtended (extended);
                setData (sizeof...(data), data...);
        }

        template <typename... T> static can_frame create (uint32_t id, bool extended, T... data) { return can_frame{id, extended, data...}; }

        can_frame const &value () const { return frame; }

        uint32_t getId () const { return (isExtended ()) ? (frame.can_id & CAN_EFF_MASK) : (frame.can_id & CAN_SFF_MASK); }
        void setId (uint32_t i) { frame.can_id = i; }

        bool isExtended () const { return bool (frame.can_id & CAN_EFF_FLAG); }
        void setExtended (bool b)
        {
                if (b) {
                        frame.can_id |= CAN_EFF_FLAG;
                }
                else {
                        frame.can_id &= ~CAN_EFF_FLAG;
                }
        }

        uint8_t getDlc () const { return frame.can_dlc; }
        void setDlc (uint8_t d) { frame.can_dlc = d; }

        uint8_t get (size_t i) const { return gsl::at (frame.data, i); }
        void set (size_t i, uint8_t b) { gsl::at (frame.data, i) = b; }

private:
        can_frame frame{};
};

/**
 * Sending a single can Frame on Arduino using https://github.com/autowp/arduino-mcp2515
 */
struct ArduinoCanOutputInterface {
        bool operator() (CanFrame const & /*unused*/) { return true; }
};
/**
 * @brief The ArduinoTimeProvider struct
 */
struct ArduinoTimeProvider {
        long operator() () const
        {
                // TODO
                //                static long i = 0;
                //                return ++i;
                return 0;
        }
};

/*****************************************************************************/

template <typename CanFrameT = CanFrame, typename AddressResolverT = Normal29AddressEncoder, typename IsoMessageT /*= IsoMessage*/,
          size_t MAX_MESSAGE_SIZE = MAX_ALLOWED_ISO_MESSAGE_SIZE, typename CanOutputInterfaceT /*= LinuxCanOutputInterface*/,
          typename TimeProviderT /*= ChronoTimeProvider*/, typename ExceptionHandlerT = InfiniteLoop, typename CallbackT /*= CoutPrinter*/>
auto create (Address const &myAddress, CallbackT callback, CanOutputInterfaceT outputInterface /*= {}*/, TimeProviderT timeProvider /* = {}*/,
             ExceptionHandlerT errorHandler /*= {}*/)
{
        using TP = TransportProtocol<TransportProtocolTraits<CanFrameT, IsoMessageT, MAX_MESSAGE_SIZE, AddressResolverT, CanOutputInterfaceT,
                                                             TimeProviderT, ExceptionHandlerT, CallbackT, 4>>;

        return TP{myAddress, callback, outputInterface, timeProvider, errorHandler};
}

} // namespace tp
