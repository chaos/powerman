/*****************************************************************************
 *  Copyright (C) 2001 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
 *  UCRL-CODE-2002-008.
 *
 *  This file is part of PowerMan, a remote power management program.
 *  For details, see http://github.com/chaos/powerman/
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
/* Force select() on darwin as poll() can't be used on devices (e.g. ptys)
 * FIXME: This should be a configure-time test.
 */
#if defined(__APPLE__) && defined(HAVE_POLL)
#undef HAVE_POLL
#endif
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#if HAVE_POLL_H
#include <sys/poll.h>
#endif
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <assert.h>

#include "xtime.h"
#include "xtypes.h"
#include "xmalloc.h"
#include "error.h"
#include "xpoll.h"

#ifndef MAX
#define MAX(x, y) (((x) > (y))? (x) : (y))
#endif

#define XPOLLFD_ALLOC_CHUNK  16

#define XPOLLFD_MAGIC    0x56452334
struct xpollfd {
    int             magic;
#if HAVE_POLL
    unsigned int    nfds;
    unsigned int    ufds_size;
    struct pollfd  *ufds;
#else /* select */
    int             maxfd;
    fd_set          rset;
    fd_set          wset;
#endif
};

#if HAVE_POLL
static short
xflag2flag(short x)
{
    short f = 0;

    if ((x & XPOLLIN))
        f |= POLLIN;
    if ((x & XPOLLOUT))
        f |= POLLOUT;
    if ((x & XPOLLHUP))
        f |= POLLHUP;
    if ((x & XPOLLERR))
        f |= POLLERR;
    if ((x & XPOLLNVAL))
        f |= POLLNVAL;

    return f;
}

static short
flag2xflag(short f)
{
    short x = 0;

    if ((f & POLLIN))
        x |= XPOLLIN;
    if ((f & POLLOUT))
        x |= XPOLLOUT;
    if ((f & POLLHUP))
        x |= XPOLLHUP;
    if ((f & POLLERR))
        x |= XPOLLERR;
    if ((f & POLLNVAL))
        x |= XPOLLNVAL;

    return x;
}
#endif

/* a null tv means no timeout (could block forever) */
int
xpoll(xpollfd_t pfd, struct timeval *tv)
{
    struct timeval tv_cpy, *tvp = NULL;
    struct timeval start, end, delta;
    int n;

    if (tv) {
        tv_cpy = *tv;
        if (gettimeofday(&start, NULL) < 0)
            err_exit(TRUE, "gettimeofday");
        tvp = &tv_cpy;
    }

    /* repeat poll if interrupted */
    do {
#if HAVE_POLL
        int tv_msec = -1;

        if (tvp)
            tv_msec = tvp->tv_sec * 1000 + tvp->tv_usec / 1000;

        n = poll(pfd->ufds, pfd->nfds, tv_msec);
#else
        n = select(pfd->maxfd + 1, &pfd->rset, &pfd->wset, NULL, tvp);
#endif
        if (n < 0 && errno != EINTR)
            err_exit(TRUE, "select/poll");
        if (n < 0 && tv != NULL) {
            if (gettimeofday(&end, NULL) < 0)
                err_exit(TRUE, "gettimeofday");
            timersub(&end, &start, &delta);     /* delta = end - start */
            timersub(tv, &delta, tvp);          /* *tvp = tv - delta */
        }
    } while (n < 0);
    return n;
}

#if HAVE_POLL
static void
_grow_pollfd(xpollfd_t pfd, int n)
{
    assert(pfd->magic == XPOLLFD_MAGIC);
    while (pfd->ufds_size < n) {
        pfd->ufds_size += XPOLLFD_ALLOC_CHUNK;
        pfd->ufds = (struct pollfd *)xrealloc((char *)pfd->ufds, sizeof(struct pollfd) * pfd->ufds_size);
    }
}
#endif

