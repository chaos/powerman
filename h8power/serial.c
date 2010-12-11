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
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "cbuf.h"
#include "xtypes.h"
#include "error.h"

/*
 * Hardwired tty parameters for RS485<->232 converter.
 * N.B. Baud, bits, parity, stopb are the same on both RS232 and RS485.
 * RS485 is just a "party line" RS232, that uses differential pairs instead
 * of ground-referenced signals for greater noise immunity.
 */
#define RS485_BAUD    B57600
#define RS485_BITS    CS8     /* CS8 | CS7 */
#define RS485_PARITY  'n'     /* 'e' | 'o' | 'n' */
#define RS485_STOPB   1       /* 1 | 2 */

void serial_fini(int fd)
{
    if (lockf(fd, F_ULOCK, 0) < 0)
        err_exit("lockf(F_ULOCK): %m");
    if (close(fd) < 0)
        err_exit("close: %m");
}

int serial_init(char *device)
{
    int fd;
    struct termios tio;

    if ((fd = open(device, O_RDWR)) < 0)
        err_exit("open(%s): %m", device);
    if (lockf(fd, F_TLOCK, 0) < 0)
        err_exit("lockf(%s, F_TLOCK): %m", device);
    if (tcgetattr(fd, &tio) < 0)
        err_exit("tcgetattr(%s): %m", device);
    if (cfsetispeed(&tio, RS485_BAUD) < 0)
        err_exit("cfsetispeed(%s): %m", device);
    if (cfsetospeed(&tio, RS485_BAUD) < 0)
        err_exit("cfsetospeed(%s): %m", device);
    tio.c_cflag &= ~CSIZE;
    tio.c_cflag |= RS485_BITS;
   
    if (RS485_STOPB == 1)
        tio.c_cflag &= ~CSTOPB;
    else if (RS485_STOPB == 2)
        tio.c_cflag |= CSTOPB;
    else {
	    assert(1);
    }

    if (RS485_PARITY == 'n')
        tio.c_cflag &= ~PARENB;
    else if (RS485_PARITY == 'o') {
        tio.c_cflag |= PARENB;
        tio.c_cflag |= PARODD;
    } else if (RS485_PARITY == 'e') {
        tio.c_cflag |= PARENB;
        tio.c_cflag &= ~PARODD;
    } else {
        assert(1);
    }

    tio.c_cflag |= CLOCAL;

    tio.c_oflag &= ~OPOST; /* turn off post-processing of output */
    tio.c_iflag = tio.c_lflag = 0;
#if 0
    tio.c_cc[VMIN] = 1
    tio.c_cc[VTIME] = 1;
#endif
    if (tcsetattr(fd, TCSANOW, &tio) < 0)
        err_exit("tcsetattr(%s): %m", device);

    return fd;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
