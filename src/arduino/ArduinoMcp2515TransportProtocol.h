/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "ArduinoExceptionHandler.h"
#include "ArduinoMcp2515Can.h"
#include "ArduinoTimeProvider.h"
#include "TransportProtocol.h"

namespace tp {

template <typename CanFrameT = can_frame, typename AddressResolverT = Normal29AddressEncoder, typename IsoMessageT /*= IsoMessage*/,
          size_t MAX_MESSAGE_SIZE = MAX_ALLOWED_ISO_MESSAGE_SIZE, typename CanOutputInterfaceT, typename TimeProviderT = ArduinoTimeProvider,
          typename ExceptionHandlerT = ArduinoExceptionHandler, typename CallbackT>
auto create (Address const &myAddress, CallbackT callback, CanOutputInterfaceT outputInterface, TimeProviderT timeProvider = {},
             ExceptionHandlerT errorHandler = {})
{
        using TP = TransportProtocol<TransportProtocolTraits<CanFrameT, IsoMessageT, MAX_MESSAGE_SIZE, AddressResolverT, CanOutputInterfaceT,
                                                             TimeProviderT, ExceptionHandlerT, CallbackT, 4>>;

        return TP{myAddress, callback, outputInterface, timeProvider, errorHandler};
}

} // namespace tp
