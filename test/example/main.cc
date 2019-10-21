/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "TransportProtocol.h"
#include <iostream>

void receivingAsyncCallback ()
{
        auto tp = tp::create (
                tp::Address (0x12, 0x34), [] (auto &&tm) { std::cout << "TransportMessage : " << tm; },
                [] (auto &&canFrame) {
                        std::cout << "CAN Tx : " << canFrame << std::endl;
                        return true;
                },
                tp::ChronoTimeProvider{}, [] (auto &&error) { std::cout << "Erorr : " << uint32_t (error) << std::endl; });

        // Asynchronous - callback API
        tp.onCanNewFrame (tp::CanFrame (0x00, true, 1, 0x01, 0x67));
}

/**
 * TODO list:
 * + : covered (done).
 * - : not covered
 *
 * + Handle ISO messages that are constrained to less than maximum 4095B allowed by the
 * ISO document.
 * - Przejechać walgrindem od czasu do czasu
 * - Optymalizacja, bo testy idą wolno!
 * - Test the above.
 * - It is possible to define a TP object without a defazult address and then use its send
 * method also without an address. This way you are sending a message into oblivion. Have
 * it sorted out.
 * - TODO get rid of all warinigs and c-tidy issues.
 * -
 * TODO : If errors occur during multi frame message receiving, the isoMessage should be
 * removed (eventually, probably some timeouts are mentioned in the ISO). Now it is not
 * possible to receive second message if first has failed to be received entirely.
 * TODO Check if retyurn value from sendFrame is taken into account (checked).
 * TODO implement all types of addressing.
 * TODO Use some beter means of unit testing. Test time dependent calls, maybe use some
 * clever unit testing library like trompeleoleil for mocking.
 * TODO test crosswise connected objects. They should be able to speak to each other.
 * TODO test this library with python-can-isotp.
 * TODO add blocking API.
 * TODO test this api with std::threads.
 * + implement FF parameters : BS and STime
 * TODO verify with the ISO pdf whether everything is implemented.
 * TODO Redesign API - I (as a user) hate being forced to provide dozens of arguyments upon
 * construction that I don't really care about and do not use them. In this particular case
 * my biggest concern is the create function and callbacks that it takes.
 * + implement all enums that can be found in the ISO document.
 * + encapsulate more functionality into CanFrameWrapper. Like gettype, get length, getSerialnumber  etc.
 * TODO Include a note about N_As and NAr timeouts in the README.md. User shall check if sending
 * a single CAN frame took les than 1000ms + 50%. He should return false in that case, true otherwise.
 *
 * TODO tests:
 * - Test instantiation and usage with other CanFrame type
 * - Test instantiation and usage with other IsoMessage type
 * - Test flow control.
 *
 * ISO Specification areas
 * - Communication services (page 3):
 *  + N_USData.request (address, message)
 *  - N_USData.confirm (address, result) <- request (this above an only this) completed successfully or not.
 *  - N_USData_FF.indication (address, LENGTH) <- callback that first frame was received. It tells what is the lenghth of expected message
 *  - N_USData.indication (address, Message, result) - after Sf or after multi-can-message. Indicates that new data has arrived.
 *
 *  + N_ChangeParameter.request (faddress, key, value) - requests a parameter change in peer or locally, I'm not sure.
 *  + N_ChangeParameter.confirm  (address, key, result).
 * + N_ChangeParameter.request and N_ChangeParameter.confirm are optional. Fixed values may be used instead, and this is the
 * way to go I think. They are in fact hardcoded to 0 (best performance) in sendFlowFrame (see comment).
 * + Parameters (fixed or changeable) are : STmin and BS. Both hardcoded to 0.
 * + 5.2.3 and 5.2.4 when to send an indication and ff indication.
 *
 * + enum N_Result
 * - Flow control during transmission (chapter 6.3 pages 12, 13, 14).
 *  + Reading BS and STmin from FC frame received aftrer sending FF.
 *  + Waiting for FC between blocks of size BS CAN frames.
 *   + Reading this incoming FC and deciding what to do next.
 *   + If CTS, resume normally,
 *   + WAIT - wait (how much to wait?),
 *    + If WAIT is received more than N_WFTmax times then fail (this also means, that FC can be received few times ina row).
 *   + OVFLW - what to do then?
 *  + Delay of STmin between CFs
 *  + Protocol data units : create some factory and polymorphic types berived from CanFrameWrapper
 *
 * + Timing
 *
 * - Addressing
 *  - Address information is included in every CAN frame whether FF, CF, FC or SF
 *  - normal (uses arbitration ID, no constraints on the value)
 *   - 11b
 *   - 29b
 *  - mormal fixed (uses arbitration ID, further requirements as to how to encode this address into the arbitration ID). 29b only
 *   - physical
 *   - functional
 *  - extended. Like normal, but first data byte contains targetAddress, which is absent in arbitration ID.
 *   - 11b
 *    - physical
 *    - functional
 *   - 29b
 *    - physical
 *    - functional
 *  - mixed
 *
 *
 * - Unexpected N_PDU
 *  - implent (if not imlenmented already)
 *  - test
 *
 * - Get rid of dynamic allocation, because there is one.
 * - Get rid of non English comments.
 */
int main () { receivingAsyncCallback (); }
