/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

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
        CanFrame (uint32_t id, bool extended, T... t) : id (id), extended (extended), data{ uint8_t (t)... }, dlc (sizeof...(t))
        { /*add (t...);*/
        }

        //        template <typename T> void add (T t)
        //        {
        //                static_assert (std::is_same<T, uint8_t>::value);
        //                data[data.size ()] = t;
        //        }

        //        template <typename T, typename... Rs> void add (T t, Rs... r)
        //        {
        //                static_assert (std::is_same<T, uint8_t>::value);
        //                data[data.size ()] = t;
        //                add (r...);
        //        }

        uint32_t id;
        bool extended;
        std::array<uint8_t, 8> data;
        uint8_t dlc;
};

class Address {
public:
        /// 5.3.1 Mtype
        enum class MessageType : uint8_t { DIAGNOSTICS, REMOTE_DIAGNOSTICS };
        /// Type and range of this address information.
        uint8_t mType;

        /// 5.3.2.2 N_SA encodes the network sender address. Valid both for DIAGNOSTICS and REMOTE_DIAGNOSTICS.
        uint8_t sourceAddress;

        /// 5.3.2.3 N_TA encodes the network target address. Valid both for DIAGNOSTICS and REMOTE_DIAGNOSTICS.
        uint8_t targetAddress;

        /// N_TAtype
        enum class TargetAddressType : uint8_t {
                PHYSICAL,  /// 1 to 1 communiaction supported for multiple and single frame communications
                FUNCTIONAL /// 1 to n communication (like broadcast?) is allowed only for single frame comms.
        };

        TargetAddressType targetAddressType;

        /// N_AE used optionally to extend both sa and ta.
        uint8_t networkAddressExtension;
};

struct TimeProvider {
        uint32_t operator() () const { return 0; }
};

enum class Error : uint32_t {
        N_NO_ERROR = 0,              /// No error.
        N_WRONG_SN = 0x40000000,     /// Zły sequence number w Consecutive Frame.
        N_BUFFER_OVFLW = 0x20000000, /// Przekroczony rozmiar danych które ktoś nam przysyła
        N_TIMEOUT = 0x10000000,      /// 6.7.2 Any N_PDU not transmitted in time on the sender side.
        N_UNEXP_PDU = 0x08000000,
        N_MESSAGE_NUM_MAX = 0x04000000, /// Za dużo wiadomości ISO (długich) na raz. To może oznaczać więcej
        /// niż MAX_MESSAGE_NUM ECU nadającychjednocześnie
        NOT_ISO_15765_4 = 0x02000000, /// Not ISO compliant
        CRITICAL_ALGORITHM
};

struct InfiniteLoop {
        [[noreturn]] void operator() (Error)
        {
                while (true) {
                }
        }
};

struct EmptyCallback {
        template <typename... T>[[noreturn]] void operator() (T...) {}
};

/**
 * This interface assumes that sending single CAN frame is instant and we
 * now the status (whether it failed or succeeded) instantly (thus boolean)
 * return type.
 */
struct LinuxCanOutputInterface {
        bool operator() (CanFrame const &) { return true; }
};

struct CoutPrinter {
        template <typename T> void operator() (T &&a) { std::cout << std::forward<T> (a) << std::endl; }
};

template <typename CanMessageT> struct CanMessageWrapper {
};

template <> class CanMessageWrapper<CanFrame> {
public:
        explicit CanMessageWrapper (CanFrame const &cf) : frame (cf) {} /// Construct from underlying implementation type.

        template <typename... T> CanMessageWrapper (uint32_t id, bool extended, T... data) : frame (id, extended, data...) {}

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
        CanFrame frame;
};

/**
 * Buffer for all input and ouptut operations
 */
using IsoMessage = std::vector<uint8_t>;

