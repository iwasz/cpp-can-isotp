/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once

namespace tp {

/**
 * @brief The ArduinoTimeProvider struct
 */
struct ArduinoTimeProvider {
        long operator() () const
        {
                // TODO
                //                static long i = 0;
                //                return ++i;
                return 0;
        }
};

} // namespace tp
