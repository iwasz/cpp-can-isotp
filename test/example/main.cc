#include "Iso15765TransportProtocol.h"
#include <iostream>

int main ()
{
        TransportProtocol tp{ [](auto &&isoMessage) {
                                     // std::cout << "Decoded : " << isoMessage << std::endl;
                                     // TODO czemu!?!?!?!
                                     isoMessage.operator<< (std::cout);
                             },
                              [](auto &&canFrame) { std::cout << "CAN Tx : " << canFrame << std::endl; }, TimeProvider{},
                              [](auto &&error) { std::cout << "Erorr : " << uint32_t (error) << std::endl; } };

        tp.onCanNewFrame (CanFrame (0x01, false, 2, 0x67, 0x89));
}
