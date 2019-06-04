/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "CanFrame.h"
#include "ITransportProtocolCallback.h"
#include "TransportMessage.h"
#include "Types.h"
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>

// TODO add short namespace

/**
 * Impl details :
 * - Can interface (Can link layer)
 *  - setCanCallback - delete
 *  - send
 *  - setFilterAndMask
 * - CanFrame interface
 * - Timer
 * - Error handling
 *
 * - Buffer type - dynamic versus static allocation
 */

struct TimeProvider {
        uint32_t operator() () const { return 0; }
};

enum class Error : uint32_t {
        N_NO_ERROR = 0,              /// No error.
        N_WRONG_SN = 0x40000000,     /// Zły serial number w Consecutive Frame.
        N_BUFFER_OVFLW = 0x20000000, /// Przekroczony rozmiar danych które ktoś nam przysyła
        N_TIMEOUT = 0x10000000,      /// 6.7.2 Any N_PDU not transmitted in time on the sender side.
        N_UNEXP_PDU = 0x08000000,
        N_MESSAGE_NUM_MAX = 0x04000000, /// Za dużo wiadomości ISO (długich) na raz. To może oznaczać więcej
        /// niż MAX_MESSAGE_NUM ECU nadającychjednocześnie
        NOT_ISO_15765_4 = 0x02000000, /// Not ISO compliant
        CRITICAL_ALGORITHM
};

struct InfiniteLoop {
        void operator() (Error e)
        {
                while (true) {
                }
        }
};

struct LinuxCanOutputInterface {
        bool operator() (CanFrame const &cf) { return true; }
};

struct CoutPrinter {
        template <typename T> void operator() (T &&a) { std::cout << std::forward<T> (a) << std::endl; }
};

template <typename CanMessageT> struct CanMessageWrapper {
};

