/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "TransportProtocol.h"
#include <iostream>

int main ()
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
