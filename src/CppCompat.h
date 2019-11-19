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
#include <cstdint>
#else
#include <stdint.h>
#endif

#include <gsl/gsl>
#include <array>
#include <gsl/gsl>