template <> class CanMessageWrapper<CanFrame> {
public:
        explicit CanMessageWrapper (CanFrame const &cf) : frame (cf) {} /// Construct from underlying implementation type.

        CanMessageWrapper (uint32_t id, bool extended, uint8_t dlc, uint8_t data0 = 0x55) : frame (id, extended, dlc, data0) {}

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
 * Implementuje  ISO-TP, czyli ISO 15765-2 ze szczególnym nacisiekiem na OBD, a więc nie
 * implementuje niektórych rzeczy, które do OBD nie są potrzebne. ISO-TP jest bowiem wykorzystywane
 * także w komunikacji ECU-ECU wewnątrz pojazdów (nie tylko do diagnostyki).
 * Dokumentacja:
 * - http://iwasz.pl/private/index.php/STM32_CAN#ISO_15765-2
 * - ISO-15765-2-2004.pdf rozdział 6 "Network Layer Protocol"
 * - https://en.wikipedia.org/wiki/ISO_15765-2
 * - http://canbushack.com/iso-15765-2/
 * - http://iwasz.pl/private/index.php/STM32_CAN#ISO_15765-2
 */
template <typename CanFrameT = CanFrame, typename CanOutputInterfaceT = LinuxCanOutputInterface, typename TimeProviderT = TimeProvider,
          typename ErrorHandlerT = InfiniteLoop, typename CallbackT = CoutPrinter>
class TransportProtocol {
public:
        using CanMessageWrapperType = CanMessageWrapper<CanFrameT>;

        /// NPDU -> Network Protocol Data Unit
        enum IsoNPduType { CAN_SINGLE_FRAME = 0, CAN_FIRST_FRAME = 1, CAN_CONSECUTIVE_FRAME = 2, CAN_FLOW_CONTROL_FRAME = 3 };

        /*
         * As in 6.7.1 "Timing parameters". According to ISO 15765-2 it's 1000ms.
         * ISO 15765-4 and J1979 applies further restrictions down to 50ms but it applies to
         * time between query and the response of tester messages. See J1979:2006 chapter 5.2.2.6
         */
        enum Timeouts { P2_TIMEOUT = 50, N_A_TIMEOUT = 1000, N_B_TIMEOUT = 1000, N_C_TIMEOUT = 1000 };
        /// Max allowed by the ISO standard.
        const int MAX_ALLOWED_ISO_MESSAGE_SIZE = 4095;
        /// Max allowed by this implementation. Can be lowered if memory is scarce.
        const int MAX_ACCEPTED_ISO_MESSAGE_SIZE = 4095;
        /// FS field in Flow Control Frame
        enum FlowStatus { STATUS_CONTINUE_TO_SEND = 0, STATUS_WAIT = 1, STATUS_OVERFLOW = 2 };

        TransportProtocol (CallbackT callback, CanOutputInterfaceT outputInterface = {}, TimeProviderT timeProvider = {},
                           ErrorHandlerT errorHandler = {})
            : stack{ errorHandler },
              callback{ callback },
              outputInterface{ outputInterface },
              timeProvider{ timeProvider },
              errorHandler{ errorHandler }
        {
        }

        TransportProtocol (TransportProtocol const &) = delete;
        TransportProtocol (TransportProtocol &&) = delete;
        TransportProtocol &operator= (TransportProtocol const &) = delete;
        TransportProtocol &operator= (TransportProtocol &&) = delete;
        ~TransportProtocol () = default;

        /**
         * Sending is curently limited to 1 CAN frame which means, that we can send only 7 bytes if
         * ISO 15765-2 extended addressing if off, or 6 when its on. When extended addressing is active,
         * first byte of CAN payload is used for the extended address, and second is used for PCI (Protocol
         * Control Information).
         */
        bool request (Buffer const &msg);

        /**
         * Connection parameters are beeing figured out here according to procedures described in
         * ISO 15765-4 chapter 4. CAN connection properties like baudrate and number of bits in ID
         * are tried one and one and after successful communication we now what setting to use.
         */
        //        bool connect ();

        void setCanExtAddr (uint8_t u);
        uint8_t getCanExtAddr () const { return canExtAddr; }
        bool isCanExtAddrActive () const { return canExtAddrActive; }

        /// Checks for timeouts.
        void checkTimeouts ();

        /// If connected.
        //        bool isConnected () const { return connected; }

        /*****************************************************************************/
        /* Timer class                                                               */
        /*****************************************************************************/

        static uint32_t getTick ()
        {
                static TimeProvider tp;
                return tp ();
        }

        /**
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
                uint32_t getTick () { return getTick (); }

        protected:
                uint32_t startTime = 0;
                uint32_t intervalMs = 0;
        };

        /*****************************************************************************/
        /* Timer class                                                               */
        /*****************************************************************************/

        /// Messages composed from CAN frames.
        struct TransportMessage {

                //                TransportMessage &operator= (TransportMessage &&t)
                //                {
                //                        this->address = t.address;
                //                        this->data = std::move (t.data);
                //                        this->prev return *this;
                //                }

                int append (CanMessageWrapperType const &frame, size_t offset, size_t len);

                // TODO ifdef debug or sth
                std::ostream &operator<< (std::ostream &o) const
                {
                        o << "TransportMessage addr = " << address << ", data = " << data;
                        return o;
                }

                uint32_t address = 0; /// Address Information M_AI
                Buffer data;          /// Max 4095 (according to ISO 15765-2) or less if more strict requirements programmed by the user.
                TransportMessage *prev = nullptr; /// Double linked list implementation.
                TransportMessage *next = nullptr; /// Double linked list implementation.
                int multiFrameRemainingLen = 0;   /// For tracking number of bytes remaining.
                int currentSn = 0;                /// Serial number of Consecutive Frame.
                Timer timer;                      /// For tracking time between first and consecutive frames with the same address.
        };

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
        void onCanNewFrame (CanFrame const &f) { onCanNewFrame (CanMessageWrapperType{ f } /*, nullptr*/); }
        void onCanError (uint32_t e);

private:
        bool onCanNewFrame (CanMessageWrapperType const &frame);

        /// Double linked list wiadomości ISO 15765-2 (tych długich do 4095).
        class TransportMessageList {
        public:
                enum { MAX_MESSAGE_NUM = 8 };
                TransportMessageList (ErrorHandlerT &e) : first (nullptr), size (0), errorHandler (e) {}

                TransportMessage *findMessage (uint32_t address);
                TransportMessage *addMessage (uint32_t address, size_t bufferSize);
                TransportMessage *removeMessage (TransportMessage *m);
                size_t getSize () const { return size; }
                TransportMessage *getFirst () { return first; }

        private:
                TransportMessage *first;
                size_t size;
                ErrorHandlerT &errorHandler;
        };

        uint32_t getID (bool extended) const;
        //        void setFilterAndMask (bool extended);
        void processFlowFrame (const CanMessageWrapperType &msgBuffer, FlowStatus fs = STATUS_CONTINUE_TO_SEND);

        //        enum ConnectionState { CONNECTED, SEND_FUNCTIONAL_11, WAIT_REPLY_11, SEND_FUNCTIONAL_29, WAIT_REPLY_29, NOT_ISO_COMPLIANT };
        //        ConnectionState connectSingleBaudrate ();

        TransportMessageList stack;
        /// Tells if CAN frames are extended (29bit ID), or standard (11bit ID).
        bool extendedFrame = false;
        /// Tells if ISO 15765-2 extended addressing is used or not. Not do be confused with extended CAN frames.
        bool canExtAddrActive = false;
        //        bool connected = false;
        uint8_t canExtAddr = 0;

        CallbackT callback;
        CanOutputInterfaceT outputInterface;
        TimeProviderT timeProvider;
        ErrorHandlerT errorHandler;
};

/*****************************************************************************/

// TODO to trzeba zaimplementować od nowa
template <typename CanFrameT, typename CanOutputInterfaceT, typename TimeProviderT, typename ErrorHandlerT, typename CallbackT>
bool TransportProtocol<CanFrameT, CanOutputInterfaceT, TimeProviderT, ErrorHandlerT, CallbackT>::request (Buffer const &msg)
{
        if ((isCanExtAddrActive () && (msg.size () > 6)) || (msg.size () > 7)) {
                return false;
        }

        int totalLen = msg.size () + 1; // Data size + 1 byte for 'Single Frame' PCI.
        // CanFrame canFrame (getID (extendedFrame), extendedFrame, 8, 0);

        CanMessageWrapperType canFrame;

        int index = 0;

        // Another +1 byte for extended address.
        if (isCanExtAddrActive ()) {
                ++totalLen;
                canFrame.data[index++] = getCanExtAddr ();
        }

        canFrame.data[index++] = msg.size ();
        //        memcpy (canFrame.data + index, msg.data (), msg.size ());

        for (size_t i = 0; i < msg.size (); ++i) {
                canFrame.at (index + i) = msg[i];
        }

        if (!outputInterface (canFrame)) {
                return false;
        }

        return true;
}

/*****************************************************************************/

template <typename CanFrameT, typename CanOutputInterfaceT, typename TimeProviderT, typename ErrorHandlerT, typename CallbackT>
void TransportProtocol<CanFrameT, CanOutputInterfaceT, TimeProviderT, ErrorHandlerT, CallbackT>::onCanError (uint32_t e)
{
        errorHandler (e);
}

/*****************************************************************************/

template <typename CanFrameT, typename CanOutputInterfaceT, typename TimeProviderT, typename ErrorHandlerT, typename CallbackT>
bool TransportProtocol<CanFrameT, CanOutputInterfaceT, TimeProviderT, ErrorHandlerT, CallbackT>::onCanNewFrame (
        const CanMessageWrapperType &frame /*, TransportMessage *outMessage*/)
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
        uint8_t npduType = (nPciByte1 & 0xF0) >> 4;

        switch (npduType) {
        case CAN_SINGLE_FRAME: {
                TransportMessage message;
                int singleFrameLen = nPciByte1; // Nie trzeba maskować z 0x0F, bo N_PCItype == 0.
                message.data.resize (singleFrameLen);

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
                // std::copy (frame.data + dataOffset, frame.data + dataOffset + singleFrameLen, message.data.begin ());
                message.append (frame, dataOffset, singleFrameLen);

                //                if (outMessage) {
                //                        *outMessage = std::move (message);
                //                }
                //                else {
                //                        callback (std::move (message));
                callback (message);
                //                }
        } break;

        case CAN_FIRST_FRAME: {
                uint16_t multiFrameRemainingLen = ((nPciByte1 & 0x0f) << 8) | frame.get (nPciOffset + 1);

                // Error situation. Such frames should be ignored according to ISO.
                if (multiFrameRemainingLen < 8 || (isCanExtAddrActive () && multiFrameRemainingLen < 7)) {
                        return false;
                }

                // Error situation : too much data. Should reply with apropriate flow control frame.
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
                isoMessage = stack.addMessage (frame.getId (), multiFrameRemainingLen);

                if (!isoMessage) {
                        errorHandler (Error::N_MESSAGE_NUM_MAX);
                        return false;
                }

                isoMessage->currentSn = 1;
                isoMessage->multiFrameRemainingLen = multiFrameRemainingLen - firstFrameLen;
                isoMessage->timer.start (N_B_TIMEOUT);
                uint8_t dataOffset = nPciOffset + 2;
                // std::copy (frame.data + dataOffset, frame.data + dataOffset + firstFrameLen, std::back_inserter (isoMessage->data));
                isoMessage->append (frame, dataOffset, firstFrameLen);

                // Send Flow Control
                processFlowFrame (frame, STATUS_CONTINUE_TO_SEND);
                return true;
        } break;

        case CAN_CONSECUTIVE_FRAME: {
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
                        //                        if (outMessage) {
                        //                                *outMessage = std::move (*isoMessage);
                        //                        }
                        //                        else {
                        // callback (std::move (*isoMessage));
                        callback (*isoMessage);
                        //                        }
                        stack.removeMessage (isoMessage);
                }
        } break;

        default:
                break;
        }

        return false;
}

