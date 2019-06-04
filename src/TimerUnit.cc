/*
 *          _,.-------.,_
 *      ,;~'             '~;,
 *    ,;                     ;,
 *   ;                         ;
 *  ,'                         ',
 * ,;                           ;,
 * ; ;      .           .      ; ;
 * | ;   ______       ______   ; |
 * |  `/~"     ~" . "~     "~\'  |
 * |  ~  ,-~~~^~, | ,~^~~~-,  ~  |
 *  |   |        }:{        |   |
 *  |   l       / | \       !   |
 *  .~  (__,.--" .^. "--.,__)  ~.
 *  |     ---;' / | \ `;---     |
 *   \__.       \/^\/       .__/
 *    V| \                 / |V
 *     | |T~\___!___!___/~T| |
 *     | |`IIII_I_I_I_IIII'| |
 *     |  \,III I I I III,/  |
 *      \   `~~~~~~~~~~'    /
 *        \   .       .   /     -dcau (4/15/95)
 *          \.    ^    ./
 *            ^~~~^~~~^
 *
 *
 */

#include "Timer.h"
#include <chrono>
#include <cstdint>
#include <iostream>

using namespace std::chrono;

/*****************************************************************************/

void Timer::start (uint32_t interval)
{
        system_clock::time_point now = system_clock::now ();
        auto duration = now.time_since_epoch ();
        startTime = duration_cast<milliseconds> (duration).count ();
        // std::cerr << "Start Time : " << startTime << std::endl;
        this->intervalMs = interval;
}

/*****************************************************************************/

bool Timer::isExpired () const { return elapsed () >= intervalMs; }

/*****************************************************************************/

uint32_t Timer::elapsed () const
{
        uint32_t actualTime = getTick ();
        // std::cerr << "Actual : " << actualTime << ", diff : " << actualTime - startTime << ", intervalMs: " << intervalMs << std::endl;
        return actualTime - startTime;
}

void Timer::delay (uint32_t delayMs)
{
        while (true) { /* TODO */
        }
}

uint32_t Timer::getTick ()
{
        time_point<system_clock> now = system_clock::now ();
        auto duration = now.time_since_epoch ();
        return duration_cast<milliseconds> (duration).count ();
}
