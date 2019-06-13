#include "Iso15765TransportProtocol.h"
#include <iostream>

void receivingAsyncCallback ()
{
        auto tp = tp::create ([](auto &&tm) { std::cout << "TransportMessage : " << tm; },
                              [](auto &&canFrame) { std::cout << "CAN Tx : " << canFrame << std::endl; }, tp::TimeProvider{},
                              [](auto &&error) { std::cout << "Erorr : " << uint32_t (error) << std::endl; });

        // Asynchronous - callback API
        tp.onCanNewFrame (tp::CanFrame (0x00, true, 1, 0x01, 0x67));
}

/**
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
 * TODO Redesign API - I (as a user) hate beeing forced to provide dozens of arguyments upon
 * construction that I don't really care about and do not use them. In this particular case
 * my biggest concern is the create function and callbacks that it takes.
 * TODO implement all enums that can be found in the ISO document.
 * TODO encapsulate more functionality into CanFrameWrapper. Like gettype, get length, getSerialnumber  etc.
 *
 * TODO tests:
 * - Test instantiation and usage with other CanFrame type
 * - Test instantiation and usage with other IsoMessage type
 * - Test flow control.
 *
 * + : covered (done).
 * - : not covered
 *
 * ISO Specification areas
 * - Communication services (page 3):
 *  + N_USData.request (address, message)
 *  + N_USData.confirm (address, result) <- request (this above an only this) completed successfully or not.
 *  + N_USData_FF.indication (address, LENGTH) <- callback that first frame was received. It tells what is the lenghth of expected message
 *  + N_USData.indication (address, Message, result) - after Sf or after multi-can-message. Indicates that new data has arrived.
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
 *    - If WAIT is received more than N_WFTmax times then fail (this also means, that FC can be received few times ina row).
 *   + OVFLW - what to do then?
 *  + Delay of STmin between CFs
 *  + Protocol data units : create some factory and polymorphic types berived from CanFrameWrapper
 *
 * - Timing
 *
 * - Addressing
 *  - Address information is included in every CAN frame whether FF, CF, FC or SF
 *
 *
 */
int main () { receivingAsyncCallback (); }
