/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "CanFrame.h"
#include <linux/can.h>

namespace tp {

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
} // namespace tp
