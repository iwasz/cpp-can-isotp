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

struct ArduinoExceptionHandler {
        template <typename T> void operator() (T const &error)
        {
                Serial.print ("CAN ISO TP erorr : ");
                Serial.println (int (error));
        }
};

} // namespace tp