/*****************************************************************************/
#if 0
bool Iso15765TransportProtocol::connect ()
{
        // 500kbps
        driver->setBaudratePrescaler (6);
        Debug::singleton ()->print ("500kbps\n");

        if (connectSingleBaudrate () == CONNECTED) {
                return true;
        }

        // 250kbps
        driver->setBaudratePrescaler (12);
        Debug::singleton ()->print ("250kbps\n");

        if (connectSingleBaudrate () == CONNECTED) {
                return true;
        }
        else {
                driver->disable ();
                return false;
        }
}

/*****************************************************************************/

Iso15765TransportProtocol::ConnectionState Iso15765TransportProtocol::connectSingleBaudrate ()
{
        driver->interrupts (false);
        ConnectionState currentState = SEND_FUNCTIONAL_11;
        int retry = 1;

        while (currentState != CONNECTED && currentState != NOT_ISO_COMPLIANT) {
                switch (currentState) {
                case SEND_FUNCTIONAL_11: // Connector A
                        setFilterAndMask (false);
                        Debug::singleton ()->print ("send11\n");
                        if (!driver->send (CanFrame (getID (false), false, 8, 0x02, 0x01, 0x00), 100)) { // TODO timeout według ISO
                                currentState = NOT_ISO_COMPLIANT;
                                Debug::singleton ()->print ("11 not iso\n");
                        }
                        else {
                                currentState = WAIT_REPLY_11;
                                Debug::singleton ()->print ("wait for 11 reply\n");
                        }
                        break;

                case WAIT_REPLY_11: { // Connector B
                        TransportMessage reply;
                        CanFrame f;

                        do {
                                f = driver->read (P2_TIMEOUT);

                                // Timeout już podczas odbierania pierwszej ramki.
                                if (!f) {
                                        currentState = SEND_FUNCTIONAL_29; // Go to C.
                                        break;
                                }
                        } while (onCanNewFrame (f, &reply));

                        if (currentState == SEND_FUNCTIONAL_29) {
                                Debug::singleton ()->print ("11 reply timeout. try 29\n");
                                break;
                        }

                        Debug::singleton ()->print ("11 reply OK\n");

                        if (reply.data[0] == 0x41) { // Positive respone
                                Debug::singleton ()->print ("11 reply positive\n");
                                currentState = CONNECTED;
                                extendedFrame = false;
                        }
                        else { // Negative reponse 0x21
                                Debug::singleton ()->print ("11 reply negative\n");
                                if (++retry >= 6) {
                                        currentState = NOT_ISO_COMPLIANT;
                                        Debug::singleton ()->print ("not iso\n");
                                }
                                else {
                                        HAL_Delay (200);
                                        currentState = SEND_FUNCTIONAL_11;
                                        Debug::singleton ()->print ("11 retry\n");
                                }
                        }
                } break;

                case SEND_FUNCTIONAL_29: // Connector C
                        setFilterAndMask (true);
                        retry = 1;
                        if (!driver->send (CanFrame (getID (true), true, 8, 0x02, 0x01, 0x00), 100)) { // TODO timeout według ISO
                                currentState = NOT_ISO_COMPLIANT;
                        }
                        else {
                                currentState = WAIT_REPLY_29;
                        }
                        break;

                case WAIT_REPLY_29: { // Connector D
                        TransportMessage reply;
                        CanFrame f;

                        do {
                                f = driver->read (P2_TIMEOUT);

                                // Timeout już podczas odbierania pierwszej ramki.
                                if (!f) {
                                        currentState = NOT_ISO_COMPLIANT; // Go to C.
                                        break;
                                }
                        } while (onCanNewFrame (f, &reply));

                        if (currentState == NOT_ISO_COMPLIANT) {
                                break;
                        }

                        if (reply.data[0] == 0x41) { // Positive respone
                                currentState = CONNECTED;
                                extendedFrame = true;
                        }
                        else { // Negative reponse 0x21
                                if (++retry >= 6) {
                                        currentState = NOT_ISO_COMPLIANT;
                                }
                                else {
                                        HAL_Delay (200);
                                        currentState = SEND_FUNCTIONAL_29;
                                }
                        }
                } break;

                case NOT_ISO_COMPLIANT: // Connector F
                default:
                        connected = false;
                        break;
                }
        }

        connected = (currentState == CONNECTED);

        if (connected) {
                driver->interrupts (true);
        }

        return currentState;
}
#endif
/*****************************************************************************/

