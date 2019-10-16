/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "Address.h"
#include "LinuxCanFrame.h"
#include "TransportProtocol.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fmt/format.h>
#include <gsl/gsl>
#include <iostream>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

/****************************************************************************/

int createSocket ()
{
        int socketFd = socket (PF_CAN, SOCK_RAW, CAN_RAW);

        if (socketFd < 0) {
                fmt::print ("Error creating the socket\n");
                return -1;
        }

        sockaddr_can addr{};
        addr.can_family = AF_CAN;
        // addr.can_ifindex = 0; // Any can interface. I assume there is only one.
        //

        ifreq ifr{};
        strncpy (ifr.ifr_name, "slcan0", IFNAMSIZ - 1);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';
        ifr.ifr_ifindex = if_nametoindex (ifr.ifr_name);

        if (!ifr.ifr_ifindex) {
                fmt::print ("Error during if_nametoindex\n");
                return -1;
        }

        addr.can_ifindex = ifr.ifr_ifindex;

        // setsockopt (socketFd, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);

        if (bind (socketFd, (struct sockaddr *)&addr, sizeof (addr)) < 0) {
                fmt::print ("Error during bind\n");
                return -1;
        }

        return socketFd;
}

/**
 * Starts listening on a CAN_FD socket.
 */
template <typename C> void listenSocket (int socketFd, C callback)
{
        /* these settings are static and can be held out of the hot path */
        iovec iov{};
        msghdr msg{};

        can_frame frame{};
        iov.iov_base = &frame;
        // msg.msg_name = &addr;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        fd_set rdfs{};

        while (true) {
                FD_ZERO (&rdfs);
                FD_SET (socketFd, &rdfs);

                int ret{};
                struct timeval timeout_config = {5, 0};

                if ((ret = select (socketFd + 1, &rdfs, nullptr, nullptr, &timeout_config)) <= 0) {
                        // running = false;
                        continue;
                }

                if (FD_ISSET (socketFd, &rdfs)) {

                        /* these settings may be modified by recvmsg() */
                        iov.iov_len = sizeof (frame);
                        // msg.msg_namelen = sizeof (addr);
                        msg.msg_flags = 0;

                        int nbytes = recvmsg (socketFd, &msg, 0);

                        if (nbytes < 0) {
                                if (errno == ENETDOWN) {
                                        fmt::print ("Interface down\n");
                                        continue;
                                }

                                fmt::print ("Error during read\n");
                                return;
                        }

                        callback (frame);
                }
        }
}

/****************************************************************************/

int sendSocket (int socketFd, can_frame const &frame)
{
        int bytesToSend = int (sizeof (frame)); // CAN_MTU
        int bytesActuallySent = ::write (socketFd, &frame, bytesToSend);
        return (bytesActuallySent == bytesToSend);
}

/****************************************************************************/

int main ()
{
        using namespace tp;

        int socketFd = createSocket ();

        auto tp = create<can_frame> (
                [] (auto const &tm) {
                        std::transform (std::begin (tm), std::end (tm), std::ostream_iterator<char> (std::cout), [] (auto b) {
                                static_assert (std::is_same<decltype (b), uint8_t>::value);
                                return char (b);
                        });

                        std::cout << std::endl;
                },
                [socketFd] (auto const &frame) {
                        if (!sendSocket (socketFd, frame)) {
                                fmt::print ("Error transmitting frame Id : {:x}, dlc : {}, data[0] = {}\n", frame.can_id, frame.can_dlc,
                                            frame.data[0]);

                                return false;
                        }

                        return true;
                },
                ChronoTimeProvider{}, [] (auto &&error) { std::cout << "Erorr : " << uint32_t (error) << std::endl; });

        tp.setMyAddress (Address (0x456, 0x123));

        listenSocket (socketFd, [&tp] (auto const &frame) {
                fmt::print ("Received frame Id : {:x}, dlc : {}, data[0] = {}\n", frame.can_id, frame.can_dlc, frame.data[0]);
                tp.onCanNewFrame (frame);
        });
}
