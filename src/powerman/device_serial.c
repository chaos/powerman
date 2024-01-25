/*****************************************************************************
 *  Copyright (C) 2003 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>
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
 * Implement connect/disconnect device methods for serial devices.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <sys/time.h>

#include "xtypes.h"
#include "cbuf.h"
#include "hostlist.h"
#include "list.h"
#include "parse_util.h"
#include "xpoll.h"
#include "xmalloc.h"
#include "pluglist.h"
#include "arglist.h"
#include "xregex.h"
#include "device_private.h"
#include "device_serial.h"
#include "error.h"
#include "debug.h"
#include "xpty.h"

typedef struct {
    char *special;
    char *flags;
} SerialDev;

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
#ifdef B57600
    {57600, B57600},
#endif
#ifdef B115200
    {115200,B115200},
#endif
#ifdef B230400
    {230400,B230400},
#endif
#ifdef B460800
    {460800,B460800},
#endif
};

void *serial_create(char *special, char *flags)
{
    SerialDev *ser = (SerialDev *)xmalloc(sizeof(SerialDev));

    ser->special = xstrdup(special);
    ser->flags = xstrdup(flags);

    return (void *)ser;
}

void serial_destroy(void *data)
{
    SerialDev *ser = (SerialDev *)data;

    if (ser->special)
        xfree(ser->special);
    if (ser->flags)
        xfree(ser->flags);
    xfree(ser);
}

/* Set up serial port: 0 on success, <0 on error */
static int _serial_setup(char *devname, int fd, int baud, int databits,
        char parity, int stopbits)
{
    int res;
    struct termios tio;
    int i;
    res = tcgetattr(fd, &tio);
    if (res < 0) {
        err(TRUE, "%s: error getting serial attributes", devname);
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
        err(FALSE, "%s: error setting baud rate to %d", devname, baud);
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
            err(FALSE, "%s: error setting data bits to %d", devname, databits);
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
            err(FALSE, "%s: error setting stop bits to %d", devname, stopbits);
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
            err(FALSE, "%s: error setting parity to %c", devname, parity);
            return -1;
    }

    tio.c_oflag &= ~OPOST; /* turn off post-processing of output */
    tio.c_iflag = tio.c_lflag = 0;


    if (tcsetattr(fd, TCSANOW, &tio) < 0) {
        err(TRUE, "%s: error setting serial attributes", devname);
        return -1;
    }
    return 0;
}

/*
 * Open the special file associated with this device.
 */
bool serial_connect(Device * dev)
{
    SerialDev *ser;
    int baud = 9600, databits = 8, stopbits = 1;
    char parity = 'N';
    int res;
    int n;

    assert(dev->magic == DEV_MAGIC);
    assert(dev->connect_state == DEV_NOT_CONNECTED);
    assert(dev->fd == NO_FD);

    ser = (SerialDev *)dev->data;

    dev->fd = open(ser->special, O_RDWR | O_NONBLOCK | O_NOCTTY);
    if (dev->fd < 0) {
        err(TRUE, "_serial_connect(%s): open %s", dev->name, ser->special);
        goto out;
    }
    if (!isatty(dev->fd)) {
        err(FALSE, "_serial_connect(%s): not a tty", dev->name);
        goto out;
    }
    /*  [lifted from conman] According to the UNIX Programming FAQ v1.37
     *    <http://www.faqs.org/faqs/unix-faq/programmer/faq/>
     *    (Section 3.6: How to Handle a Serial Port or Modem),
     *    systems seem to differ as to whether a nonblocking
     *    open on a tty will affect subsequent read()s.
     *    Play it safe and be explicit!
     */
    nonblock_set(dev->fd);

    /* Conman takes an fcntl F_WRLCK on serial devices.
     * Powerman should respect conman's locks and vice-versa.
     */
    if (lockf(dev->fd, F_TLOCK, 0) < 0) {
        err(TRUE, "_serial_connect(%s): could not lock device\n", dev->name);
        goto out;
    }

    /* parse the serial flags and set up port accordingly */
    n = sscanf(ser->flags, "%d,%d%c%d", &baud, &databits, &parity, &stopbits);
    assert(n >= 0 && n <= 4); /* 0-4 matches OK (defaults if no match) */
    res = _serial_setup(dev->name, dev->fd, baud, databits, parity, stopbits);
    if (res < 0)
        goto out;

    dev->connect_state = DEV_CONNECTED;
    dev->stat_successful_connects++;

    err(FALSE, "_serial_connect(%s): opened", dev->name);
    return TRUE;

out:
    if (dev->fd >= 0) {
        if (close(dev->fd) < 0)
            err(TRUE, "_serial_connect(%s): close", dev->name);
        dev->fd = NO_FD;
    }
    return FALSE;
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
        if (close(dev->fd) < 0)
            err(TRUE, "_serial_disconnect(%s): close", dev->name);
        dev->fd = NO_FD;
    }

    err(FALSE, "_serial_disconnect(%s): closed", dev->name);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
