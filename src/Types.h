/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#ifndef UTILS_TYPES_H
#define UTILS_TYPES_H

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
//#include <list>
//#include <array>

/**
 * Buffer for all input and ouptut operations
 */
typedef std::vector<uint8_t> Buffer;
//using Buffer = std::array<uint8_t, 128> ;

inline std::ostream &operator<< (std::ostream &o, Buffer const &b)
{
        o << "[";

        for (auto i = b.cbegin (); i != b.cend ();) {

                o << std::hex << int(*i);

                if (++i != b.cend ()) {
                        o << ",";
                }
        }

        o << "]";
        return o;
}

std::string to_ascii (const uint8_t *bytes, uint32_t length);

#endif // TYPES_H
