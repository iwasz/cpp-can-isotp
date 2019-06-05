#include "Iso15765TransportProtocol.h"
#include <iostream>

void receivingAsyncCallback ()
{
        TransportProtocol tp{ [](auto &&tm) { std::cout << "TransportMessage : " << tm; },
                              [](auto &&canFrame) { std::cout << "CAN Tx : " << canFrame << std::endl; }, TimeProvider{},
                              [](auto &&error) { std::cout << "Erorr : " << uint32_t (error) << std::endl; } };

        // Asynchronous - callback API
        tp.onCanNewFrame (CanFrame (0x00, true, 1, 0x01, 0x67));
}

int main () { receivingAsyncCallback (); }
