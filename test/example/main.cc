/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "LinuxTransportProtocol.h"
#include <iostream>

std::ostream &operator<< (std::ostream &o, tp::IsoMessage const &b);
std::ostream &operator<< (std::ostream &o, tp::CanFrame const &cf);

/****************************************************************************/

int main ()
{
        auto tp = tp::create (
                tp::Address (0x12, 0x34), [] (auto &&tm) { std::cout << "TransportMessage : " << tm; },
                [] (auto &&canFrame) {
                        std::cout << "CAN Tx : " << canFrame << std::endl;
                        return true;
                },
                tp::ChronoTimeProvider{}, [] (auto &&error) { std::cout << "Error : " << uint32_t (error) << std::endl; });

        tp.onCanNewFrame (tp::CanFrame (0x00, true, 1, 0x01, 0x67));
}

/****************************************************************************/

std::ostream &operator<< (std::ostream &o, tp::IsoMessage const &b)
{
        o << "[";

        for (auto i = b.cbegin (); i != b.cend ();) {

                o << std::hex << int (*i);

                if (++i != b.cend ()) {
                        o << ",";
                }
        }

        o << "]";
        return o;
}

/*****************************************************************************/

std::ostream &operator<< (std::ostream &o, tp::CanFrame const &cf)
{
        o << "CanFrame id = " << cf.id << ", dlc = " << int (cf.dlc) << ", ext = " << cf.extended << ", data = [";

        for (int i = 0; i < cf.dlc;) {
                o << int (gsl::at (cf.data, i));

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

/****************************************************************************/
