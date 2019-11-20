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

/**
 * Default CAN message API. You can use it right out of the box or you can use some
 * other class among with a wrapper which you have to provide in sych a scenario.
 * This lets you integrate this transport protocol layer with existing link protocol
 * libraries.
 */
struct CanFrame {

        CanFrame () = default;

        template <typename... T>
        CanFrame (uint32_t id, bool extended, T... t) : id (id), extended (extended), data{uint8_t (t)...}, dlc (sizeof...(t))
        {
                Expects ((id & 0xE0000000) == 0);
        }

        uint32_t id;
        bool extended;
        etl::array<uint8_t, 8> data;
        uint8_t dlc;
};

template <typename CanFrameT> struct CanFrameWrapper {
};

/**
 * This is a wrapper which helps to use this library with different
 * CAN bus implementations. You can reimplement (add your own specialisation)
 * of this template class. Wrappers were split into this CanFrameWrapperBase
 * and CanFrameWrapper so users didn't have to reimplement all of the
 * methods CanFrameWrapper provide.
 */
template <> class CanFrameWrapper<CanFrame> {
public:
        explicit CanFrameWrapper (CanFrame cf) : frame (std::move (cf)) {} /// Construct from underlying implementation type.
        template <typename... T> CanFrameWrapper (uint32_t id, bool extended, T... data) : frame (id, extended, data...) {}
        CanFrameWrapper () = default;

        template <typename... T> static CanFrame create (uint32_t id, bool extended, T... data) { return CanFrame{id, extended, data...}; }

        CanFrame const &value () const { return frame; } /// Transform to underlying implementation type.

        uint32_t getId () const { return frame.id; } /// Arbitration id.
        void setId (uint32_t i) { frame.id = i; }    /// Arbitration id.

        bool isExtended () const { return frame.extended; }
        void setExtended (bool b) { frame.extended = b; }

        uint8_t getDlc () const { return frame.dlc; }
        void setDlc (uint8_t d) { frame.dlc = d; }

        uint8_t get (size_t i) const { return gsl::at (frame.data, i); }
        void set (size_t i, uint8_t b) { gsl::at (frame.data, i) = b; }

private:
        // TODO this should be some kind of handler if we wantto call this class a "wrapper".
        // This way we would get rid of one copy during reception of a CAN frame.
        // But at the other hand there must be a possibility to create new "wrappers" withgout
        // providing the internal object when sending frames.
        CanFrame frame{};
};

} // namespace tp
