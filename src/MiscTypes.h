/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <chrono>
#include <cstdint>
#include <gsl/gsl>
#include <iostream>
#include <vector>

namespace tp {
class CanFrame;

/**
 * @brief The TimeProvider struct
 * TODO if I'm not mistaken, this classes ought to have 100µs resolutiuon instead of 1000.
 */
struct ChronoTimeProvider {
        long operator() () const
        {
                using namespace std::chrono;
                system_clock::time_point now = system_clock::now ();
                auto duration = now.time_since_epoch ();
                return duration_cast<milliseconds> (duration).count ();
        }
};

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

/**
 * This interface assumes that sending single CAN frame is instant and we
 * now the status (whether it failed or succeeded) instantly (thus boolean)
 * return type.
 *
 * Important! If this interface fails, it shoud return false. Also, ISO requires,
 * that sending a CAN message shoud take not more as 1000ms + 50% i.e. 1500ms. If
 * this interface detects (internally) that, this time constraint wasn't met, it
 * also should return false. Those are N_As and N_Ar timeouts.
 */
struct LinuxCanOutputInterface {
        bool operator() (CanFrame const & /*unused*/) { return true; }
};

struct CoutPrinter {
        template <typename T> void operator() (T &&a) { std::cout << std::forward<T> (a) << std::endl; }
};

/// NPDU -> Network Protocol Data Unit
enum class IsoNPduType { SINGLE_FRAME = 0, FIRST_FRAME = 1, CONSECUTIVE_FRAME = 2, FLOW_FRAME = 3 };

/// FS field in Flow Control Frame
enum class FlowStatus { CONTINUE_TO_SEND = 0, WAIT = 1, OVERFLOW = 2 };

/**
 * Buffer for all input and ouptut operations
 */
using IsoMessage = std::vector<uint8_t>;

} // namespace tp