template <typename CanFrameT, typename CanOutputInterfaceT, typename TimeProviderT, typename ErrorHandlerT, typename CallbackT>
void TransportProtocol<CanFrameT, CanOutputInterfaceT, TimeProviderT, ErrorHandlerT, CallbackT>::checkTimeouts ()
{
        TransportMessage *ptr = stack.getFirst ();

        while (ptr) {
                if (ptr->timer.isExpired ()) {
                        errorHandler (Error::N_TIMEOUT);
                        ptr = stack.removeMessage (ptr);
                        continue;
                }

                ptr = ptr->next;
        }
}

/*****************************************************************************/

template <typename CanFrameT, typename CanOutputInterfaceT, typename TimeProviderT, typename ErrorHandlerT, typename CallbackT>
void TransportProtocol<CanFrameT, CanOutputInterfaceT, TimeProviderT, ErrorHandlerT, CallbackT>::setCanExtAddr (uint8_t u)
{
        canExtAddrActive = true;
        canExtAddr = u;
}

/*****************************************************************************/

template <typename CanFrameT, typename CanOutputInterfaceT, typename TimeProviderT, typename ErrorHandlerT, typename CallbackT>
uint32_t TransportProtocol<CanFrameT, CanOutputInterfaceT, TimeProviderT, ErrorHandlerT, CallbackT>::getID (bool extended) const
{
        // ISO 15765-4:2005 chapter 6.3.2.2 & 6.3.2.3
        return (extended) ? (0x18db33f1) : (0x7df);
}

