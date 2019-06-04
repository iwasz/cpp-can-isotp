/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#ifndef ITRANSPORTPROTOCOLCALLBACK_H
#define ITRANSPORTPROTOCOLCALLBACK_H

#include "TransportMessage.h"
#include <cstdint>

/**
 * Callback for TransportProtocol classes
 */
struct ITransportProtocolCallback {


        virtual ~ITransportProtocolCallback () {}

        /*
         * To jest wywoływane z std::move, więc wiadomości są bezpieczni przenoszone. API
         * asynchroniczne jest dlatego, że na jedno zapytanie może przyjść wiele odpowiedzi.
         */
//        virtual void onNewMessage (TransportMessage) = 0;
        virtual void onError (uint32_t e) = 0;
};

#endif // ITRANSPORTPROTOCOLCALLBACK_H
