/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <Arduino.h>
#include <avr/pgmspace.h>

namespace tp {

const char ARDUINO_ERROR_STR[] PROGMEM = "CAN ISO TP erorr : ";

struct ArduinoExceptionHandler {
        template <typename T> void operator() (T const &error)
        {
                Serial.print (ARDUINO_ERROR_STR);
                Serial.println (int (error));
        }
};

} // namespace tp
