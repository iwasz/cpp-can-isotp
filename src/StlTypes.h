/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <chrono>
#include <cstdint>
#include <gsl/gsl>
#include <iostream>
#include <vector>

namespace tp {
class CanFrame;

/**
 * @brief The TimeProvider struct
 * TODO if I'm not mistaken, those classes ought to have 100µs resolution instead of 1ms (1000µs).
 */
struct ChronoTimeProvider {
        long operator() () const
        {
                using namespace std::chrono;
                system_clock::time_point now = system_clock::now ();
                auto duration = now.time_since_epoch ();
                return duration_cast<milliseconds> (duration).count ();
        }
};

struct CoutPrinter {
        template <typename T> void operator() (T &&a) { std::cout << std::forward<T> (a) << std::endl; }
};

/**
 * Buffer for all input and output operations
 */
using IsoMessage = std::vector<uint8_t>;

} // namespace tp