template <typename CanFrameT = CanFrame, typename IsoMessageT = IsoMessage, size_t MAX_MESSAGE_SIZE_T = 4095,
          typename CanOutputInterfaceT = LinuxCanOutputInterface, typename TimeProviderT = TimeProvider, typename ErrorHandlerT = InfiniteLoop,
          typename CallbackT = CoutPrinter>
struct TransportProtocolTraits {
        using CanMessage = CanFrameT;
        using IsoMessageTT = IsoMessageT;
        using CanOutputInterface = CanOutputInterfaceT;
        using TimeProvider = TimeProviderT;
        using ErrorHandler = ErrorHandlerT;
        using Callback = CallbackT;
        static constexpr size_t MAX_MESSAGE_SIZE = MAX_MESSAGE_SIZE_T;
};

/**
 * Implements ISO 15765-2 which is also called CAN ISO-TP or CAN ISO transport protocol.
 * Sources:
 * - ISO-15765-2-2004.pdf document.
 * - https://en.wikipedia.org/wiki/ISO_15765-2
 * - http://canbushack.com/iso-15765-2/
 * - http://iwasz.pl/private/index.php/STM32_CAN#ISO_15765-2
 *
 * TODO Handle ISO messages that are constrained to less than maximum 4095B allowed by the
 * ISO document.
 * TODO implement all types of addressing.
 * TODO Use some beter means of unit testing. Test time dependent calls, maybe use some
 * clever unit testing library like trompeleoleil for mocking.
 * TODO test crosswise connected objects. They should be able to speak to each other.
 * TODO test this library with python-can-isotp.
 * TODO add blocking API.
 * TODO test this api with std::threads.
 * TODO implement FF parameters : BS and STime
 * TODO verify with the ISO pdf whether everything is implemented.
 *
 * N_USData.request (fullAddress, message)
 * N_USData.confirm (fullAddress, result)
 * N_USData_FF.indication (fullAddress, LENGTH) <- callback that first frame was received. It tells what is the lenghth of expected message
 * N_USData.indication (fullAddr, Message, result) - after Sf or after multi-can-message
 * N_ChangeParameter.request (fullAddr, key, value) - requests a parameter change in peer
 * N_ChangeParameter.confirm  (fullAddr, key, result).
 */
