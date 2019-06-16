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

struct IAddress {
        virtual ~IAddress () = default;

        //        /// 5.3.1 Mtype
        //        enum class MessageType : uint8_t { DIAGNOSTICS, REMOTE_DIAGNOSTICS };

        //        /// Type and range of this address information.
        //        uint8_t mType;

        /// 5.3.2.2 N_SA encodes the network sender address. Valid both for DIAGNOSTICS and REMOTE_DIAGNOSTICS.
        virtual uint32_t getSourceAddress () const = 0;
        virtual void setSourceAddress (uint32_t a) = 0;

        /// 5.3.2.3 N_TA encodes the network target address. Valid both for DIAGNOSTICS and REMOTE_DIAGNOSTICS.
        virtual uint32_t getTargetAddress () const = 0;
        virtual void setTargetAddress (uint32_t a) = 0;

        //        /// N_TAtype
        //        enum class TargetAddressType : uint8_t {
        //                PHYSICAL,  /// 1 to 1 communiaction supported for multiple and single frame communications
        //                FUNCTIONAL /// 1 to n communication (like broadcast?) is allowed only for single frame comms.
        //        };

        //        TargetAddressType targetAddressType;

        //        /// N_AE used optionally to extend both sa and ta.
        //        uint8_t networkAddressExtension;
};

/**
 * @brief The Address class
 */
class Address : public IAddress {
public:
        Address (uint32_t ta, uint32_t sa = 0x0) : targetAddress (ta), sourceAddress (sa) {}

        /// 5.3.2.3 N_TA encodes the network target address. Valid both for DIAGNOSTICS and REMOTE_DIAGNOSTICS.
        uint32_t getTargetAddress () const override { return targetAddress; }
        void setTargetAddress (uint32_t a) override { targetAddress = a; }

        uint32_t getSourceAddress () const override { return sourceAddress; }
        void setSourceAddress (uint32_t a) override { sourceAddress = a; }

private:
        /// 5.3.2.3 N_TA encodes the network target address. Valid both for DIAGNOSTICS and REMOTE_DIAGNOSTICS.
        uint32_t targetAddress = 0x0;

        /// 5.3.2.2 N_SA encodes the network sender address. Valid both for DIAGNOSTICS and REMOTE_DIAGNOSTICS.
        uint32_t sourceAddress = 0x0;
};

template <typename CanFrameT> class NormalAddress29 : public IAddress {
public:
        NormalAddress29 (CanFrameT const &cf) : canFrame{ cf } {}

        /// 5.3.2.2 N_SA encodes the network sender address. Valid both for DIAGNOSTICS and REMOTE_DIAGNOSTICS.
        uint32_t getSourceAddress () const override { return canFrame.getId (); }
        void setSourceAddress (uint32_t a) override
        { /*canFrame.setId (a);*/
        }

        /// 5.3.2.3 N_TA encodes the network target address. Valid both for DIAGNOSTICS and REMOTE_DIAGNOSTICS.
        uint32_t getTargetAddress () const override { return canFrame.getId (); }
        void setTargetAddress (uint32_t a) override
        { /*canFrame.setId (a);*/
        }

private:
        CanFrameT const &canFrame;
};

template <typename CanFrameT> struct NormalAddress29Resolver {
        using AddressType = NormalAddress29<CanFrameT>;

        static NormalAddress29<CanFrameT> create (CanFrameT const &f)
        {
                NormalAddress29 address{ f };
                return address;
        }

        static void apply (IAddress const &a, CanFrameT *f) { f->setId (a.getTargetAddress ()); }
};

/**
 * @brief The TimeProvider struct
 */
