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
 * canfd_frame in this case).
 * TODO Add this to the main source tree.
 */
template <> class CanMessageWrapperBase<canfd_frame> {
public:
        explicit CanMessageWrapperBase (canfd_frame const &cf) : frame (cf) {} /// Construct from underlying implementation type.

        template <typename A, typename... B> void setData (A c1, B... cr)
        {
                static_assert (sizeof...(cr) <= 7);
                set (7 - sizeof...(cr), c1);

                if constexpr (sizeof...(cr) > 0) {
                        setData (cr...);
                }
        }

        template <typename... T> CanMessageWrapperBase (uint32_t id, bool extended, T... data)
        {
                setId (id);
                setExtended (extended);
                setData (data...);
        }

        template <typename... T> static canfd_frame create (uint32_t id, bool extended, T... data) { return canfd_frame{id, extended, data...}; }

        canfd_frame const &value () const { return frame; }

        uint32_t getId () const { return frame.can_id; }
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

        uint8_t getDlc () const { return frame.len; }
        void setDlc (uint8_t d) { frame.len = d; }

        uint8_t get (size_t i) const { return gsl::at (frame.data, i); }
        void set (size_t i, uint8_t b) { gsl::at (frame.data, i) = b; }

private:
        canfd_frame frame{};
};
} // namespace tp
