/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#ifndef LIB_MICRO_TIMER_H
#define LIB_MICRO_TIMER_H

#include <cstdint>

class Timer {
public:
        Timer () = default;
        Timer (uint32_t intervalMs) { start (intervalMs); }

        /// Resets the timer (it starts from 0) and sets the interval. So isExpired will return true after whole interval has passed.
        void start (uint32_t intervalMs);

        /// Change interval without reseting the timer. Can extend as well as shorten.
        void extend (uint32_t intervalMs) { this->intervalMs = intervalMs; }

        /// Says if intervalMs has passed since start () was called.
        bool isExpired () const;

        /// Returns how many ms has passed since start () was called.
        uint32_t elapsed () const;

        /// Convenience method, simple delay ms.
        static void delay (uint32_t delayMs);

        /// Returns system wide ms since system start.
        static uint32_t getTick ();

protected:
        uint32_t startTime = 0;
        uint32_t intervalMs = 0;
};

#endif //__TIMER_H__