xpollfd_t
xpollfd_create(void)
{
    xpollfd_t pfd = (xpollfd_t)xmalloc(sizeof(struct xpollfd));

    pfd->magic = XPOLLFD_MAGIC;
#if HAVE_POLL
    pfd->ufds_size += XPOLLFD_ALLOC_CHUNK;
    pfd->ufds = (struct pollfd *)xmalloc(sizeof(struct pollfd)*pfd->ufds_size);
    pfd->nfds = 0;
#else
    pfd->maxfd = 0;
    FD_ZERO(&pfd->rset);
    FD_ZERO(&pfd->wset);
#endif

    return pfd;
}

void
xpollfd_destroy(xpollfd_t pfd)
{
    assert(pfd->magic == XPOLLFD_MAGIC);
    pfd->magic = 0;
#if HAVE_POLL
    if (pfd->ufds != NULL)
        xfree(pfd->ufds);
#endif
    xfree(pfd);
}

void
xpollfd_zero(xpollfd_t pfd)
{
    assert(pfd->magic == XPOLLFD_MAGIC);
#if HAVE_POLL
    pfd->nfds = 0;
    /*memset(pfd->ufds, 0, sizeof(struct pollfd) * pfd->ufds_size);*/
#else
    FD_ZERO(&pfd->rset);
    FD_ZERO(&pfd->wset);
    pfd->maxfd = 0;
#endif
}

void
xpollfd_set(xpollfd_t pfd, int fd, short events)
{
#if HAVE_POLL
    int i;

    assert(pfd->magic == XPOLLFD_MAGIC);
    for (i = 0; i < pfd->nfds; i++) {
        if (pfd->ufds[i].fd == fd) {
            pfd->ufds[i].events |= xflag2flag(events);
            break;
        }
    }
    if (i == pfd->nfds) { /* not found */
        _grow_pollfd(pfd, ++pfd->nfds);
        pfd->ufds[i].fd = fd;
        pfd->ufds[i].events = xflag2flag(events);
    }
#else
    assert(pfd->magic == XPOLLFD_MAGIC);
    assert(fd < FD_SETSIZE);
    if (events & XPOLLIN)
        FD_SET(fd, &pfd->rset);
    if (events & XPOLLOUT)
        FD_SET(fd, &pfd->wset);
    pfd->maxfd = MAX(pfd->maxfd, fd);
#endif
}

char *
xpollfd_str(xpollfd_t pfd, char *str, int len)
{
    int i;
#if HAVE_POLL
    int maxfd = -1;
#endif
    assert(pfd->magic == XPOLLFD_MAGIC);
#if HAVE_POLL
    memset(str, '.', len);
    for (i = 0; i < pfd->nfds; i++) {
        int fd = pfd->ufds[i].fd;
        short revents = pfd->ufds[i].revents;

        if (fd < len - 1) {
            if (revents) {
                if (revents & (POLLNVAL | POLLERR | POLLHUP))
                    str[fd] = 'E';
                else if (revents & POLLIN)
                    str[fd] = 'I';
                else if (revents & POLLOUT)
                    str[fd] = 'O';
            }
            if (fd > maxfd)
                maxfd = fd;
        }
    }
    assert(maxfd + 1 < len);
    str[maxfd + 1] = '\0';
#else
    for (i = 0; i <= pfd->maxfd; i++) {
        if (FD_ISSET(i, &pfd->rset))
            str[i] = 'I';
        else if (FD_ISSET(i, &pfd->wset))
            str[i] = 'O';
        else
            str[i] = '.';
    }
    assert(i < len);
    str[i] = '\0';
#endif
    return str;
}

short
xpollfd_revents(xpollfd_t pfd, int fd)
{
    short flags = 0;
#if HAVE_POLL
    int i;

    assert(pfd->magic == XPOLLFD_MAGIC);
    for (i = 0; i < pfd->nfds; i++) {
        if (pfd->ufds[i].fd == fd) {
            flags = flag2xflag(pfd->ufds[i].revents);
            break;
        }
    }
#else
    assert(pfd->magic == XPOLLFD_MAGIC);
    if (FD_ISSET(fd, &pfd->rset))
        flags |= XPOLLIN;
    if (FD_ISSET(fd, &pfd->wset))
        flags |= XPOLLOUT;
#endif
    return flags;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