struct TimeProvider {
        uint32_t operator() () const { return 0; }
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
 * @brief The InfiniteLoop struct
 */
struct InfiniteLoop {
        [[noreturn]] void operator() (/*Error*/)
        {
                while (true) {
                }
        }
};

/**
 * @brief The EmptyCallback struct
 */
struct EmptyCallback {
        template <typename... T>[[noreturn]] void operator() (T...) {}
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
        bool operator() (CanFrame const &) { return true; }
};

struct CoutPrinter {
        template <typename T> void operator() (T &&a) { std::cout << std::forward<T> (a) << std::endl; }
};

/// NPDU -> Network Protocol Data Unit
enum class IsoNPduType { SINGLE_FRAME = 0, FIRST_FRAME = 1, CONSECUTIVE_FRAME = 2, FLOW_FRAME = 3 };

template <typename CanMessageT> struct CanMessageWrapperBase {
};

template <> class CanMessageWrapperBase<CanFrame> {
public:
        explicit CanMessageWrapperBase (CanFrame const &cf) : frame (cf) {} /// Construct from underlying implementation type.
        template <typename... T> CanMessageWrapperBase (uint32_t id, bool extended, T... data) : frame (id, extended, data...) {}

        template <typename... T> static CanFrame create (uint32_t id, bool extended, T... data) { return CanFrame{ id, extended, data... }; }

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

/// FS field in Flow Control Frame
enum class FlowStatus { CONTINUE_TO_SEND = 0, WAIT = 1, OVERFLOW = 2 };

template <typename CanFrameT> class CanMessageWrapper : public CanMessageWrapperBase<CanFrameT> {
public:
        using CanMessageWrapperBase<CanFrameT>::CanMessageWrapperBase;
        using Base = CanMessageWrapperBase<CanFrameT>;

        /*---------------------------------------------------------------------------*/

        template <typename... T> static CanFrame create (uint32_t id, bool extended, T... data) { return CanFrame{ id, extended, data... }; }

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

/**
 * Buffer for all input and ouptut operations
 */
using IsoMessage = std::vector<uint8_t>;

template <typename CanFrameT, typename IsoMessageT, size_t MAX_MESSAGE_SIZE_T, typename AddressResolverT, typename CanOutputInterfaceT,
          typename TimeProviderT, typename ErrorHandlerT, typename CallbackT>
struct TransportProtocolTraits {
        using CanMessage = CanFrameT;
        using IsoMessageTT = IsoMessageT;
        using CanOutputInterface = CanOutputInterfaceT;
        using TimeProvider = TimeProviderT;
        using ErrorHandler = ErrorHandlerT;
        using Callback = CallbackT;
        using AddressResolver = AddressResolverT;
        static constexpr size_t MAX_MESSAGE_SIZE = MAX_MESSAGE_SIZE_T;
};

/*
 * As in 6.7.1 "Timing parameters". According to ISO 15765-2 it's 1000ms.
 * ISO 15765-4 and J1979 applies further restrictions down to 50ms but it applies to
 * time between query and the response of tester messages. See J1979:2006 chapter 5.2.2.6
 */
static constexpr size_t N_A_TIMEOUT = 1500;
static constexpr size_t N_BS_TIMEOUT = 1500;
static constexpr size_t N_CR_TIMEOUT = 1500;

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
        using CanMessage = typename TraitsT::CanMessage;
        using IsoMessageT = typename TraitsT::IsoMessageTT;
        using CanOutputInterface = typename TraitsT::CanOutputInterface;
        using TimeProvider = typename TraitsT::TimeProvider;
        using ErrorHandler = typename TraitsT::ErrorHandler;
        using Callback = typename TraitsT::Callback;
        using CanMessageWrapperType = CanMessageWrapper<CanMessage>;
        using AddressResolver = typename TraitsT::AddressResolver;

        /// Max allowed by the ISO standard.
        const int MAX_ALLOWED_ISO_MESSAGE_SIZE = 4095;
        /// Max allowed by this implementation. Can be lowered if memory is scarce.
        static constexpr size_t MAX_ACCEPTED_ISO_MESSAGE_SIZE = TraitsT::MAX_MESSAGE_SIZE;

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
         * In ISO this method is called a 'request'
         */
        bool send (Address const &a, IsoMessageT const &msg);

        /**
         * Sends a message. If msg is so long, that it wouldn't fit in a SINGLE_FRAME (6 or 7 bytes
         * depending on addressing used), it is MOVED into the transport protocol object for further
         * processing, and then via run method is send in multiple CONSECUTIVE_FRAMES.
         * In ISO this method is called a 'request'
         */
        bool send (Address const &a, IsoMessageT &&msg);

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
         *
         */
        bool onCanNewFrame (CanMessage const &f) { return onCanNewFrame (CanMessageWrapperType{ f }); }

        /*---------------------------------------------------------------------------*/

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
                enum class State {
                        IDLE,
                        SEND_FIRST_FRAME,
                        RECEIVE_FIRST_FLOW_CONTROL_FRAME,
                        SEND_CONSECUTIVE_FRAME,
                        RECEIVE_BS_FLOW_CONTROL_FRAME,
                        DONE
                };

                StateMachine (CanOutputInterface &o, const Address &a, IsoMessageT const &m) : outputInterface (o), address (a), message (m) {}
                StateMachine (CanOutputInterface &o, const Address &a, IsoMessageT &&m) : outputInterface (o), address (a), message (m) {}
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
                Address address;
                IsoMessageT message;
                State state = State::IDLE;
                size_t bytesSent = 0;
                uint16_t blocksSent = 0;
                uint8_t sequenceNumber = 1;
                uint8_t blockSize = 0;
                uint16_t separationTimeUs = 0;
                Timer separationTimer;
                Timer bsCrTimer;
                uint8_t waitFrameNumber = 0;
        };

        /*---------------------------------------------------------------------------*/

        void confirm (IAddress const &a, Result r) {}
        void firstFrameIndication (IAddress const &a, uint16_t len) {}
        void indication (IAddress const &a, IsoMessageT const &msg, Result r) { callback (msg); /*TODO tidy up this mess with callbacks*/ }

        /*---------------------------------------------------------------------------*/

        uint32_t getID (bool extended) const;
        bool sendFlowFrame (const CanMessageWrapperType &msgBuffer, FlowStatus fs = FlowStatus::CONTINUE_TO_SEND);
        bool sendSingleFrame (const Address &a, IsoMessageT const &msg);
        bool sendMultipleFrames (const Address &a, IsoMessageT const &msg);
        bool sendMultipleFrames (const Address &a, IsoMessageT &&msg);

        TransportMessageList stack;
        /// Tells if CAN frames are extended (29bit ID), or standard (11bit ID).
        bool extendedFrame = false;
        /// Tells if ISO 15765-2 extended addressing is used or not. Not do be confused with extended CAN frames.
        bool canExtAddrActive = false;
        uint8_t canExtAddr = 0;

        Callback callback;
        CanOutputInterface outputInterface;
        ErrorHandler errorHandler;
        std::optional<StateMachine> stateMachine;
};

/*****************************************************************************/

template <typename CanFrameT = CanFrame, typename IsoMessageT = IsoMessage, size_t MAX_MESSAGE_SIZE = 4095,
          typename AddressResolverT = NormalAddress29Resolver<CanMessageWrapper<CanFrameT>>,
          typename CanOutputInterfaceT = LinuxCanOutputInterface, typename TimeProviderT = TimeProvider, typename ErrorHandlerT = InfiniteLoop,
          typename CallbackT = CoutPrinter>
TransportProtocol<TransportProtocolTraits<CanFrameT, IsoMessageT, MAX_MESSAGE_SIZE, AddressResolverT, CanOutputInterfaceT, TimeProviderT,
                                          ErrorHandlerT, CallbackT>>
create (CallbackT callback, CanOutputInterfaceT outputInterface = {}, TimeProviderT timeProvider = {}, ErrorHandlerT errorHandler = {})
{
        return { callback, outputInterface, timeProvider, errorHandler };
}

/*****************************************************************************/

template <typename TraitsT> bool TransportProtocol<TraitsT>::send (const Address &a, IsoMessageT const &msg)
{
        if (msg.size () > MAX_ACCEPTED_ISO_MESSAGE_SIZE) {
                return false;
        }

        // 6 or 7 depending on addressing used
        const size_t SINGLE_FRAME_MAX_SIZE = 7 - int(isCanExtAddrActive ());

        if (msg.size () <= SINGLE_FRAME_MAX_SIZE) { // Send using single Frame
                return sendSingleFrame (a, msg);
        }
        else { // Send using multiple frames, state machine, and timing controll and whatnot.
                return sendMultipleFrames (a, msg);
        }

        return true;
}

/*****************************************************************************/

template <typename TraitsT> bool TransportProtocol<TraitsT>::send (const Address &a, IsoMessageT &&msg)
{
        if (msg.size () > MAX_ACCEPTED_ISO_MESSAGE_SIZE) {
                return false;
        }

        // 6 or 7 depending on addressing used
        const size_t SINGLE_FRAME_MAX_SIZE = 7 - int(isCanExtAddrActive ());

        if (msg.size () <= SINGLE_FRAME_MAX_SIZE) { // Send using single Frame
                return sendSingleFrame (a, msg);
        }
        else { // Send using multiple frames, state machine, and timing controll and whatnot.
                return sendMultipleFrames (a, msg);
        }

        return true;
}

/*****************************************************************************/

template <typename TraitsT> bool TransportProtocol<TraitsT>::sendSingleFrame (const Address &a, IsoMessageT const &msg)
{
        CanMessageWrapperType canFrame{ 0x00, true, int(IsoNPduType::SINGLE_FRAME) | (msg.size () & 0x0f) };
        AddressResolver::apply (a, &canFrame);

        for (size_t i = 0; i < msg.size (); ++i) {
                canFrame.set (i + 1, msg.at (i));
        }

        canFrame.setDlc (1 + msg.size ());
        return outputInterface (canFrame.value ());
}

/*****************************************************************************/

template <typename TraitsT> bool TransportProtocol<TraitsT>::sendMultipleFrames (const Address &a, IsoMessageT const &msg)
{
        stateMachine = StateMachine{ outputInterface, a, msg };
        return true;
}

// TODO I wouldn't multiply those methods but rather use perfect forwarding magic (?)
template <typename TraitsT> bool TransportProtocol<TraitsT>::sendMultipleFrames (const Address &a, IsoMessageT &&msg)
{
        stateMachine = StateMachine{ outputInterface, a, msg };
        return true;
}

/*****************************************************************************/

template <typename TraitsT> bool TransportProtocol<TraitsT>::onCanNewFrame (const CanMessageWrapperType &frame)
{
        // Address as received in the CAN frame frame.
        typename AddressResolver::AddressType address = AddressResolver::create (frame);

        switch (frame.getType ()) {
        case IsoNPduType::SINGLE_FRAME: {
                TransportMessage message;
                int singleFrameLen = frame.getDataLengthS ();

                // Error situation. Such frames should be ignored according to 6.5.2.2 page 24.
                if (singleFrameLen <= 0 || (isCanExtAddrActive () && singleFrameLen > 6) || singleFrameLen > 7) {
                        return false;
                }

                // TODO Proper addressing should be used.
                TransportMessage *isoMessage = stack.findMessage (frame.getId ());

                if (isoMessage != nullptr) {
                        // As in 6.7.3 Table 18
                        // TODO this API is inefficient, we create empty IsoMessageT objecyt (which possibly can allocate as much as 4095B) only
                        // to discard it.
                        indication (address, {}, Result::N_UNEXP_PDU);
                        // Terminate the current reception of segmented message.
                        stack.removeMessage (isoMessage);
                        break;
                }

                uint8_t dataOffset = frame.getNPciOffset () + 1;
                message.append (frame, dataOffset, singleFrameLen);
                indication (address, message.data, Result::N_OK);
        } break;

        case IsoNPduType::FIRST_FRAME: {
                uint16_t multiFrameRemainingLen = frame.getDataLengthF ();

                // Error situation. Such frames should be ignored according to ISO.
                if (multiFrameRemainingLen < 8 || (isCanExtAddrActive () && multiFrameRemainingLen < 7)) {
                        return false;
                }

                // 6.5.3.3 Error situation : too much data. Should reply with apropriate flow control frame.
                if (multiFrameRemainingLen > MAX_ACCEPTED_ISO_MESSAGE_SIZE || multiFrameRemainingLen > MAX_ALLOWED_ISO_MESSAGE_SIZE) {
                        sendFlowFrame (frame, FlowStatus::OVERFLOW);
                        return false;
                }

                // TODO Proper addressing should be used.
                TransportMessage *isoMessage = stack.findMessage (frame.getId ());

                if (isoMessage) {
                        // As in 6.7.3 Table 18
                        indication (address, {}, Result::N_UNEXP_PDU);
                        // Terminate the current reception of segmented message.
                        stack.removeMessage (isoMessage);
                }

                int firstFrameLen = (isCanExtAddrActive ()) ? (5) : (6);
                isoMessage = stack.addMessage (frame.getId () /*, multiFrameRemainingLen*/);

                if (!isoMessage) {
                        indication (address, {}, Result::N_MESSAGE_NUM_MAX);
                        return false;
                }

                firstFrameIndication (address, multiFrameRemainingLen);

                isoMessage->currentSn = 1;
                isoMessage->multiFrameRemainingLen = multiFrameRemainingLen - firstFrameLen;
                isoMessage->timer.start (N_BS_TIMEOUT);
                uint8_t dataOffset = frame.getNPciOffset () + 2;
                isoMessage->append (frame, dataOffset, firstFrameLen);

                // Send Flow Control
                if (!sendFlowFrame (frame, FlowStatus::CONTINUE_TO_SEND)) {
                        indication (address, {}, Result::N_ERROR);
                        // Terminate the current reception of segmented message.
                        stack.removeMessage (isoMessage);
                }

                return true;
        } break;

        case IsoNPduType::CONSECUTIVE_FRAME: {
                TransportMessage *isoMessage = stack.findMessage (frame.getId ());

                if (!isoMessage) {
                        // As in 6.7.3 Table 18 - ignore
                        return false;
                }

                isoMessage->timer.start (N_CR_TIMEOUT);

                if (frame.getSerialNumber () != isoMessage->currentSn) {
                        // 6.5.4.3 SN error handling
                        indication (address, {}, Result::N_WRONG_SN);
                        return false;
                }

                ++(isoMessage->currentSn);
                isoMessage->currentSn %= 16;
                int maxConsecutiveFrameLen = (isCanExtAddrActive ()) ? (6) : (7);
                int consecutiveFrameLen = std::min (maxConsecutiveFrameLen, isoMessage->multiFrameRemainingLen);
                isoMessage->multiFrameRemainingLen -= consecutiveFrameLen;

                uint8_t dataOffset = frame.getNPciOffset () + 1;
                isoMessage->append (frame, dataOffset, consecutiveFrameLen);

                if (isoMessage->multiFrameRemainingLen) {
                        return true;
                }
                else {
                        indication (address, isoMessage->data, Result::N_OK);
                        stack.removeMessage (isoMessage);
                }

        } break;

        case IsoNPduType::FLOW_FRAME: {
                if (!stateMachine) { // No segmented transmission active
                        break;       // Ignore
                }

                stateMachine->run (&frame);
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
        TransportMessage *ptr = stack.getFirst ();

        while (ptr) {
                if (ptr->timer.isExpired ()) {
                        // TODO errorHandler (Error::N_TIMEOUT);
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

/*****************************************************************************/

// TODO addresses are messed up, whole addressing thing should be reimplemented, checked.
template <typename TraitsT> bool TransportProtocol<TraitsT>::sendFlowFrame (CanMessageWrapperType const &frame, FlowStatus fs)
{
        if (extendedFrame) {
                /*
                 * BlockSize == 0, which means indefinite block size, and STMin == 0 which means
                 * minimum delay between consecutive frames
                 */
                CanMessageWrapperType ctrlData (0x18DA00F1 | ((frame.getId () & 0xFF) << 8), true, 0x30 | uint8_t (fs));
                ctrlData.set (1, 0);
                ctrlData.set (2, 0);
                return outputInterface (ctrlData.value ());
        }
        else {
                CanMessageWrapperType ctrlData (0x7E0 | (frame.getId () & 0x07), false, 0x30 | uint8_t (fs));
                ctrlData.set (1, 0);
                ctrlData.set (2, 0);
                return outputInterface (ctrlData.value ());
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
                // errorHandler (Error::CRITICAL_ALGORITHM);
                return nullptr;
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
        if (state != State::IDLE && state != State::SEND_FIRST_FRAME && bsCrTimer.isExpired ()) {
                if (state == State::RECEIVE_BS_FLOW_CONTROL_FRAME || state == State::RECEIVE_FIRST_FLOW_CONTROL_FRAME) {
                        // confirm (address, Result::N_TIMEOUT_BS);
                }
                else {
                        // TODO confirm ()Result::N_TIMEOUT_Cr
                }

                state = State::DONE;
        }

        uint16_t isoMessageSize = message.size ();

        switch (state) {
        case State::IDLE:
                state = State::SEND_FIRST_FRAME;
                break;

        case State::SEND_FIRST_FRAME: {
                // TODO addressing
                CanMessageWrapperType canFrame (0x00, true, (int(IsoNPduType::FIRST_FRAME) << 4) | (isoMessageSize & 0xf00) >> 8,
                                                (isoMessageSize & 0x0ff));
                AddressResolver::apply (address, &canFrame);

                int toSend = std::min<int> (isoMessageSize, 6);
                for (int i = 0; i < toSend; ++i) {
                        canFrame.set (i + 2, message.at (i));
                }

                canFrame.setDlc (2 + toSend);

                if (!outputInterface (canFrame.value ())) {
                        // confirm Result::N_TIMEOUT_A
                        state = State::DONE;
                        break;
                }

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
                auto addressRx = AddressResolver::create (*frame);

                // TODO I don't know if thaths OK:
                if (addressRx.getTargetAddress () != address.getSourceAddress ()) {
                        break;
                }

                IsoNPduType type = frame->getType ();

                if (type != IsoNPduType::FLOW_FRAME) {
                        break;
                }

                FlowStatus fs = frame->getFlowStatus ();

                if (fs != FlowStatus::CONTINUE_TO_SEND && fs != FlowStatus::WAIT && fs != FlowStatus::OVERFLOW) {
                        // TODO confirm (Address{}, Result::N_INVALID_FS); // 6.5.5.3
                        state = State::DONE; // abort
                }

                if (fs == FlowStatus::OVERFLOW) {
                        // TODO confirm (Address{}, Result::N_BUFFER_OVFLW);
                        state = State::DONE; // abort
                }

                if (fs == FlowStatus::WAIT) {
                        bsCrTimer.start (N_BS_TIMEOUT);
                        // TODO bsTimer
                        ++waitFrameNumber;

                        if (waitFrameNumber >= MAX_WAIT_FRAME_NUMBER) { // In case of MAX_WAIT_FRAME_NUMBER == 0 message will be aborted
                                                                        // immediately, which is fine according to the ISO.
                                // TODO confirm (Address{}, Result::N_WFT_OVRN);
                                state = State::DONE; // abort
                        }

                        break; // state stays at RECEIVE_*_FLOW_CONTROL_FRAME
                }

                if (state == State::RECEIVE_FIRST_FLOW_CONTROL_FRAME) {
                        blockSize = frame->get (1); // 6.5.5.4 page 21
                        separationTimeUs = frame->get (2);

                        if (separationTimeUs >= 0 && separationTimeUs <= 0x7f) {
                                separationTimeUs *= 1000; // Convert to µs
                        }
                        else if (separationTimeUs >= 0xf1 && separationTimeUs <= 0xf9) {
                                separationTimeUs = (separationTimeUs - 0xf0) * 100;
                        }
                        else {
                                separationTimeUs = 0x7f * 1000; // 6.5.5.6 ST error handling
                        }
                }

                waitFrameNumber = 0;
                separationTimer.start (0);
                state = State::SEND_CONSECUTIVE_FRAME;
                bsCrTimer.start (N_CR_TIMEOUT);
        } break;

        case State::SEND_CONSECUTIVE_FRAME: {
                if (!separationTimer.isExpired ()) {
                        break;
                }

                CanMessageWrapperType canFrame (0x00, true, (int(IsoNPduType::CONSECUTIVE_FRAME) << 4) | sequenceNumber);
                AddressResolver::apply (address, &canFrame);

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

                if (bytesSent >= message.size ()) {
                        state = State::DONE;
                        break;
                }

                if (blockSize && blocksSent++ >= blockSize) {
                        state = State::RECEIVE_BS_FLOW_CONTROL_FRAME;
                        bsCrTimer.start (N_BS_TIMEOUT);
                        break;
                }

                separationTimer.start (separationTimeUs);
                break;

        } break;

        default:
                break;
        }
} // namespace tp

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