// template <typename CanFrameT, typename CanOutputInterfaceT, typename TimeProviderT, typename ErrorHandlerT, typename CallbackT>
// void TransportProtocol<CanFrameT, CanOutputInterfaceT, TimeProviderT, ErrorHandlerT, CallbackT>::setFilterAndMask (bool extended)
//{
//        if (extended) {
//                driver->setFilterAndMask (0x18DAF100, 0x1FFFFF00, extended);
//        }
//        else {
//                driver->setFilterAndMask (0x7E8, 0x7F8, extended);
//        }
//}

/*****************************************************************************/

template <typename CanFrameT, typename CanOutputInterfaceT, typename TimeProviderT, typename ErrorHandlerT, typename CallbackT>
void TransportProtocol<CanFrameT, CanOutputInterfaceT, TimeProviderT, ErrorHandlerT, CallbackT>::processFlowFrame (
        CanMessageWrapperType const &frame, FlowStatus fs)
{
        if (extendedFrame) {
                /*
                 * BlockSize == 0, which means indefinite block size, and STMin == 0 which means
                 * minimum delay between consecutive frames
                 */
                CanMessageWrapperType ctrlData (0x18DA00F1 | ((frame.getId () & 0xFF) << 8), true, 8, 0x30 | fs);
                ctrlData.set (1, 0);
                ctrlData.set (2, 0);
                outputInterface (ctrlData.value ());
        }
        else {
                CanMessageWrapperType ctrlData (0x7E0 | (frame.getId () & 0x07), false, 8, 0x30 | fs);
                ctrlData.set (1, 0);
                ctrlData.set (2, 0);
                outputInterface (ctrlData.value ());
        }
}

