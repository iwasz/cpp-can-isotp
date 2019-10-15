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
        std::array<uint8_t, 8> data;
        uint8_t dlc;
};

template <typename CanMessageT> struct CanMessageWrapperBase {
};

/**
 * This is a wrapper which helps to use this library with different
 * CAN bus implementations. You can reimplement (add your own specialisation)
 * of this template class. Wrappers were split into this CanMessageWrapperBase
 * and CanMessageWrapper so users didn't have to reimplement all of the
 * methods CanMessageWrapper provide.
 */
template <> class CanMessageWrapperBase<CanFrame> {
public:
        explicit CanMessageWrapperBase (CanFrame const &cf) : frame (cf) {} /// Construct from underlying implementation type.
        template <typename... T> CanMessageWrapperBase (uint32_t id, bool extended, T... data) : frame (id, extended, data...) {}

        template <typename... T> static CanFrame create (uint32_t id, bool extended, T... data) { return CanFrame{id, extended, data...}; }

        CanFrame const &value () const { return frame; } /// Transform to underlying implementation type.

        uint32_t getId () const { return frame.id; } /// Arbitration id.
        void setId (uint32_t i) { frame.id = i; }    /// Arbitration id.

        bool isExtended () const { return frame.extended; }
        void setExtended (bool b) { frame.extended = b; }

        uint8_t getDlc () const { return frame.dlc; }
        void setDlc (uint8_t d) { frame.dlc = d; }

        uint8_t get (size_t i) const { return frame.data[i]; }
        void set (size_t i, uint8_t b) { frame.data[i] = b; }

private:
        // TODO this should be some kind of handler if we wantto call this class a "wrapper".
        // This way we would get rid of one copy during reception of a CAN frame.
        // But at the other hand there must be a possibility to create new "wrappers" withgout
        // providing the internal object when sending frames.
        CanFrame frame;
};

/**
 * This is more speciffic interface and is not meant to be re-implemented.
 */
template <typename CanFrameT> class CanMessageWrapper : public CanMessageWrapperBase<CanFrameT> {
public:
        using CanMessageWrapperBase<CanFrameT>::CanMessageWrapperBase;
        using Base = CanMessageWrapperBase<CanFrameT>;

        /*---------------------------------------------------------------------------*/

        template <typename... T> static CanFrame create (uint32_t id, bool extended, T... data) { return CanFrame{id, extended, data...}; }

        // TODO implement
        bool isCanExtAddrActive () const { return false; }

        uint8_t getNPciOffset () const { return (isCanExtAddrActive ()) ? (1) : (0); }

        uint8_t getNPciByte () const { return Base::get (getNPciOffset ()); }
        IsoNPduType getType () const { return getType (*this); }
        static IsoNPduType getType (CanMessageWrapper const &f) { return IsoNPduType ((f.getNPciByte () & 0xF0) >> 4); }

        static bool isCanExtAddrActive (CanFrame const &f) { return false; }
        static IsoNPduType getType (CanFrame const &f) { return IsoNPduType ((f.data[(isCanExtAddrActive (f)) ? (1) : (0)] & 0xF0) >> 4); }

        /*---------------------------------------------------------------------------*/

        // Not the cleanest way of doning this, but it's a internal class. Maybe someday...
        uint8_t getDataLengthS () const { return getNPciByte () & 0x0f; }

        uint16_t getDataLengthF () const { return ((getNPciByte () & 0x0f) << 8) | Base::get (getNPciOffset () + 1); }

        uint8_t getSerialNumber () const { return getNPciByte () & 0x0f; }

        FlowStatus getFlowStatus () const { return FlowStatus (getNPciByte () & 0x0f); }
        uint8_t getBlockSize () const { return get (getNPciOffset () + 1); }
        uint8_t getSeparationTime () const { return get (getNPciOffset () + 2); }
};

} // namespace tp