template <typename TraitsT> class TransportProtocol {
public:
        using CanMessage = typename TraitsT::CanMessage;
        using IsoMessageT = typename TraitsT::IsoMessageTT;
        using CanOutputInterface = typename TraitsT::CanOutputInterface;
        using TimeProvider = typename TraitsT::TimeProvider;
        using ErrorHandler = typename TraitsT::ErrorHandler;
        using Callback = typename TraitsT::Callback;
        using CanMessageWrapperType = CanMessageWrapper<CanMessage>;

        /// NPDU -> Network Protocol Data Unit
        enum IsoNPduType { SINGLE_FRAME = 0, FIRST_FRAME = 1, CONSECUTIVE_FRAME = 2, FLOW_FRAME = 3 };

        /*
         * As in 6.7.1 "Timing parameters". According to ISO 15765-2 it's 1000ms.
         * ISO 15765-4 and J1979 applies further restrictions down to 50ms but it applies to
         * time between query and the response of tester messages. See J1979:2006 chapter 5.2.2.6
         */
        enum Timeouts { P2_TIMEOUT = 50, N_A_TIMEOUT = 1000, N_B_TIMEOUT = 1000, N_C_TIMEOUT = 1000 };
        /// Max allowed by the ISO standard.
        const int MAX_ALLOWED_ISO_MESSAGE_SIZE = 4095;
        /// Max allowed by this implementation. Can be lowered if memory is scarce.
        static constexpr size_t MAX_ACCEPTED_ISO_MESSAGE_SIZE = TraitsT::MAX_MESSAGE_SIZE;
        /// FS field in Flow Control Frame
        enum FlowStatus { STATUS_CONTINUE_TO_SEND = 0, STATUS_WAIT = 1, STATUS_OVERFLOW = 2 };

        TransportProtocol (Callback callback, CanOutputInterface outputInterface = {}, TimeProvider /*timeProvider*/ = {},
                           ErrorHandler errorHandler = {})
            : stack{ errorHandler },
              callback{ callback },
              outputInterface{ outputInterface },
              //              timeProvider{ timeProvider },
              errorHandler{ errorHandler }
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
         */
        bool send (IsoMessageT const &msg);

        /**
         * Sends a message. If msg is so long, that it wouldn't fit in a SINGLE_FRAME (6 or 7 bytes
         * depending on addressing used), it is MOVED into the transport protocol object for further
         * processing, and then via run method is send in multiple CONSECUTIVE_FRAMES.
         */
        bool send (IsoMessageT &&msg);

        /**
         * Does the book keeping (checks for timeouts, runs the sending state machine).
         */
        void run ();

        /**
         *
         */
        bool isSending () const { return bool(stateMachine); }

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
         */
        /**
         * Składa wiadomość ISO z poszczególnych ramek CAN. Zwraca czy potrzeba więcej ramek can aby
         * złożyć wiadomość ISO w całości. Kiedy podamy obiekt wiadomości jako drugi parametr, to
         * nie wywołuje callbacku, tylko zapisuje wynik w tym obiekcie. To jest używane w connect.
         */
        void onCanNewFrame (CanMessage const &f) { onCanNewFrame (CanMessageWrapperType{ f }); }

        //?? Po co to
        void onCanError (uint32_t e);

        /**
         * Connection parameters are beeing figured out here according to procedures described in
         * ISO 15765-4 chapter 4. CAN connection properties like baudrate and number of bits in ID
         * are tried one and one and after successful communication we now what setting to use.
         */
        //        bool connect ();

        void setCanExtAddr (uint8_t u);
        uint8_t getCanExtAddr () const { return canExtAddr; }
        bool isCanExtAddrActive () const { return canExtAddrActive; }

private:
        static uint32_t now ()
        {
                static TimeProvider tp;
                return tp ();
        }

        /*
         * @brief The Timer class
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
                        Timer t{ delayMs };
                        while (!t.isExpired ())
                                ;
                }

                /// Returns system wide ms since system start.
                uint32_t getTick () const { return now (); }

        protected:
                uint32_t startTime = 0;
                uint32_t intervalMs = 0;
        };

        /*
         * An ISO 15765-2 message (up to 4095B long). Messages composed from CAN frames.
         */
        struct TransportMessage {

                int append (CanMessageWrapperType const &frame, size_t offset, size_t len);

                uint32_t address = 0; /// Address Information M_AI
                IsoMessageT data;     /// Max 4095 (according to ISO 15765-2) or less if more strict requirements programmed by the user.
                TransportMessage *prev = nullptr; /// Double linked list implementation.
                TransportMessage *next = nullptr; /// Double linked list implementation.
                int multiFrameRemainingLen = 0;   /// For tracking number of bytes remaining.
                int currentSn = 0;                /// Sequence number of Consecutive Frame.
                Timer timer;                      /// For tracking time between first and consecutive frames with the same address.
        };

        bool onCanNewFrame (CanMessageWrapperType const &frame);

        /*
         * Double linked list of ISO 15765-2 messages (up to 4095B long).
         */
        class TransportMessageList {
        public:
                enum { MAX_MESSAGE_NUM = 8 };
                TransportMessageList (ErrorHandler &e) : first (nullptr), size (0), errorHandler (e) {}

                TransportMessage *findMessage (uint32_t address);
                TransportMessage *addMessage (uint32_t address /*, size_t bufferSize*/);
                TransportMessage *removeMessage (TransportMessage *m);
                size_t getSize () const { return size; }
                TransportMessage *getFirst () { return first; }

        private:
                TransportMessage *first;
                size_t size;
                ErrorHandler &errorHandler;
        };

        /*
         * StateMachine class implements an algorithm for sending a single ISO message, which
         * can be up to 4095B longa and thus has to be divided into multiple CAN frames.
         */
        class StateMachine {
        public:
                enum class State { IDLE, SEND_FIRST_FRAME, RECEIVE_FLOW_FRAME, SEND_CONSECUTIVE_FRAME, DONE };

                StateMachine (CanOutputInterface &o, IsoMessageT const &m)
                    : outputInterface (o), message (m), state{ State::IDLE }, bytesSent (0)
                {
                }

                StateMachine (CanOutputInterface &o, IsoMessageT &&m) : outputInterface (o), message (m), state{ State::IDLE }, bytesSent (0) {}

                ~StateMachine () = default;

                StateMachine (StateMachine &&sm) noexcept = default;
                StateMachine &operator= (StateMachine &&sm) noexcept = default;

                StateMachine (StateMachine const &sm) noexcept : message{ sm.message }, state{ sm.state } {}
                StateMachine &operator= (StateMachine const &sm) noexcept
                {
                        message = sm.message;
                        state = sm.state;
                        return *this;
                }

                void run (CanMessageWrapperType const *frame = nullptr);
                State getState () const { return state; }

        private:
                CanOutputInterface &outputInterface;
                IsoMessageT message;
                State state;
                size_t bytesSent = 0;
                uint8_t sequenceNumber = 1;
        };

        /*---------------------------------------------------------------------------*/

        uint32_t getID (bool extended) const;
        void processFlowFrame (const CanMessageWrapperType &msgBuffer, FlowStatus fs = STATUS_CONTINUE_TO_SEND);
        bool sendSingleFrame (IsoMessageT const &msg);
        bool sendMultipleFrames (IsoMessageT const &msg);
        bool sendMultipleFrames (IsoMessageT &&msg);

        //        enum ConnectionState { CONNECTED, SEND_FUNCTIONAL_11, WAIT_REPLY_11, SEND_FUNCTIONAL_29, WAIT_REPLY_29, NOT_ISO_COMPLIANT };
        //        ConnectionState connectSingleBaudrate ();

        TransportMessageList stack;
        /// Tells if CAN frames are extended (29bit ID), or standard (11bit ID).
        bool extendedFrame = false;
        /// Tells if ISO 15765-2 extended addressing is used or not. Not do be confused with extended CAN frames.
        bool canExtAddrActive = false;
        //        bool connected = false;
        uint8_t canExtAddr = 0;

        Callback callback;
        CanOutputInterface outputInterface;
        ErrorHandler errorHandler;
        std::optional<StateMachine> stateMachine;
};

