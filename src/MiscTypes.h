/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "CppCompat.h"

namespace tp {

/**
 * Codes signaled to the user via indication and confirmation callbacks.
 */
enum class Result {
        // standared result codes
        N_OK,           /// Service execution performed successfully.
        N_TIMEOUT_A,    /// Timer N_Ar / N_As exceeded N_Asmax / N_Armax
        N_TIMEOUT_BS,   /// N_Bs time exceeded N_Bsmax
        N_TIMEOUT_CR,   /// N_Cr time exceeded N_Crmax
        N_WRONG_SN,     /// Unexpected sequence number in incoming consecutive frame
        N_INVALID_FS,   /// Invalid FlowStatus value received in flow controll frame.
        N_UNEXP_PDU,    /// Unexpected protocol data unit received.
        N_WFT_OVRN,     /// This signals thge user that
        N_BUFFER_OVFLW, /// When receiving flow controll with FlowStatus = OVFLW. Transmission is aborted.
        N_ERROR,        /// General error.
        // implementation defined result codes
        N_MESSAGE_NUM_MAX /// Not a standard error. This one means that there is too many different isoMessages beeing assembled from multiple
                          /// chunks of CAN frames now.

};

/**
 * Status (mostly error) codes passed into the errorHandler.
 */
enum class Status {
        OK,                   /// No error
        ADDRESS_DECODE_ERROR, /// Address encoder was unable to decode an address from frame id.
        ADDRESS_ENCODE_ERROR, /// Address encoder was unable to encode an address into a frame id.
        SEND_FAILED           /// Unable to send a can frame.
};

/**
 * @brief The InfiniteLoop struct
 */
struct InfiniteLoop {
        template <typename T> void operator() (T const & /*Error*/)
        {
                while (true) {
                }
        }
};

/**
 * @brief The EmptyCallback struct
 */
struct EmptyCallback {
        template <typename... T> void operator() (T... /* a */) {}
};

/// NPDU -> Network Protocol Data Unit
enum class IsoNPduType { SINGLE_FRAME = 0, FIRST_FRAME = 1, CONSECUTIVE_FRAME = 2, FLOW_FRAME = 3 };

/// FS field in Flow Control Frame
enum class FlowStatus { CONTINUE_TO_SEND = 0, WAIT = 1, OVERFLOWED = 2 };

} // namespace tp
