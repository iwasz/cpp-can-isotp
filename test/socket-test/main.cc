/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "TransportProtocol.h"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fmt/format.h>
#include <gsl/gsl>
#include <iostream>
#include <libgen.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

void receivingAsyncCallback () {}

int main ()
{
        int socketFd = socket (PF_CAN, SOCK_RAW, CAN_RAW);

        if (socketFd < 0) {
                perror ("socket");
                return 1;
        }

        sockaddr_can addr{};

        addr.can_family = AF_CAN;

        // Nazwa interfejsu
        canfd_frame frame{};
        ifreq ifr{};
        memset (&ifr.ifr_name, 0, sizeof (ifr.ifr_name));

        gsl::czstring<> interfaceName = "slcan0";
        strncpy (ifr.ifr_name, interfaceName, strlen (interfaceName));

        // jeżeli nie ANY:
        // if (ioctl (socketFd, SIOCGIFINDEX, &ifr) < 0) {
        //         perror ("SIOCGIFINDEX");
        //         exit (1);
        // }

        // addr.can_ifindex = ifr.ifr_ifindex; // 6

        // Jeżeli any

        addr.can_ifindex = 0; /* any can interface */

        // po co to jest? Chyba próbuje wykryć czy to CAN FD
        /* try to switch the socket into CAN FD mode */
        int canfd_on = 1;
        setsockopt (socketFd, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &canfd_on, sizeof (canfd_on));

        if (bind (socketFd, (struct sockaddr *)&addr, sizeof (addr)) < 0) {
                perror ("bind");
                return 1;
        }

        /* these settings are static and can be held out of the hot path */
        iovec iov{};
        msghdr msg{};

        iov.iov_base = &frame;
        msg.msg_name = &addr;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        // msg.msg_control = &ctrlmsg;
        fd_set rdfs{};

        bool running = true;
        while (running) {
                /// ODCZYT
                FD_ZERO (&rdfs);
                FD_SET (socketFd, &rdfs);

                int ret{};
                struct timeval timeout_config = {5, 0};

                if ((ret = select (socketFd + 1, &rdfs, nullptr, nullptr, &timeout_config)) <= 0) {
                        // perror("select");
                        running = false;
                        continue;
                }

                if (FD_ISSET (socketFd, &rdfs)) {

                        /* these settings may be modified by recvmsg() */
                        iov.iov_len = sizeof (frame);
                        msg.msg_namelen = sizeof (addr);
                        // msg.msg_controllen = sizeof (ctrlmsg);
                        msg.msg_flags = 0;

                        int nbytes = recvmsg (socketFd, &msg, 0);

                        if (nbytes < 0) {
                                if (errno == ENETDOWN) {
                                        fprintf (stderr, "interface down\n");
                                        continue;
                                }
                                perror ("read");
                                return 1;
                        }

                        // fprint_long_canframe (stdout, &frame, NULL, view, maxdlen);
                        fmt::print ("CanFrame Id : {}, dlc : {}, data[0] = {}\n", frame.can_id, frame.len, frame.data[0]);
                }
        }

        /*--------------------------------------------------------------------------*/

        // auto tp = tp::create ([] (auto const &tm) { std::cout << "TransportMessage : " << tm; },
        //                       [] (auto const &canFrame) {
        //                               std::cout << "CAN Tx : " << canFrame << std::endl;
        //                               return true;
        //                       },
        //                       tp::ChronoTimeProvider{}, [] (auto &&error) { std::cout << "Erorr : " << uint32_t (error) << std::endl; });

        // // Asynchronous - callback API
        // tp.onCanNewFrame (tp::CanFrame (0x00, true, 1, 0x01, 0x67));
}