/*****************************************************************************/

template <typename CanFrameT = CanFrame, typename IsoMessageT = IsoMessage, size_t MAX_MESSAGE_SIZE = 4095,
          typename CanOutputInterfaceT = LinuxCanOutputInterface, typename TimeProviderT = TimeProvider, typename ErrorHandlerT = InfiniteLoop,
          typename CallbackT = CoutPrinter>
TransportProtocol<
        TransportProtocolTraits<CanFrameT, IsoMessageT, MAX_MESSAGE_SIZE, CanOutputInterfaceT, TimeProviderT, ErrorHandlerT, CallbackT>>
create (CallbackT callback, CanOutputInterfaceT outputInterface = {}, TimeProviderT timeProvider = {}, ErrorHandlerT errorHandler = {})
{
        return { callback, outputInterface, timeProvider, errorHandler };
}

/*****************************************************************************/

template <typename TraitsT> bool TransportProtocol<TraitsT>::send (IsoMessageT const &msg)
{
        if (msg.size () > MAX_ACCEPTED_ISO_MESSAGE_SIZE) {
                return false;
        }

        // 6 or 7 depending on addressing used
        const size_t SINGLE_FRAME_MAX_SIZE = 7 - int(isCanExtAddrActive ());

        if (msg.size () <= SINGLE_FRAME_MAX_SIZE) { // Send using single Frame
                return sendSingleFrame (msg);
        }
        else { // Send using multiple frames, state machine, and timing controll and whatnot.
                return sendMultipleFrames (msg);
        }

        return true;
}

