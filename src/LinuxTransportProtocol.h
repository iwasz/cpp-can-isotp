/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "LinuxCanFrame.h"
#include "StlTypes.h"
#include "TransportProtocol.h"

namespace tp {

template <typename CanFrameT = CanFrame, typename AddressResolverT = Normal29AddressEncoder, typename IsoMessageT = IsoMessage,
          size_t MAX_MESSAGE_SIZE = MAX_ALLOWED_ISO_MESSAGE_SIZE, typename CanOutputInterfaceT = LinuxCanOutputInterface,
          typename TimeProviderT = ChronoTimeProvider, typename ExceptionHandlerT = InfiniteLoop, typename CallbackT = CoutPrinter>
auto create (Address const &myAddress, CallbackT callback, CanOutputInterfaceT outputInterface = {}, TimeProviderT timeProvider = {},
             ExceptionHandlerT errorHandler = {})
{
        using TP = TransportProtocol<TransportProtocolTraits<CanFrameT, IsoMessageT, MAX_MESSAGE_SIZE, AddressResolverT, CanOutputInterfaceT,
                                                             TimeProviderT, ExceptionHandlerT, CallbackT, 4>>;

        return TP{myAddress, callback, outputInterface, timeProvider, errorHandler};
}

} // namespace tp
