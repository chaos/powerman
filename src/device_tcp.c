/*****************************************************************************\
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton (uselton2@llnl.gov>
 *  UCRL-CODE-2002-008.
 *  
 *  This file is part of PowerMan, a remote power management program.
 *  For details, see <http://www.llnl.gov/linux/powerman/>.
 *  
 *  PowerMan is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *  
 *  PowerMan is distributed in the hope that it will be useful, but WITHOUT 
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
 *  for more details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with PowerMan; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/

#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

#include "powerman.h"
#include "list.h"
#include "parse_util.h"
#include "device.h"
#include "error.h"
#include "wrappers.h"
#include "cbuf.h"
#include "hostlist.h"
#include "debug.h"
#include "client_proto.h"
#include "device_telnet.h"
#include "device_serial.h"
#include "device_tcp.h"

/*
 *   We've supposedly reconnected, so check if we really are.
 * If not go into reconnect mode.  If this is a succeeding
 * reconnect, log that fact.  When all is well initiate the
 * logon script.
 */
bool tcp_finish_connect(Device * dev, struct timeval *timeout)
{
    int rc;
    int error = 0;
    int len = sizeof(err);

    assert(dev->type == TCP_DEV || dev->type == TELNET_DEV);
    assert(dev->connect_status == DEV_CONNECTING);
    rc = getsockopt(dev->fd, SOL_SOCKET, SO_ERROR, &error, &len);
    /*
     *  If an error occurred, Berkeley-derived implementations
     *    return 0 with the pending error in 'err'.  But Solaris
     *    returns -1 with the pending error in 'errno'.  -dun
     */
    if (rc < 0)
        error = errno;
    if (error) {
        err(FALSE, "tcp_finish_connect: %s: %s", dev->name, strerror(error));
        dev_disconnect(dev);
        if (dev_time_to_reconnect(dev, timeout))
            dev_reconnect(dev);
    } else {
        err(FALSE, "tcp_finish_connect: %s: connected", dev->name);
        dev->connect_status = DEV_CONNECTED;
        dev->stat_successful_connects++;
        dev->retry_count = 0;
        if (dev->type == TELNET_DEV)
            telnet_init(dev);
        dev_login(dev);
    }


    return (dev->connect_status == DEV_CONNECTED);
}


/*
 * Nonblocking TCP connect.
 * tcp_finish_connect will finish the job.
 */
bool tcp_reconnect(Device * dev)
{
    struct addrinfo hints, *addrinfo;
    int sock_opt;
    int fd_settings;

    assert(dev->magic == DEV_MAGIC);
    assert(dev->type == TCP_DEV || dev->type == TELNET_DEV);
    assert(dev->connect_status == DEV_NOT_CONNECTED);
    assert(dev->fd == NO_FD);

    Gettimeofday(&dev->last_retry, NULL);
    dev->retry_count++;

    memset(&hints, 0, sizeof(struct addrinfo));
    addrinfo = &hints;
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    Getaddrinfo(dev->u.tcp.host, dev->u.tcp.service, &hints, &addrinfo);

    dev->fd = Socket(addrinfo->ai_family, addrinfo->ai_socktype,
                     addrinfo->ai_protocol);

    dbg(DBG_DEVICE, "tcp_reconnect: %s on fd %d", dev->name, dev->fd);

    /* set up and initiate a non-blocking connect */

    sock_opt = 1;
    Setsockopt(dev->fd, SOL_SOCKET, SO_REUSEADDR,
               &(sock_opt), sizeof(sock_opt));
    fd_settings = Fcntl(dev->fd, F_GETFL, 0);
    Fcntl(dev->fd, F_SETFL, fd_settings | O_NONBLOCK);

    /* Connect - 0 = connected, -1 implies EINPROGRESS */
    dev->connect_status = DEV_CONNECTING;
    if (Connect(dev->fd, addrinfo->ai_addr, addrinfo->ai_addrlen) >= 0)
        tcp_finish_connect(dev, NULL);
    freeaddrinfo(addrinfo);

    return (dev->connect_status == DEV_CONNECTED);
}

/*
 * Close the socket associated with this device.
 */
void tcp_disconnect(Device * dev)
{
    assert(dev->connect_status == DEV_CONNECTING
           || dev->connect_status == DEV_CONNECTED);
    assert(dev->type == TCP_DEV || dev->type == TELNET_DEV);

    dbg(DBG_DEVICE, "tcp_disconnect: %s on fd %d", dev->name, dev->fd);

    /* close socket if open */
    if (dev->fd >= 0) {
        Close(dev->fd);
        dev->fd = NO_FD;
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
