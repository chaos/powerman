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
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>

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
#include "device_serial.h"

/*
 * Implement connect/disconnect device methods for serial devices.
 */

typedef struct {
    int baud;
    speed_t bconst;
} baudmap_t;

static baudmap_t baudmap[] = {
    {300,   B300},   
    {1200,  B1200},   
    {2400,  B2400},   
    {4800,  B4800}, 
    {9600,  B9600}, 
    {19200, B19200}, 
    {38400, B38400},
};

/* Set up serial port: 0 on success, <0 on error */
static int _serial_setup(char *devname, int fd, int baud, int databits, 
        char parity, int stopbits)
{
    int res;
    struct termios tio;
    int i;
    res = tcgetattr(fd, &tio);
    if (res < 0) {
        err(TRUE, "%s: error getting serial attributes\n", devname);
        return -1;
    }

    res = -1;
    for (i = 0; i < sizeof(baudmap)/sizeof(baudmap_t); i++) {
        if (baudmap[i].baud == baud) {
            if ((res = cfsetispeed(&tio, baudmap[i].bconst)) == 0)
                 res = cfsetospeed(&tio, baudmap[i].bconst);
            break;
        }
    }
    if (res < 0) {
        err(FALSE, "%s: error setting baud rate to %d\n", devname, baud);
        return -1;
    }

    switch (databits) {
        case 7:
            tio.c_cflag &= ~CSIZE;
            tio.c_cflag |= CS7;
            break;
        case 8:
            tio.c_cflag &= ~CSIZE;
            tio.c_cflag |= CS8;
            break;
        default:
            err(FALSE, "%s: error setting data bits to %d\n", 
                    devname, databits);
            return -1;
    }

    switch (stopbits) {
        case 1:
            tio.c_cflag &= ~CSTOPB;
            break;
        case 2:
            tio.c_cflag |= CSTOPB;
            break;
        default:
            err(FALSE, "%s: error setting stop bits to %d\n", 
                    devname, stopbits);
            return -1;
    }

    switch (parity) {
        case 'n':
        case 'N':
            tio.c_cflag &= ~PARENB;
            break;
        case 'e':
        case 'E':
            tio.c_cflag |= PARENB;
            tio.c_cflag &= ~PARODD;
            break;
        case 'o':
        case 'O':
            tio.c_cflag |= PARENB;
            tio.c_cflag |= PARODD;
            break;
        default:
            err(FALSE, "%s: error setting parity to %c\n", 
                    devname, parity);
            return -1;
    }

    tio.c_oflag &= ~OPOST; /* turn off post-processing of output */
    tio.c_iflag = tio.c_lflag = 0;


    if (tcsetattr(fd, TCSANOW, &tio) < 0) {
        err(TRUE, "%s: error setting serial attributes\n", devname);
        return -1;
    }
    return 0;
}

/*
 * Open the special file associated with this device.
 */
bool serial_connect(Device * dev)
{
    int baud = 9600, databits = 8, stopbits = 1; 
    char parity = 'N';
    int res;
    int fd_settings;

    assert(dev->magic == DEV_MAGIC);
    assert(dev->connect_state == DEV_NOT_CONNECTED);
    assert(dev->fd == NO_FD);

    dev->fd = open(dev->host, O_RDWR | O_NONBLOCK | O_NOCTTY);
    if (dev->fd < 0) {
        dbg(DBG_DEVICE, "_serial_connect: %s open %s failed", 
                dev->name, dev->host);
        goto out;
    }
    if (!isatty(dev->fd)) {
        err(FALSE, "_serial_connect: %s is not a tty\n", dev->name);
        goto out;
    }
    /*  [lifted from conman] According to the UNIX Programming FAQ v1.37
     *    <http://www.faqs.org/faqs/unix-faq/programmer/faq/>
     *    (Section 3.6: How to Handle a Serial Port or Modem),
     *    systems seem to differ as to whether a nonblocking
     *    open on a tty will affect subsequent read()s.
     *    Play it safe and be explicit!
     */
    fd_settings = Fcntl(dev->fd, F_GETFL, 0);
    Fcntl(dev->fd, F_SETFL, fd_settings | O_NONBLOCK);

    /* FIXME: take an flock F_WRLOCK to coexist with conman */

    /* parse the serial flags and set up port accordingly */
    sscanf(dev->flags, "%d,%d%c%d", &baud, &databits, &parity, &stopbits);
    res = _serial_setup(dev->name, dev->fd, baud, databits, parity, stopbits);
    if (res < 0)
        goto out;

    dev->connect_state = DEV_CONNECTED;
    dev->stat_successful_connects++;
    dev->retry_count = 0;

    err(FALSE, "_serial_connect: %s opened", dev->name);
out: 
    return (dev->connect_state == DEV_CONNECTED);
}


/*
 * Close the special file associated with this device.
 */
void serial_disconnect(Device * dev)
{
    assert(dev->connect_state == DEV_CONNECTED);
    dbg(DBG_DEVICE, "_serial_disconnect: %s on fd %d", dev->name, dev->fd);

    /* close device if open */
    if (dev->fd >= 0) {
        Close(dev->fd);
        dev->fd = NO_FD;
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
