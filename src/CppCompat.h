/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once

/**
 * This file is for maintaining compatibility with systems where C++ standard library is
 * not available i.e. for Arduino. I was totally unaware, that there is no C++ standard
 * library implemented for this toy platform.
 */

#if defined(__GNUC__) && defined(AVR)
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#else
#include <cstdint>
//#include <gsl/gsl>
#endif

//#include <array>
#include <etl/map.h>
//#include <optional>

#define Expects(x) assert (x)

namespace gsl {

template <class T, std::size_t N> constexpr T &at (T (&arr)[N], const int i)
{
        Expects (i >= 0 && i < static_cast<int> (N));
        return arr[static_cast<size_t> (i)];
}

template <class Cont> constexpr auto at (Cont &cont, const int i) -> decltype (cont[cont.size ()])
{
        Expects (i >= 0 && i < static_cast<int> (cont.size ()));
        using size_type = decltype (cont.size ());
        return cont[static_cast<size_type> (i)];
}

} // namespace gsl
