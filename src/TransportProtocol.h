/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "Address.h"
#include "CanFrame.h"
#include "MiscTypes.h"
#include <cstdint>
#include <etl/map.h>
#include <gsl/gsl>
#include <optional>

/**
 * Set maximum number of Flow Control frames with WAIT bit set that can be received
 * before aborting further transmission of pending message. Check paragraph 6.6 of ISO
 * on page 23.
 */
#if !defined(MAX_WAIT_FRAME_NUMBER)
#define MAX_WAIT_FRAME_NUMBER 10
#endif

namespace tp {

/**
 *
 */
template <typename CanFrameT, typename IsoMessageT, size_t MAX_MESSAGE_SIZE_N, typename AddressResolverT, typename CanOutputInterfaceT,
          typename TimeProviderT, typename ExceptionHandlerT, typename CallbackT, size_t MAX_INTERLEAVED_ISO_MESSAGES_N>
struct TransportProtocolTraits {
        using CanFrame = CanFrameT;
        using IsoMessageTT = IsoMessageT;
        using CanOutputInterface = CanOutputInterfaceT;
        using TimeProvider = TimeProviderT;
        using ErrorHandler = ExceptionHandlerT;
        using Callback = CallbackT;
        using AddressEncoderT = AddressResolverT;
        static constexpr size_t MAX_MESSAGE_SIZE = MAX_MESSAGE_SIZE_N;
        static constexpr size_t MAX_INTERLEAVED_ISO_MESSAGES = MAX_INTERLEAVED_ISO_MESSAGES_N;
};

/*
 * As in 6.7.1 "Timing parameters". According to ISO 15765-2 it's 1000ms.
 * ISO 15765-4 and J1979 applies further restrictions down to 50ms but it applies to
 * time between query and the response of tester messages. See J1979:2006 chapter 5.2.2.6
 */
static constexpr size_t N_A_TIMEOUT = 1500;
static constexpr size_t N_BS_TIMEOUT = 1500;
static constexpr size_t N_CR_TIMEOUT = 1500;

/// Max allowed by the ISO standard.
static constexpr int MAX_ALLOWED_ISO_MESSAGE_SIZE = 4095;

/**
 * Implements ISO 15765-2 which is also called CAN ISO-TP or CAN ISO transport protocol.
 * Sources:
 * - ISO-15765-2-2004.pdf document.
 * - https://en.wikipedia.org/wiki/ISO_15765-2
 * - http://canbushack.com/iso-15765-2/
 * - http://iwasz.pl/private/index.php/STM32_CAN#ISO_15765-2
 *
 * Note : whenever I refer to a paragraph, or a page in an ISO document, I mean the ISO 15765-2:2004
 * because this is the only one I could find for free.
 *
 * Note : I'm assuming full-duplex communication, when onCanNewFrame and run calls are
 * interleaved.
 */
template <typename TraitsT> class TransportProtocol {
public:
        using CanFrame = typename TraitsT::CanFrame;
        using IsoMessageT = typename TraitsT::IsoMessageTT;
        using CanOutputInterface = typename TraitsT::CanOutputInterface;
        using TimeProvider = typename TraitsT::TimeProvider;
        using ErrorHandler = typename TraitsT::ErrorHandler;
        using Callback = typename TraitsT::Callback;
        using CanFrameWrapperType = CanFrameWrapper<CanFrame>;
        using AddressEncoderT = typename TraitsT::AddressEncoderT;
        using AddressTraitsT = AddressTraits<AddressEncoderT>;

        static constexpr size_t MAX_INTERLEAVED_ISO_MESSAGES = TraitsT::MAX_INTERLEAVED_ISO_MESSAGES;

        /// Max allowed by this implementation. Can be lowered if memory is scarce.
        static constexpr size_t MAX_ACCEPTED_ISO_MESSAGE_SIZE = TraitsT::MAX_MESSAGE_SIZE;

        TransportProtocol (Callback callback, CanOutputInterface outputInterface = {}, TimeProvider /*timeProvider*/ = {},
                           ErrorHandler errorHandler = {})
            : callback{callback},
              outputInterface{outputInterface},
              //              timeProvider{ timeProvider },
              errorHandler{errorHandler},
              stateMachine{this->outputInterface}
        {
        }

        TransportProtocol (Address myAddress, Callback callback, CanOutputInterface outputInterface = {}, TimeProvider /*timeProvider*/ = {},
                           ErrorHandler errorHandler = {})
            : callback{callback},
              outputInterface{outputInterface},
              //              timeProvider{ timeProvider },
              errorHandler{errorHandler},
              stateMachine{*this, this->outputInterface},
              myAddress (myAddress)
        {
        }

        TransportProtocol (TransportProtocol const &) = delete;
        TransportProtocol (TransportProtocol &&) = delete;
        TransportProtocol &operator= (TransportProtocol const &) = delete;
        TransportProtocol &operator= (TransportProtocol &&) = delete;
        ~TransportProtocol () = default;

        /**
         * Sends a message. If msg is so long, that it wouldn't fit in a SINGLE_FRAME (6 or 7 bytes
         * depending on addressing used), it is COPIED into the transport protocol object for further
         * processing, and then via run method is send in multiple CONSECUTIVE_FRAMES.
         * In ISO this method is called a 'request'
         */
        bool send (Address const &a, IsoMessageT msg);

        /**
         * Sends a message. If msg is so long, that it wouldn't fit in a SINGLE_FRAME (6 or 7 bytes
         * depending on addressing used), it is MOVED into the transport protocol object for further
         * processing, and then via run method is send in multiple CONSECUTIVE_FRAMES.
         * In ISO this method is called a 'request'
         */
        bool send (IsoMessageT msg) { return send (myAddress, std::move (msg)); }

        /**
         * Does the book keeping (checks for timeouts, runs the sending state machine).
         */
        void run ();

        /**
         *
         */
        bool isSending () const { return stateMachine.getState () != StateMachine::State::DONE; }

        /*
         * API jest asynchroniczne, bo na prawdę nie ma tego jak inaczej zrobić. Ramki CAN
         * przychodzą asynchronicznie (odpowiedzi na żądanie, ale także mogą przyjść same z
         * siebie) oraz może ich przyjść różna ilość. Zadając jakieś pytanie (na przykład o
         * błędy) nie jesteśmy w stanie do końca powiedzieć ile tych ramek przyjdzie. Na
         * przykład jeżeli mamy 1 ECU, to mogą przyjść 2 ramki składające się na 1 wiadomość
         * ISO, ale jeżeli mamy 2 ECU to mogą przyjść 3 albo 4 i tak dalej.
         *
         * API synchroniczne działało tak, że oczekiwało odpowiedzi tylko od jednego ECU. To
         * może być SingleFrame, albo FF + x * CF, ale dało się określić koniec tej wiadomości.
         * Kiedy wykryło koniec, to kończyło działanie ignorując ewentualne inne wiadomości.
         * Asynchroniczne API naprawia ten problem.
         *
         * Składa wiadomość ISO z poszczególnych ramek CAN. Zwraca czy potrzeba więcej ramek can aby
         * złożyć wiadomość ISO w całości. Kiedy podamy obiekt wiadomości jako drugi parametr, to
         * nie wywołuje callbacku, tylko zapisuje wynik w tym obiekcie. To jest używane w connect.
         */
        bool onCanNewFrame (CanFrame const &f) { return onCanNewFrame (CanFrameWrapperType{f}); }

        /**
         * myAddress address is used during reception
         * - target address of incoming message is checked with myAddress.sourceAddress
         * - flow control frames during reception are sent with myAddress.targetAddress.
         * And during sending:
         * - myAddress.targetAddress is used for outgoing frames if no address was specified during request (in send method).
         * - myAddress.sourceAddress is checked with incoming flowFrames if no address was specified during request (in send method).
         */
        void setMyAddress (Address const &a) { myAddress = a; }
        Address const &getMyAddress () const { return myAddress; }

        /**
         * This value will be sent in first flow control flow frame, and it tells the peer
         * how long to wait between sending consecutive frames. This is to offload the receiver.
         * Paragraph 6.5.5.5 in the 2004 ISO document. Default value 0 means that everything
         * has to be sent immediately.
         */
        void setSeparationTime (uint8_t s)
        {
                Expects ((s >= 0 && s <= 0x7f) || (s >= 0xf1 && s <= 0xf9));
                separationTime = s;
        }

        /**
         * This value will be sent in first flow control flow frame, and it tells the peer
         * how many consecutive frames to send in one burst. Then the receiver (the peer
         * who calls setBlockSize) has to send a flow frame ackgnowledging received frames and
         * and thus allowing for next batch of consecutive frames. Default 0 means that the
         * receiver wants all frames at once without any interruption. See 6.5.5.4.
         */
        void setBlockSize (uint8_t b) { blockSize = b; }

#ifndef UNIT_TEST
private:
#endif

        static uint32_t now ()
        {
                static TimeProvider tp;
                return tp ();
        }

        /*
         * @brief The Timer class
         * TODO this timer should have 100µs resolution and 100µs units.
         */
        class Timer {
        public:
                Timer (uint32_t intervalMs = 0) { start (intervalMs); }

                /// Resets the timer (it starts from 0) and sets the interval. So isExpired will return true after whole interval has passed.
                void start (uint32_t intervalMs)
                {
                        this->intervalMs = intervalMs;
                        this->startTime = getTick ();
                }

                /// Change interval without reseting the timer. Can extend as well as shorten.
                void extend (uint32_t intervalMs) { this->intervalMs = intervalMs; }

                /// Says if intervalMs has passed since start () was called.
                bool isExpired () const { return elapsed () >= intervalMs; }

                /// Returns how many ms has passed since start () was called.
                uint32_t elapsed () const
                {
                        uint32_t actualTime = getTick ();
                        return actualTime - startTime;
                }

                /// Convenience method, simple delay ms.
                void delay (uint32_t delayMs)
                {
                        Timer t{delayMs};
                        while (!t.isExpired ()) {
                        }
                }

                /// Returns system wide ms since system start.
                uint32_t getTick () const { return now (); }

        private:
                uint32_t startTime = 0;
                uint32_t intervalMs = 0;
        };

        /*
         * An ISO 15765-2 message (up to 4095B long). Messages composed from CAN frames.
         */
        struct TransportMessage {

                int append (CanFrameWrapperType const &frame, size_t offset, size_t len);

                // uint32_t address = 0; /// Address Information M_AI
                IsoMessageT data{};           /// Max 4095 (according to ISO 15765-2) or less if more strict requirements programmed by the user.
                int multiFrameRemainingLen{}; /// For tracking number of bytes remaining.
                int currentSn{};              /// Sequence number of Consecutive Frame.
                int consecutiveFramesReceived{}; /// For comparison with block size.
                Timer timer;                     /// For tracking time between first and consecutive frames with the same address.
                Result timeoutReason{};          /// It fimer expired, what was the result.
        };

        /*
         * StateMachine class implements an algorithm for sending a single ISO message, which
         * can be up to 4095B longa and thus has to be divided into multiple CAN frames.
         */
        class StateMachine {
        public:
                enum class State {
                        IDLE,
                        SEND_FIRST_FRAME,
                        RECEIVE_FIRST_FLOW_CONTROL_FRAME,
                        SEND_CONSECUTIVE_FRAME,
                        RECEIVE_BS_FLOW_CONTROL_FRAME,
                        DONE
                };

                explicit StateMachine (TransportProtocol &tp, CanOutputInterface &o) : tp (tp), outputInterface (o) {}
                ~StateMachine () = default;

                StateMachine (StateMachine &&sm) noexcept = delete;
                StateMachine &operator= (StateMachine &&sm) = delete;

                StateMachine (StateMachine const &sm) noexcept = delete;
                StateMachine &operator= (StateMachine const &sm) noexcept = delete;

                void reset (Address const &a, IsoMessageT &&m)
                {
                        myAddress = a;
                        message = std::move (m);
                        state = State::IDLE;
                }

                Status run (CanFrameWrapperType const *frame = nullptr);
                State getState () const { return state; }

        private:
                TransportProtocol &tp;
                CanOutputInterface &outputInterface;
                Address myAddress{};
                IsoMessageT message{};
                State state{State::DONE};
                size_t bytesSent{};
                uint16_t blocksSent{};
                uint8_t sequenceNumber{1};
                uint8_t receivedBlockSize{};
                uint32_t receivedSeparationTimeUs{};
                Timer separationTimer{};
                Timer bsCrTimer{};
                uint8_t waitFrameNumber{};
        };

        /*---------------------------------------------------------------------------*/

        bool onCanNewFrame (CanFrameWrapperType const &frame);

        /*---------------------------------------------------------------------------*/

        /// Checks if callback accepts single IsoMessage param thus has the form callback (IsoMessage msg) TODO use std::is_invocable_v
        template <typename T, typename = void> struct IsCallbackSimple : public std::false_type {
        };

        /// Checks if callback accepts single IsoMessage param thus has the form callback (IsoMessage msg)
        template <typename T>
        struct IsCallbackSimple<T, typename std::enable_if<true, decltype ((void)(std::declval<T &> () (IsoMessageT{})))>::type>
            : public std::true_type {
        };

        /// Checks for a callback which have 3 params like this : Address{}, IsoMessageT{}, Result{}
        template <typename T, typename = void> struct IsCallbackAdvanced : public std::false_type {
        };

        /// Checks for a callback which have 3 params like this : Address{}, IsoMessageT{}, Result{}
        template <typename T>
        struct IsCallbackAdvanced<
                T, typename std::enable_if<true, decltype ((void)(std::declval<T &> () (Address{}, IsoMessageT{}, Result{})))>::type>
            : public std::true_type {
        };

        template <typename T, typename = void> struct IsCallbackAdvancedMethod : public std::false_type {
        };

        template <typename T>
        struct IsCallbackAdvancedMethod<
                T, typename std::enable_if<true, decltype ((void)(std::declval<T &> ().indication (Address{}, IsoMessageT{}, Result{})))>::type>
            : public std::true_type {
        };

        template <typename T, typename = void> struct HasCallbackConfirmMethod : public std::false_type {
        };

        template <typename T>
        struct HasCallbackConfirmMethod<
                T, typename std::enable_if<true, decltype ((void)(std::declval<T &> ().confirm (Address{}, Result{})))>::type>
            : public std::true_type {
        };

        template <typename T, typename = void> struct HasCallbackFFIMethod : public std::false_type {
        };

        template <typename T>
        struct HasCallbackFFIMethod<
                T, typename std::enable_if<true, decltype ((void)(std::declval<T &> ().firstFrameIndication (Address{}, uint16_t{})))>::type>
            : public std::true_type {
        };

        void confirm (Address const &a, Result r)
        {
                if constexpr (HasCallbackConfirmMethod<Callback>::value) {
                        callback.confirm (a, r);
                }
        }

        void firstFrameIndication (Address const &a, uint16_t len)
        {
                if constexpr (HasCallbackFFIMethod<Callback>::value) {
                        callback.firstFrameIndication (a, len);
                }
        }

        void indication (Address const &a, IsoMessageT const &msg, Result r)
        {
                constexpr bool simpleCallback = IsCallbackSimple<Callback>::value;
                constexpr bool advancedCallback = IsCallbackAdvanced<Callback>::value;
                constexpr bool advancedMethodCallback = IsCallbackAdvancedMethod<Callback>::value;

                static_assert (simpleCallback || advancedCallback || advancedMethodCallback,
                               "Wrong callback interface. Use either 'simple', 'advanced', or 'advancedMethod' callback. See the README.md for "
                               "more info.");

                if constexpr (simpleCallback) {
                        callback (msg);
                }
                else if constexpr (advancedCallback) {
                        callback (a, msg, r);
                }
                else if constexpr (advancedMethodCallback) {
                        callback.indication (a, msg, r);
                }
        }

        /*---------------------------------------------------------------------------*/

        uint32_t getID (bool extended) const;
        bool sendFlowFrame (const Address &outgoingAddress, FlowStatus fs = FlowStatus::CONTINUE_TO_SEND);
        bool sendSingleFrame (const Address &a, IsoMessageT const &msg);
        bool sendMultipleFrames (const Address &a, IsoMessageT &&msg);

#ifndef UNIT_TEST
private:
#endif

        etl::map<Address, TransportMessage, MAX_INTERLEAVED_ISO_MESSAGES> transportMessagesMap;
        uint8_t blockSize{};
        uint8_t separationTime{};
        Callback callback;
        CanOutputInterface outputInterface;
        ErrorHandler errorHandler;
        StateMachine stateMachine;
        Address myAddress;
};

/*****************************************************************************/

template <typename TraitsT> bool TransportProtocol<TraitsT>::send (const Address &a, IsoMessageT msg)
{
        if (msg.size () > MAX_ACCEPTED_ISO_MESSAGE_SIZE) {
                return false;
        }

        // 6 or 7 depending on addressing used
        const size_t SINGLE_FRAME_MAX_SIZE = 7 - int (AddressTraitsT::USING_EXTENDED);

        if (msg.size () <= SINGLE_FRAME_MAX_SIZE) { // Send using single Frame
                return sendSingleFrame (a, msg);
        }

        // Send using multiple frames, state machine, and timing controll and whatnot.
        return sendMultipleFrames (a, std::move (msg));
}

/*****************************************************************************/

template <typename TraitsT> bool TransportProtocol<TraitsT>::sendSingleFrame (const Address &a, IsoMessageT const &msg)
{
        CanFrameWrapperType canFrame{0x00, true, int (IsoNPduType::SINGLE_FRAME) | (msg.size () & 0x0f)};

        if (!AddressEncoderT::toFrame (a, canFrame)) {
                errorHandler (Status::ADDRESS_ENCODE_ERROR);
                return false;
        }

        for (size_t i = 0; i < msg.size (); ++i) {
                canFrame.set (i + 1, msg.at (i));
        }

        canFrame.setDlc (1 + msg.size ());
        bool result = outputInterface (canFrame.value ());

        if (!result) {
                confirm (myAddress, Result::N_TIMEOUT_A);
        }
        else {
                confirm (myAddress, Result::N_OK);
        }

        return result;
}

/*****************************************************************************/

template <typename TraitsT> bool TransportProtocol<TraitsT>::sendMultipleFrames (const Address &a, IsoMessageT &&msg)
{
        if (stateMachine.getState () != StateMachine::State::DONE) {
                return false;
        }

        stateMachine.reset (a, std::move (msg));
        return true;
}

/*****************************************************************************/

template <typename TraitsT> bool TransportProtocol<TraitsT>::onCanNewFrame (const CanFrameWrapperType &frame)
{
        // Address as received in the CAN frame frame.
        auto theirAddress = AddressEncoderT::fromFrame (frame);
        Address const &outgoingAddress = myAddress;

        // Check if the received frame is meant for us.
        if (!theirAddress || !AddressEncoderT::matches (*theirAddress, myAddress)) {
                return false;
        }

        switch (AddressTraitsT::getType (frame)) {
        case IsoNPduType::SINGLE_FRAME: {
                TransportMessage message;
                int singleFrameLen = AddressTraitsT::getDataLengthS (frame);

                // Error situation. Such frames should be ignored according to 6.5.2.2 page 24.
                if (singleFrameLen <= 0 || (AddressTraitsT::USING_EXTENDED && singleFrameLen > 6) || singleFrameLen > 7) {
                        return false;
                }

                auto iter = transportMessagesMap.find (*theirAddress);

                if (iter != transportMessagesMap.cend ()) { // found
                        // As in 6.7.3 Table 18
                        indication (*theirAddress, {}, Result::N_UNEXP_PDU);
                        // Terminate the current reception of segmented message.
                        transportMessagesMap.erase (iter);
                        break;
                }

                uint8_t dataOffset = AddressTraitsT::N_PCI_OFSET + 1;
                message.append (frame, dataOffset, singleFrameLen);
                indication (*theirAddress, message.data, Result::N_OK);
        } break;

        case IsoNPduType::FIRST_FRAME: {
                uint16_t multiFrameRemainingLen = AddressTraitsT::getDataLengthF (frame);

                // Error situation. Such frames should be ignored according to ISO.
                if (multiFrameRemainingLen < 8 || (AddressTraitsT::USING_EXTENDED && multiFrameRemainingLen < 7)) {
                        return false;
                }

                // 6.5.3.3 Error situation : too much data. Should reply with apropriate flow control frame.
                if (multiFrameRemainingLen > MAX_ACCEPTED_ISO_MESSAGE_SIZE || multiFrameRemainingLen > MAX_ALLOWED_ISO_MESSAGE_SIZE) {
                        sendFlowFrame (outgoingAddress, FlowStatus::OVERFLOW);
                        return false;
                }

                // incomingAddress Should be used (as a key)!
                auto iter = transportMessagesMap.find (*theirAddress);

                if (iter != transportMessagesMap.cend ()) {
                        // As in 6.7.3 Table 18
                        indication (*theirAddress, {}, Result::N_UNEXP_PDU);
                        // Terminate the current reception of segmented message.
                        transportMessagesMap.erase (iter);
                }

                int firstFrameLen = (AddressTraitsT::USING_EXTENDED) ? (5) : (6);

                if (transportMessagesMap.full ()) {
                        indication (*theirAddress, {}, Result::N_MESSAGE_NUM_MAX);
                        return false;
                }

                auto &isoMessage = transportMessagesMap[*theirAddress];

                firstFrameIndication (*theirAddress, multiFrameRemainingLen);

                isoMessage.currentSn = 1;
                isoMessage.multiFrameRemainingLen = multiFrameRemainingLen - firstFrameLen;
                isoMessage.timer.start (N_BS_TIMEOUT);
                isoMessage.timeoutReason = Result::N_TIMEOUT_BS;
                uint8_t dataOffset = AddressTraitsT::N_PCI_OFSET + 2;
                isoMessage.append (frame, dataOffset, firstFrameLen);

                // Send Flow Control
                if (!sendFlowFrame (outgoingAddress, FlowStatus::CONTINUE_TO_SEND)) {
                        indication (*theirAddress, {}, Result::N_ERROR);
                        // Terminate the current reception of segmented message.
                        transportMessagesMap.erase (transportMessagesMap.find (*theirAddress));
                }

                return true;
        } break;

        case IsoNPduType::CONSECUTIVE_FRAME: {
                auto iter = transportMessagesMap.find (*theirAddress);

                if (iter == transportMessagesMap.cend ()) {
                        // As in 6.7.3 Table 18 - ignore
                        return false;
                }

                auto &transportMessage = iter->second;
                transportMessage.timer.start (N_CR_TIMEOUT);
                transportMessage.timeoutReason = Result::N_TIMEOUT_CR;

                if (AddressTraitsT::getSerialNumber (frame) != transportMessage.currentSn) {
                        // 6.5.4.3 SN error handling
                        indication (*theirAddress, {}, Result::N_WRONG_SN);
                        return false;
                }

                ++(transportMessage.currentSn);
                transportMessage.currentSn %= 16;
                int maxConsecutiveFrameLen = (AddressTraitsT::USING_EXTENDED) ? (6) : (7);
                int consecutiveFrameLen = std::min (maxConsecutiveFrameLen, transportMessage.multiFrameRemainingLen);
                transportMessage.multiFrameRemainingLen -= consecutiveFrameLen;
#if 0                
                fmt::print ("Bytes left : {}\n", isoMessage->multiFrameRemainingLen);
#endif

                uint8_t dataOffset = AddressTraitsT::N_PCI_OFSET + 1;
                transportMessage.append (frame, dataOffset, consecutiveFrameLen);

                // Send flow control frame.
                if (blockSize > 0 && ++transportMessage.consecutiveFramesReceived >= blockSize) {
                        transportMessage.consecutiveFramesReceived = 0;

                        if (!sendFlowFrame (outgoingAddress, FlowStatus::CONTINUE_TO_SEND)) {
                                indication (*theirAddress, {}, Result::N_ERROR);
                                // Terminate the current reception of segmented message.
                                transportMessagesMap.erase (transportMessagesMap.find (*theirAddress));
                        }
                }

                if (transportMessage.multiFrameRemainingLen) {
                        return true;
                }

                indication (*theirAddress, transportMessage.data, Result::N_OK);
                transportMessagesMap.erase (iter);

        } break;

        case IsoNPduType::FLOW_FRAME: {
                if (Status s = stateMachine.run (&frame); s != Status::OK) {
                        errorHandler (s);
                        return false;
                }

        } break;

        default:
                break; // Ignore unidentified N_PDU. See Table 18.
        }

        return false;
}

/*****************************************************************************/

template <typename TraitsT> void TransportProtocol<TraitsT>::run ()
{
        // Check for timeouts between CAN frames while receiving.
        for (auto i = transportMessagesMap.begin (); i != transportMessagesMap.end ();) {
                auto &tpMsg = i->second;

                if (tpMsg.timer.isExpired ()) {
                        indication (i->first, {}, i->second.timeoutReason);
                        auto j = i;
                        ++i;
                        transportMessagesMap.erase (j);
                        continue;
                }

                ++i;
        }

        // Run state machine(s) if any to perform transmission.
        if (Status s = stateMachine.run (); s != Status::OK) {
                errorHandler (s);
                return;
        }
}

/*****************************************************************************/

template <typename TraitsT> bool TransportProtocol<TraitsT>::sendFlowFrame (Address const &outgoingAddress, FlowStatus fs)
{
        CanFrameWrapperType fcCanFrame;

        if (!AddressEncoderT::toFrame (outgoingAddress, fcCanFrame)) {
                errorHandler (Status::ADDRESS_ENCODE_ERROR);
                return false;
        }

        fcCanFrame.set (AddressTraitsT::N_PCI_OFSET + 0, (uint8_t (IsoNPduType::FLOW_FRAME) << 4) | uint8_t (fs));
        fcCanFrame.set (AddressTraitsT::N_PCI_OFSET + 1, blockSize);      // BS
        fcCanFrame.set (AddressTraitsT::N_PCI_OFSET + 2, separationTime); // Stmin
        fcCanFrame.setDlc (3 + AddressTraitsT::N_PCI_OFSET);

        if (!outputInterface (fcCanFrame.value ())) {
                errorHandler (Status::SEND_FAILED);
                return false;
        }

        return true;
}

/*****************************************************************************/

template <typename TraitsT>
int TransportProtocol<TraitsT>::TransportMessage::append (CanFrameWrapperType const &frame, size_t offset, size_t len)
{
        for (size_t inputIndex = 0; inputIndex < len; ++inputIndex) {
                data.insert (data.end (), frame.get (inputIndex + offset));
        }

        return len;
}

/*****************************************************************************/

template <typename TraitsT> Status TransportProtocol<TraitsT>::StateMachine::run (CanFrameWrapperType const *frame)
{
        if (state == State::DONE) {
                return Status::OK;
        }

        if (state != State::IDLE && state != State::SEND_FIRST_FRAME && bsCrTimer.isExpired ()) {
                if (state == State::RECEIVE_BS_FLOW_CONTROL_FRAME || state == State::RECEIVE_FIRST_FLOW_CONTROL_FRAME) {
                        tp.confirm (myAddress, Result::N_TIMEOUT_BS);
                }
                else {
                        tp.confirm (myAddress, Result::N_TIMEOUT_CR);
                }

                state = State::DONE;
        }

        IsoMessageT *message = &this->message;
        uint16_t isoMessageSize = message->size ();
        using Traits = AddressTraits<AddressEncoderT>;

        switch (state) {
        case State::IDLE:
                state = State::SEND_FIRST_FRAME;
                break;

        case State::SEND_FIRST_FRAME: {

                CanFrameWrapperType canFrame (0x00, false, (int (IsoNPduType::FIRST_FRAME) << 4) | (isoMessageSize & 0xf00) >> 8,
                                              (isoMessageSize & 0x0ff));

                if (!AddressEncoderT::toFrame (myAddress, canFrame)) {
                        return Status::ADDRESS_ENCODE_ERROR;
                }

                int toSend = std::min<int> (isoMessageSize, 6);

                for (int i = 0; i < toSend; ++i) {
                        canFrame.set (i + 2, message->at (i));
                }

                canFrame.setDlc (2 + toSend);

                if (!outputInterface (canFrame.value ())) {
                        tp.confirm (myAddress, Result::N_TIMEOUT_A); // TODO is it correct Result::?
                        state = State::DONE;
                        break;
                }

                tp.confirm (myAddress, Result::N_OK);
                state = State::RECEIVE_FIRST_FLOW_CONTROL_FRAME;
                bytesSent += toSend;
                bsCrTimer.start (N_BS_TIMEOUT);
        } break;

        case State::RECEIVE_BS_FLOW_CONTROL_FRAME:
        case State::RECEIVE_FIRST_FLOW_CONTROL_FRAME: {
                if (!separationTimer.isExpired ()) {
                        break;
                }

                if (!frame) {
                        break;
                }

                // Address as received in the CAN frame frame.
                auto theirAddress = AddressEncoderT::fromFrame (*frame);

                if (!theirAddress || !AddressEncoderT::matches (*theirAddress, myAddress)) {
                        break;
                }

                IsoNPduType type = Traits::getType (*frame);

                if (type != IsoNPduType::FLOW_FRAME) {
                        break;
                }

                FlowStatus fs = Traits::getFlowStatus (*frame);

                if (fs != FlowStatus::CONTINUE_TO_SEND && fs != FlowStatus::WAIT && fs != FlowStatus::OVERFLOW) {
                        tp.confirm (*theirAddress, Result::N_INVALID_FS); // 6.5.5.3
                        state = State::DONE;                              // abort
                }

                if (fs == FlowStatus::OVERFLOW) {
                        tp.confirm (*theirAddress, Result::N_BUFFER_OVFLW);
                        state = State::DONE; // abort
                }

                if (fs == FlowStatus::WAIT) {
                        bsCrTimer.start (N_BS_TIMEOUT);
                        ++waitFrameNumber;

                        if (waitFrameNumber >= MAX_WAIT_FRAME_NUMBER) { // In case of MAX_WAIT_FRAME_NUMBER == 0 message will be aborted
                                                                        // immediately, which is fine according to the ISO.
                                tp.confirm (*theirAddress, Result::N_WFT_OVRN);
                                state = State::DONE; // abort
                        }

                        break; // state stays at RECEIVE_*_FLOW_CONTROL_FRAME
                }

                if (state == State::RECEIVE_FIRST_FLOW_CONTROL_FRAME) {
                        receivedBlockSize = frame->get (1); // 6.5.5.4 page 21
                        receivedSeparationTimeUs = frame->get (2);

                        if (receivedSeparationTimeUs >= 0 && receivedSeparationTimeUs <= 0x7f) {
                                receivedSeparationTimeUs *= 1000; // Convert to µs
                        }
                        else if (receivedSeparationTimeUs >= 0xf1 && receivedSeparationTimeUs <= 0xf9) {
                                receivedSeparationTimeUs = (receivedSeparationTimeUs - 0xf0) * 100;
                        }
                        else {
                                receivedSeparationTimeUs = 0x7f * 1000; // 6.5.5.6 ST error handling
                        }
                }

                waitFrameNumber = 0;
                separationTimer.start (0); // Separation timer is started later with proper tomeout calculated here.
                state = State::SEND_CONSECUTIVE_FRAME;
                bsCrTimer.start (N_CR_TIMEOUT);
        } break;

        case State::SEND_CONSECUTIVE_FRAME: {
                if (!separationTimer.isExpired ()) {
                        break;
                }

                CanFrameWrapperType canFrame (0x00, true, (int (IsoNPduType::CONSECUTIVE_FRAME) << 4) | sequenceNumber);

                if (!AddressEncoderT::toFrame (myAddress, canFrame)) {
                        return Status::ADDRESS_ENCODE_ERROR;
                }

                ++sequenceNumber;
                sequenceNumber %= 16;

                int toSend = std::min<int> (isoMessageSize - bytesSent, 7);
                for (int i = 0; i < toSend; ++i) {
                        canFrame.set (i + 1, message->at (i + bytesSent));
                }

                canFrame.setDlc (1 + toSend);

                if (!outputInterface (canFrame.value ())) {
                        tp.confirm (myAddress, Result::N_TIMEOUT_A);
                        state = State::DONE;
                        break;
                }

                bytesSent += toSend;

                if (bytesSent >= message->size ()) {
                        state = State::DONE;
                        break;
                }

                if (receivedBlockSize && ++blocksSent >= receivedBlockSize) {
                        state = State::RECEIVE_BS_FLOW_CONTROL_FRAME;
                        bsCrTimer.start (N_BS_TIMEOUT);
                        break;
                }

                // TODO separationTimeUs should be in 100µs units. Now i have 1ms resolution, so f1-f9 STmin are rounded to 0
                separationTimer.start (receivedSeparationTimeUs / 1000);
                bsCrTimer.start (N_CR_TIMEOUT);
                break;

        } break;

        default:
                break;
        }

        return Status::OK;
}

} // namespace tp
