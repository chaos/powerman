/*
 * Borrowed more or less verbatim from 
 * "Advanced Programming in the UNIX Environment" by W. Richard Stevens.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <termios.h>
#include <unistd.h>
#ifndef TIOCGWINSZ
#include <sys/ioctl.h>   /* 44BSD requires this too */
#endif

#include "error.h"
#include "pty.h"

#ifdef __linux
#define PTYNAME1 "pqrstuvwxyzabcde" /* this is correct for RH 7.3 linux -jg */
#else
#define PTYNAME1 "pqrstuvwxyzPQRST" /* Stevens used this */
#endif
#define PTYNAME2 "0123456789abcdef"

static int _ptym_open(char *pts_name)
{
    int fdm;
    char *ptr1, *ptr2;

    strcpy(pts_name, "/dev/ptyXY");
    /* array index: 0123456789 (for references in following code) */
    for (ptr1 = PTYNAME1; *ptr1 != 0; ptr1++) {
        pts_name[8] = *ptr1;
        for (ptr2 = PTYNAME2; *ptr2 != 0; ptr2++) {
            pts_name[9] = *ptr2;

            /* try to open master */
            if ((fdm = open(pts_name, O_RDWR)) < 0) {
                if (errno == ENOENT)    /* different from EIO */
                    return (-1);        /* out of pty devices */
                else
                    continue;           /* try next pty device */
            }

            pts_name[5] = 't';          /* change "pty" to "tty" */
            return (fdm);               /* got it, return fd of master */
        }
    }
    return (-1);                        /* out of pty devices */
}

static int _ptys_open(int fdm, char *pts_name)
{
    struct group *grptr;
    int gid, fds;

    if ((grptr = getgrnam("tty")) != NULL)
        gid = grptr->gr_gid;
    else
        gid = -1;                       /* group tty is not in the group file */

    /* following two functions don't work unless we're root */
    chown(pts_name, getuid(), gid);
    chmod(pts_name, S_IRUSR | S_IWUSR | S_IWGRP);

    if ((fds = open(pts_name, O_RDWR)) < 0) {
        close(fdm);
        return (-1);
    }
    return (fds);
}


static pid_t
_pty_fork(int *ptrfdm, char *slave_name,
          const struct termios *slave_termios,
          const struct winsize *slave_winsize)
{
    int fdm, fds;
    pid_t pid;
    char pts_name[20];

    if ((fdm = _ptym_open(pts_name)) < 0) {
        err(FALSE, "can't open master pty: %s", pts_name);
        return (-1);
    }

    if (slave_name != NULL)
        strcpy(slave_name, pts_name);   /* return name of slave */

    if ((pid = fork()) < 0)
        return (-1);

    else if (pid == 0) {                /* child */
        if (setsid() < 0)
            err(FALSE, "setsid error");

        /* SVR4 acquires controlling terminal on open() */
        if ((fds = _ptys_open(fdm, pts_name)) < 0) {
            err_exit(TRUE, "can't open slave pty");
        }
        close(fdm);                     /* all done with master in child */

#if defined(TIOCSCTTY) && !defined(CIBAUD)
        /* 44BSD way to acquire controlling terminal */
        /* !CIBAUD to avoid doing this under SunOS */
        if (ioctl(fds, TIOCSCTTY, (char *) 0) < 0)
            err(TRUE, "TIOCSCTTY error");
#endif
        /* set slave's termios and window size */
        if (slave_termios != NULL) {
            if (tcsetattr(fds, TCSANOW, slave_termios) < 0)
                err(TRUE, "tcsetattr error on slave pty");
        }
        if (slave_winsize != NULL) {
            if (ioctl(fds, TIOCSWINSZ, slave_winsize) < 0)
                err(FALSE, "TIOCSWINSZ error on slave pty");
        }
        /* slave becomes stdin/stdout/stderr of child */
        if (dup2(fds, STDIN_FILENO) != STDIN_FILENO)
            err_exit(TRUE, "dup2 error to stdin");
        if (dup2(fds, STDOUT_FILENO) != STDOUT_FILENO)
            err_exit(TRUE, "dup2 error to stdout");
        if (dup2(fds, STDERR_FILENO) != STDERR_FILENO)
            err_exit(TRUE, "dup2 error to stderr");
        if (fds > STDERR_FILENO)
            close(fds);
        return (0);                     /* child returns 0 just like fork() */

    } else {                            /* parent */
        *ptrfdm = fdm;                  /* return fd of master */
        return (pid);                   /* parent returns pid of child */
    }
}

pid_t pty_fork(int *ptrfdm, char *slave_name)
{
    return _pty_fork(ptrfdm, slave_name, NULL, NULL);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
