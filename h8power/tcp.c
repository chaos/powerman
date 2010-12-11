/*****************************************************************************\
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick (garlick@llnl.gov)
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
/*
 * N.B. Terminal servers must be configured in "raw" mode (no telnet protocol).
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include "cbuf.h"
#include "xtypes.h"
#include "error.h"
#include "wrappers.h"
#include "config.h"

void tcp_fini(int fd)
{
    if (close(fd) < 0)
        err_exit("close: %m");
}

int tcp_init(char *device)
{
    struct addrinfo hints, *addrinfo;
    int fd = -1;
    char host[HOST_NAME_MAX+1]; 
    char *port;

    if (strlen(device) + 1 > sizeof(host))
        err_exit("error parsing %s", device);
    strcpy(host, device);
    if (!(port = strchr(host, ':')))
        err_exit("error parsing %s", device);
    *port++ = '\0';

    memset(&hints, 0, sizeof(struct addrinfo));
    addrinfo = &hints;
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    Getaddrinfo(host, port, &hints, &addrinfo);

    fd = Socket(addrinfo->ai_family, addrinfo->ai_socktype, 
            addrinfo->ai_protocol);
    if (fd < 0)
        err_exit("socket: %m");

    if (Connect(fd, addrinfo->ai_addr, addrinfo->ai_addrlen) < 0)
        err_exit("connect: %m");

    return fd;
}
