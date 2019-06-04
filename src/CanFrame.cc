/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "CanFrame.h"
#include <cstring>

//CanFrame::CanFrame () : id (0), extended (false), dlc (0) { memset (data, 0, sizeof (data)); }


/*****************************************************************************/
#ifndef UNIT_TEST

CanFrame::CanFrame (CanRxMsgTypeDef const &frame)
{
        if (frame.IDE == CAN_ID_EXT) {
                id = frame.ExtId;
                extended = true;
        }
        else {
                id = frame.StdId;
                extended = true;
        }

        dlc = frame.DLC;
        memset (data, 0, 8);
        memcpy (data, frame.Data, frame.DLC);
}

/*****************************************************************************/

CanTxMsgTypeDef CanFrame::toNative () const
{
        CanTxMsgTypeDef native;

        if (extended) {
                native.StdId = 0x00;
                native.ExtId = id;
                native.IDE = CAN_ID_EXT;
        }
        else {
                native.StdId = id;
                native.ExtId = 0x00;
                native.IDE = CAN_ID_STD;
        }

        native.RTR = CAN_RTR_DATA;
        native.DLC = dlc;
        memset (native.Data, 0, 8);
        memcpy (native.Data, data, dlc);
        return native;
}

#endif