/*****************************************************************************/

template <typename CanFrameT, typename CanOutputInterfaceT, typename TimeProviderT, typename ErrorHandlerT, typename CallbackT>
typename TransportProtocol<CanFrameT, CanOutputInterfaceT, TimeProviderT, ErrorHandlerT, CallbackT>::TransportMessage *
TransportProtocol<CanFrameT, CanOutputInterfaceT, TimeProviderT, ErrorHandlerT, CallbackT>::TransportMessageList::findMessage (uint32_t address)
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

template <typename CanFrameT, typename CanOutputInterfaceT, typename TimeProviderT, typename ErrorHandlerT, typename CallbackT>
typename TransportProtocol<CanFrameT, CanOutputInterfaceT, TimeProviderT, ErrorHandlerT, CallbackT>::TransportMessage *
TransportProtocol<CanFrameT, CanOutputInterfaceT, TimeProviderT, ErrorHandlerT, CallbackT>::TransportMessageList::addMessage (uint32_t address,
                                                                                                                              size_t bufferSize)
{
        if (size++ >= MAX_MESSAGE_NUM) {
                return nullptr;
        }

        TransportMessage *m = new TransportMessage ();
        m->address = address;
        m->data.reserve (bufferSize);
        m->data.clear ();

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

template <typename CanFrameT, typename CanOutputInterfaceT, typename TimeProviderT, typename ErrorHandlerT, typename CallbackT>
typename TransportProtocol<CanFrameT, CanOutputInterfaceT, TimeProviderT, ErrorHandlerT, CallbackT>::TransportMessage *
TransportProtocol<CanFrameT, CanOutputInterfaceT, TimeProviderT, ErrorHandlerT, CallbackT>::TransportMessageList::removeMessage (
        TransportMessage *m)
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

template <typename CanFrameT, typename CanOutputInterfaceT, typename TimeProviderT, typename ErrorHandlerT, typename CallbackT>
int TransportProtocol<CanFrameT, CanOutputInterfaceT, TimeProviderT, ErrorHandlerT, CallbackT>::TransportMessage::append (
        CanMessageWrapperType const &frame, size_t offset, size_t len)
{
        size_t outputIndex = data.size ();
        for (size_t inputIndex = offset; inputIndex < len; ++inputIndex, ++outputIndex) {
                data[outputIndex] = frame.get (inputIndex);
        }
}
