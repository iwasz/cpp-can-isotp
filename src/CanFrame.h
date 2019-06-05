/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <array>
#include <cstdint>
#include <iostream>

struct CanFrame {
        const static uint8_t DEFAULT_BYTE = 0x55;

        CanFrame () : id (0), extended (false), dlc (0), data{} {}
        CanFrame (uint32_t id, bool extended, uint8_t dlc, uint8_t data0, uint8_t data1 = DEFAULT_BYTE, uint8_t data2 = DEFAULT_BYTE,
                  uint8_t data3 = DEFAULT_BYTE, uint8_t data4 = DEFAULT_BYTE, uint8_t data5 = DEFAULT_BYTE, uint8_t data6 = DEFAULT_BYTE,
                  uint8_t data7 = DEFAULT_BYTE)
            : id (id), extended (extended), dlc (dlc)
        {
                data[0] = data0;
                data[1] = data1;
                data[2] = data2;
                data[3] = data3;
                data[4] = data4;
                data[5] = data5;
                data[6] = data6;
                data[7] = data7;
        }

        uint32_t id = 0;
        bool extended = false;
        uint8_t dlc = 0;
        std::array<uint8_t, 8> data;
};

inline std::ostream &operator<< (std::ostream &o, CanFrame const &cf)
{
        // TODO data.
        o << "CanFrame id = " << cf.id << ", dlc = " << cf.dlc << ", ext = " << cf.extended << ", data = " /*<< data*/;
        return o;
}