/*****************************************************************************/

template <typename TraitsT> bool TransportProtocol<TraitsT>::send (IsoMessageT &&msg)
{
        if (msg.size () > MAX_ACCEPTED_ISO_MESSAGE_SIZE) {
                return false;
        }

        // 6 or 7 depending on addressing used
        const size_t SINGLE_FRAME_MAX_SIZE = 7 - int(isCanExtAddrActive ());

        if (msg.size () <= SINGLE_FRAME_MAX_SIZE) { // Send using single Frame
                return sendSingleFrame (msg);
        }
        else { // Send using multiple frames, state machine, and timing controll and whatnot.
                return sendMultipleFrames (msg);
        }

        return true;
}

/*****************************************************************************/

template <typename TraitsT> bool TransportProtocol<TraitsT>::sendSingleFrame (IsoMessageT const &msg)
{
        CanMessageWrapperType canFrame{ 0x00, true, IsoNPduType::SINGLE_FRAME | (msg.size () & 0x0f) };

        for (size_t i = 0; i < msg.size (); ++i) {
                canFrame.set (i + 1, msg.at (i));
        }

        canFrame.setDlc (1 + msg.size ());
        return outputInterface (canFrame.value ());
}

/*****************************************************************************/

template <typename TraitsT> bool TransportProtocol<TraitsT>::sendMultipleFrames (IsoMessageT const &msg)
{
        // Start state machine.
        // 1. Send first frame
        stateMachine = StateMachine{ outputInterface, msg };

        // 2. wait for Flow frame
        // 3. Send consecutive frames with preset delay.
        return true;
}

// TODO I wouldn't multiply those methods but rather use perfect forwarding magic (?)
template <typename TraitsT> bool TransportProtocol<TraitsT>::sendMultipleFrames (IsoMessageT &&msg)
{
        // Start state machine.
        // 1. Send first frame
        stateMachine = StateMachine{ outputInterface, msg };

        // 2. wait for Flow frame
        // 3. Send consecutive frames with preset delay.
        return true;
}

/*****************************************************************************/

template <typename TraitsT> void TransportProtocol<TraitsT>::onCanError (uint32_t e) { errorHandler (e); }

/*****************************************************************************/

