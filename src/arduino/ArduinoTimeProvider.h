/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <Arduino.h>

namespace tp {

/**
 * @brief The ArduinoTimeProvider struct
 */
struct ArduinoTimeProvider {
        long operator() () const { return millis (); }
};

} // namespace tp
