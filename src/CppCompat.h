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

#if defined(__GNUC__) && (defined(AVR) || defined(ARDUINO))
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <etl/array.h>
#include <etl/map.h>
#include <etl/optional.h>

#if !defined Expects
#define Expects(x) assert (x)
#endif

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

namespace std {
template <typename _Tp, typename _Up = _Tp &&> _Up __declval (int);

template <typename _Tp> _Tp __declval (long);

template <typename _Tp> auto declval () noexcept -> decltype (__declval<_Tp> (0));

template <typename _Tp> struct __declval_protector {
        static const bool __stop = false;
};

template <typename _Tp> auto declval () noexcept -> decltype (__declval<_Tp> (0))
{
        static_assert (__declval_protector<_Tp>::__stop, "declval() must not be used!");
        return __declval<_Tp> (0);
}
} // namespace std
#else
#include <cstdint>
#include <gsl/gsl>
#endif

#include <etl/map.h>
#include <etl/optional.h>