template <typename TraitsT> bool TransportProtocol<TraitsT>::onCanNewFrame (const CanMessageWrapperType &frame)
{
#if 0
        Debug *d = Debug::singleton ();
        d->print ("id:");
        d->print (to_ascii ((uint8_t *)&frame.id, 4).c_str ());
        d->print (", data:");
        d->print (to_ascii (frame.data, frame.dlc).c_str ());
        d->print ("\n");
#endif

        uint8_t nPciOffset = (isCanExtAddrActive ()) ? (1) : (0);
        uint8_t nPciByte1 = frame.get (nPciOffset); /// MSB, pierwszy bajt.
        uint8_t npduType = ((nPciByte1 & 0xF0) >> 4);

        switch (npduType) {
        case IsoNPduType::SINGLE_FRAME: {
                TransportMessage message;
                int singleFrameLen = nPciByte1; // Nie trzeba maskować z 0x0F, bo N_PCItype == 0.

                // Error situation. Such frames should be ignored according to 6.5.2.2 page 24.
                if (singleFrameLen <= 0 || (isCanExtAddrActive () && singleFrameLen > 6) || singleFrameLen > 7) {
                        return false;
                }

                TransportMessage *isoMessage = stack.findMessage (frame.getId ());

                if (isoMessage != nullptr) {
                        // As in 6.7.3 Table 18
                        errorHandler (Error::N_UNEXP_PDU);
                        // Terminate the current reception of segmented message.
                        stack.removeMessage (isoMessage);
                }

                uint8_t dataOffset = nPciOffset + 1;
                message.append (frame, dataOffset, singleFrameLen);
                callback (message.data);
        } break;

        case IsoNPduType::FIRST_FRAME: {
                uint16_t multiFrameRemainingLen = ((nPciByte1 & 0x0f) << 8) | frame.get (nPciOffset + 1);

                // Error situation. Such frames should be ignored according to ISO.
                if (multiFrameRemainingLen < 8 || (isCanExtAddrActive () && multiFrameRemainingLen < 7)) {
                        return false;
                }

                // 6.5.3.3 Error situation : too much data. Should reply with apropriate flow control frame.
                if (multiFrameRemainingLen > MAX_ACCEPTED_ISO_MESSAGE_SIZE || multiFrameRemainingLen > MAX_ALLOWED_ISO_MESSAGE_SIZE) {
                        processFlowFrame (frame, STATUS_OVERFLOW);
                        errorHandler (Error::N_BUFFER_OVFLW);
                        return false;
                }

                TransportMessage *isoMessage = stack.findMessage (frame.getId ());

                if (isoMessage) {
                        // As in 6.7.3 Table 18
                        errorHandler (Error::N_UNEXP_PDU);
                        // Terminate the current reception of segmented message.
                        stack.removeMessage (isoMessage);
                }

                int firstFrameLen = (isCanExtAddrActive ()) ? (5) : (6);
                isoMessage = stack.addMessage (frame.getId () /*, multiFrameRemainingLen*/);

                if (!isoMessage) {
                        errorHandler (Error::N_MESSAGE_NUM_MAX);
                        return false;
                }

                isoMessage->currentSn = 1;
                isoMessage->multiFrameRemainingLen = multiFrameRemainingLen - firstFrameLen;
                isoMessage->timer.start (N_B_TIMEOUT);
                uint8_t dataOffset = nPciOffset + 2;
                isoMessage->append (frame, dataOffset, firstFrameLen);

                // Send Flow Control
                processFlowFrame (frame, STATUS_CONTINUE_TO_SEND);
                return true;
        } break;

        case IsoNPduType::CONSECUTIVE_FRAME: {
                TransportMessage *isoMessage = stack.findMessage (frame.getId ());

                if (!isoMessage) {
                        return false;
                }

                isoMessage->timer.start (N_C_TIMEOUT);

                if ((nPciByte1 & 0x0f) != isoMessage->currentSn) {
                        // 6.5.4.3 SN error handling
                        errorHandler (Error::N_WRONG_SN);
                        return false;
                }

                ++(isoMessage->currentSn);
                isoMessage->currentSn %= 16;
                int maxConsecutiveFrameLen = (isCanExtAddrActive ()) ? (6) : (7);
                int consecutiveFrameLen = std::min (maxConsecutiveFrameLen, isoMessage->multiFrameRemainingLen);
                isoMessage->multiFrameRemainingLen -= consecutiveFrameLen;

                uint8_t dataOffset = nPciOffset + 1;
                // std::copy (frame.data + dataOffset, frame.data + dataOffset + consecutiveFrameLen, std::back_inserter (isoMessage->data));
                isoMessage->append (frame, dataOffset, consecutiveFrameLen);

                if (isoMessage->multiFrameRemainingLen) {
                        return true;
                }
                else {
                        callback (isoMessage->data);
                        stack.removeMessage (isoMessage);
                }

        } break;

        case IsoNPduType::FLOW_FRAME: {
                if (!stateMachine) {
                        break;
                }

                stateMachine->run (&frame);
        } break;

        default:
                break;
        }

        return false;
}

/*****************************************************************************/

