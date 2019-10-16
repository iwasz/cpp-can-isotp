/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

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
#include <sys/ioctl.h>
#include <sys/socket.h>

/**
 * Starts listening on a CAN_FD socket.
 */
template <typename C> void listen (C callback)
{
        int socketFd = socket (PF_CAN, SOCK_RAW, CAN_RAW);

        if (socketFd < 0) {
                fmt::print ("Error creating the socket\n");
                return;
        }

        sockaddr_can addr{};
        addr.can_family = AF_CAN;
        addr.can_ifindex = 0; // Any can interface. I assume there is only one.

        if (bind (socketFd, (struct sockaddr *)&addr, sizeof (addr)) < 0) {
                perror ("bind");
                return;
        }

        /* these settings are static and can be held out of the hot path */
        iovec iov{};
        msghdr msg{};

        canfd_frame frame{};
        iov.iov_base = &frame;
        msg.msg_name = &addr;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        fd_set rdfs{};

        bool running = true;

        while (running) {
                FD_ZERO (&rdfs);
                FD_SET (socketFd, &rdfs);

                int ret{};
                struct timeval timeout_config = {5, 0};

                if ((ret = select (socketFd + 1, &rdfs, nullptr, nullptr, &timeout_config)) <= 0) {
                        running = false;
                        continue;
                }

                if (FD_ISSET (socketFd, &rdfs)) {

                        /* these settings may be modified by recvmsg() */
                        iov.iov_len = sizeof (frame);
                        msg.msg_namelen = sizeof (addr);
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

int main ()
{
        auto tp = tp::create<canfd_frame> (
                [] (auto const &tm) { std::cout << "TransportMessage : " << tm; },
                [] (auto const &frame) {
                        fmt::print ("Transmitting frame Id : {}, dlc : {}, data[0] = {}\n", frame.can_id, frame.len, frame.data[0]);
                        return true;
                },
                tp::ChronoTimeProvider{}, [] (auto &&error) { std::cout << "Erorr : " << uint32_t (error) << std::endl; });

        listen ([&tp] (auto const &frame) {
                fmt::print ("Received frame Id : {}, dlc : {}, data[0] = {}\n", frame.can_id, frame.len, frame.data[0]);
                tp.onCanNewFrame (frame);
        });
}
