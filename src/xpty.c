/*****************************************************************************\
 *  $Id: wrappers.c 911 2008-05-30 20:26:33Z garlick $
 *****************************************************************************
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
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

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>
#if HAVE_PTY_H
#include <pty.h>
#endif
#if HAVE_UTIL_H
#include <util.h>
#endif
#if ! HAVE_FORKPTY
#include <sys/ioctl.h>  
#include <sys/stream.h> 
#include <sys/stropts.h> 
#if HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif
#endif
#include <stdio.h>
#include <assert.h>

#include "xtypes.h"
#include "xpty.h"
#include "error.h"

void xcfmakeraw(int fd)
{
    struct termios tio;

    if (tcgetattr(fd, &tio) < 0)
        lsd_fatal_error(__FILE__, __LINE__, "tcgetattr");
#if HAVE_CFMAKERAW
    cfmakeraw(&tio);
#else
    /* Stevens: Adv. Prog. in the UNIX Env.  1ed p.345 */
    tio.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    tio.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    tio.c_cflag &= ~(CSIZE | PARENB);
    tio.c_cflag |= CS8;
    tio.c_oflag &= ~(OPOST);
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
#endif
    if (tcsetattr(fd, TCSANOW, &tio) < 0)
        lsd_fatal_error(__FILE__, __LINE__, "tcsetattr");
}

pid_t xforkpty(int *amaster, char *name, int len)
{
    pid_t pid;

#if HAVE_FORKPTY
    pid = forkpty(amaster, name, NULL, NULL);
    if (pid > 0 && name != NULL) { /* XXX forkpty takes no len parameter */
        assert(strlen(name) < len);
    }
#else
#if HAVE__DEV_PTMX 
    /* solaris style - this code initially borrowed from 
     *  http://bugs.mysql.com/bug.php?id=22429
     */
    int master, slave; 
    char *slave_name; 
  
    master = open("/dev/ptmx", O_RDWR); 
    if (master < 0) 
        return -1; 
    if (grantpt (master) < 0) { 
        close (master); 
        return -1; 
    } 
    if (unlockpt (master) < 0) { 
        close (master); 
        return -1; 
    } 
    slave_name = ptsname (master); 
    if (slave_name == NULL) { 
        close (master); 
        return -1; 
    } 
    slave = open (slave_name, O_RDWR); 
    if (slave < 0) { 
        close (master); 
        return -1; 
    } 
    if (ioctl (slave, I_PUSH, "ptem") < 0 
      || ioctl (slave, I_PUSH, "ldterm") < 0) { 
        close (slave); 
        close (master); 
        return -1; 
    } 
#elif HAVE__DEV_PTC
    /* aix style */
    int master, slave; 
    char *slave_name; 

    master = open("/dev/ptc", O_RDWR); 
    if (master < 0) 
        return -1; 
    if (grantpt (master) < 0) { 
        close (master); 
        return -1; 
    } 
    if (unlockpt (master) < 0) { 
        close (master); 
        return -1; 
    } 
    slave_name = ptsname (master); 
    if (slave_name == NULL) { 
        close (master); 
        return -1; 
    } 
    slave = open (slave_name, O_RDWR); 
    if (slave < 0) { 
        close (master); 
        return -1; 
    } 
#else
#error unknown pty master device path
#endif
    if (amaster) 
        *amaster = master; 
    if (name) 
        strncpy (name, slave_name, len); 
    pid = fork (); 
    switch (pid) { 
        case -1: /* Error */ 
            return -1; 
        case 0: /* Child */ 
            close (master); 
            dup2 (slave, STDIN_FILENO); 
            dup2 (slave, STDOUT_FILENO); 
            dup2 (slave, STDERR_FILENO); 
            return 0; 
        default: /* Parent */ 
            close (slave); 
            break;
    } 
#endif
    /* XXX setsid() needed? */
    return pid;
}


/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