template <typename TraitsT> void TransportProtocol<TraitsT>::run ()
{
        // Check for timeouts between CAN frames while receiving.
        TransportMessage *ptr = stack.getFirst ();

        while (ptr) {
                if (ptr->timer.isExpired ()) {
                        errorHandler (Error::N_TIMEOUT);
                        ptr = stack.removeMessage (ptr);
                        continue;
                }

                ptr = ptr->next;
        }

        // Run state machine(s) if any to perform transmission.
        if (stateMachine) {
                stateMachine->run ();
        }

        if (stateMachine->getState () == StateMachine::State::DONE) {
                stateMachine = std::nullopt;
        }
}

/*****************************************************************************/

template <typename TraitsT> void TransportProtocol<TraitsT>::setCanExtAddr (uint8_t u)
{
        canExtAddrActive = true;
        canExtAddr = u;
}

/*****************************************************************************/

template <typename TraitsT> uint32_t TransportProtocol<TraitsT>::getID (bool extended) const
{
        // ISO 15765-4:2005 chapter 6.3.2.2 & 6.3.2.3
        return (extended) ? (0x18db33f1) : (0x7df);
}

// template <typename TraitsT>
// void TransportProtocol<TraitsT>::setFilterAndMask (bool extended)
//{
//        if (extended) {
//                driver->setFilterAndMask (0x18DAF100, 0x1FFFFF00, extended);
//        }
//        else {
//                driver->setFilterAndMask (0x7E8, 0x7F8, extended);
//        }
//}

/*****************************************************************************/

// TODO addresses are messed up, whole addressing thing should be reimplemented, checked.
template <typename TraitsT> void TransportProtocol<TraitsT>::processFlowFrame (CanMessageWrapperType const &frame, FlowStatus fs)
{
        if (extendedFrame) {
                /*
                 * BlockSize == 0, which means indefinite block size, and STMin == 0 which means
                 * minimum delay between consecutive frames
                 */
                CanMessageWrapperType ctrlData (0x18DA00F1 | ((frame.getId () & 0xFF) << 8), true, 0x30 | fs);
                ctrlData.set (1, 0);
                ctrlData.set (2, 0);
                outputInterface (ctrlData.value ());
        }
        else {
                CanMessageWrapperType ctrlData (0x7E0 | (frame.getId () & 0x07), false, 0x30 | fs);
                ctrlData.set (1, 0);
                ctrlData.set (2, 0);
                outputInterface (ctrlData.value ());
        }
}

/*****************************************************************************/

template <typename TraitsT>
typename TransportProtocol<TraitsT>::TransportMessage *TransportProtocol<TraitsT>::TransportMessageList::findMessage (uint32_t address)
{
        TransportMessage *ptr = first;

        while (ptr) {
                if (ptr->address == address) {
                        return ptr;
                }

                ptr = ptr->next;
        }

        return nullptr;
}

/*****************************************************************************/

template <typename TraitsT>
typename TransportProtocol<TraitsT>::TransportMessage *
TransportProtocol<TraitsT>::TransportMessageList::addMessage (uint32_t address /*,size_t bufferSize*/)
{
        if (size++ >= MAX_MESSAGE_NUM) {
                return nullptr;
        }

        TransportMessage *m = new TransportMessage ();
        m->address = address;
        //        m->data.reserve (bufferSize);
        //        m->data.clear ();

        TransportMessage **ptr = &first;
        TransportMessage *last = nullptr;
        while (*ptr) {
                last = *ptr;
                ptr = &(*ptr)->next;
        }

        m->prev = last;
        *ptr = m;
        return m;
}

/*****************************************************************************/

template <typename TraitsT>
typename TransportProtocol<TraitsT>::TransportMessage *TransportProtocol<TraitsT>::TransportMessageList::removeMessage (TransportMessage *m)
{
        if (!m) {
                return nullptr;
        }

        TransportMessage *afterM = m->next;

        if (!first || !size) {
                // Critical error. Hang.
                errorHandler (Error::CRITICAL_ALGORITHM);
        }

        if (!m->prev) {
                first = m->next;

                if (first) {
                        first->prev = nullptr;
                }
        }
        else {
                m->prev->next = m->next;
        }

        if (!m->next) {
                if (m->prev) {
                        m->prev->next = nullptr;
                }
        }
        else {
                m->next->prev = m->prev;
        }

        --size;
        delete m;
        return afterM;
}

/*****************************************************************************/

template <typename TraitsT>
int TransportProtocol<TraitsT>::TransportMessage::append (CanMessageWrapperType const &frame, size_t offset, size_t len)
{
        for (size_t inputIndex = 0; inputIndex < len; ++inputIndex) {
                data.insert (data.end (), frame.get (inputIndex + offset));
        }

        return len;
}

/*****************************************************************************/

template <typename TraitsT> void TransportProtocol<TraitsT>::StateMachine::run (CanMessageWrapperType const *frame)
{
        uint16_t isoMessageSize = message.size ();

        switch (state) {
        case State::IDLE:
                state = State::SEND_FIRST_FRAME;
                break;

        case State::SEND_FIRST_FRAME: {
                // TODO addressing
                CanMessageWrapperType canFrame (0x00, true, (IsoNPduType::FIRST_FRAME << 4) | (isoMessageSize & 0xf00) >> 8,
                                                (isoMessageSize & 0x0ff));

                int toSend = std::min<int> (isoMessageSize, 6);
                for (int i = 0; i < toSend; ++i) {
                        canFrame.set (i + 2, message.at (i));
                }

                canFrame.setDlc (2 + toSend);
                // TODO what if it fails (returns false).
                outputInterface (canFrame.value ());
                state = State::RECEIVE_FLOW_FRAME;
                bytesSent += toSend;
                // TDOO start timer.
        } break;

        case State::RECEIVE_FLOW_FRAME:
                if (!frame) {
                        // checkTimer ();
                        // If timer is overdue, cal error handler, finish state machine
                        break;
                }

                if (frame) {
                        IsoNPduType type = IsoNPduType ((frame->get (0) & 0xf0) >> 4);
                        if (type == IsoNPduType::FLOW_FRAME) {
                                state = State::SEND_CONSECUTIVE_FRAME;
                        }
                }

                // if (flowFrame is valid) change state
                break;

        case State::SEND_CONSECUTIVE_FRAME: {
                if (bytesSent >= message.size ()) {
                        state = State::DONE;
                        break;
                }

                if (frame) {
                        // TODO flow frame can be received during sending of multiple
                        // frames.
                        break;
                }

                // TODO addressing
                CanMessageWrapperType canFrame (0x00, true, (IsoNPduType::CONSECUTIVE_FRAME << 4) | sequenceNumber);

                ++sequenceNumber;
                sequenceNumber %= 16;

                int toSend = std::min<int> (isoMessageSize - bytesSent, 7);
                for (int i = 0; i < toSend; ++i) {
                        canFrame.set (i + 1, message.at (i + bytesSent));
                }

                canFrame.setDlc (1 + toSend);
                // TODO what if it fails (returns false).
                outputInterface (canFrame.value ());
                bytesSent += toSend;

        } break;

        default:
                break;
        }
}

} // namespace tp

/*****************************************************************************/

inline std::ostream &operator<< (std::ostream &o, tp::IsoMessage const &b)
{
        o << "[";

        for (auto i = b.cbegin (); i != b.cend ();) {

                o << std::hex << int(*i);

                if (++i != b.cend ()) {
                        o << ",";
                }
        }

        o << "]";
        return o;
}

/*****************************************************************************/

inline std::ostream &operator<< (std::ostream &o, tp::CanFrame const &cf)
{
        // TODO data.
        o << "CanFrame id = " << cf.id << ", dlc = " << int(cf.dlc) << ", ext = " << cf.extended << ", data = [";

        for (int i = 0; i < cf.dlc;) {
                o << int(cf.data[i]);

                if (++i != cf.dlc) {
                        o << ",";
                }
                else {
                        break;
                }
        }

        o << "]";
        return o;
}
