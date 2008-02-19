/*****************************************************************************\
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
 *  Select/Poll wrap and Malloc debug added by Jim Garlick <garlick@llnl.gov>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE     /* needed for regex.h */
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <limits.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/wait.h>
#if HAVE_POLL
#include <sys/poll.h>
#endif

#include "wrappers.h"
#include "cbuf.h"
#include "error.h"

#define MAX_REG_BUF 64000

#ifndef NDEBUG
static int memory_alloc = 0;
#endif

/*********************
 *  lsd_fatal_error  *
 *********************/
/* Credit: borrowed from cbuf.c (Chris Dunlap) */

#ifdef WITH_LSD_FATAL_ERROR_FUNC
#  undef lsd_fatal_error
   extern void lsd_fatal_error(char *file, int line, char *mesg);
#else /* !WITH_LSD_FATAL_ERROR_FUNC */
#  ifndef lsd_fatal_error
#    include <errno.h>
#    include <stdio.h>
#    include <string.h>
#    define lsd_fatal_error(file, line, mesg)                                 \
          do {                                                                \
             fprintf(stderr, "ERROR: [%s:%d] %s: %s\n",                       \
                   file, line, mesg, strerror(errno));                        \
          } while (0)
#  endif /* !lsd_fatal_error */
#endif /* !WITH_LSD_FATAL_ERROR_FUNC */

/*********************
 *  lsd_nomem_error  *
 *********************/
/* Credit: borrowed from cbuf.c (Chris Dunlap) */

#ifdef WITH_LSD_NOMEM_ERROR_FUNC
#  undef lsd_nomem_error
   extern void * lsd_nomem_error(char *file, int line, char *mesg);
#else /* !WITH_LSD_NOMEM_ERROR_FUNC */
#  ifndef lsd_nomem_error
#    define lsd_nomem_error(file, line, mesg) (NULL)
#  endif /* !lsd_nomem_error */
#endif /* !WITH_LSD_NOMEM_ERROR_FUNC */

/*
 *   Taken nearly verbatim from Stevens, "UNIX Network Programming".
 * Some are my own extrapolations of his ideas, but should be
 * immediately obvious.  
 */

int Socket(int family, int type, int protocol)
{
    int fd;

    fd = socket(family, type, protocol);
    if (fd < 0)
        lsd_fatal_error(__FILE__, __LINE__, "socket");
    return fd;
}

int
Setsockopt(int fd, int level, int optname, const void *opt_val,
           socklen_t optlen)
{
    int ret_code;

    ret_code = setsockopt(fd, level, optname, opt_val, optlen);
    if (ret_code < 0)
        lsd_fatal_error(__FILE__, __LINE__, "setsockopt");
    return ret_code;
}

/* Review: socklen_t may not be defined - autoconf this later */
int Bind(int fd, struct sockaddr_in *saddr, socklen_t len)
{
    int ret_code;

    ret_code = bind(fd, (struct sockaddr *) saddr, len);
    if (ret_code < 0)
        lsd_fatal_error(__FILE__, __LINE__, "bind");
    return ret_code;
}

int
Getsockopt(int fd, int level, int optname, void *opt_val,
           socklen_t * optlen)
{
    int ret_code;

    ret_code = getsockopt(fd, level, optname, opt_val, optlen);
    if (ret_code < 0)
        lsd_fatal_error(__FILE__, __LINE__, "getsockopt");
    return ret_code;
}

int Listen(int fd, int backlog)
{
    int ret_code;

    ret_code = listen(fd, backlog);
    if (ret_code < 0)
        lsd_fatal_error(__FILE__, __LINE__, "listen");
    return ret_code;
}

int Fcntl(int fd, int cmd, int arg)
{
    int ret_code;

    ret_code = fcntl(fd, cmd, arg);
    if (ret_code < 0)
        lsd_fatal_error(__FILE__, __LINE__, "fcntl");
    return ret_code;
}

char *Strncpy(char *s1, const char *s2, int len)
{
    char *res = strncpy(s1, s2, len);

    s1[len - 1] = '\0';
    return res;
}

time_t Time(time_t * t)
{
    time_t n;

    n = time(t);
    if (n < 0)
        lsd_fatal_error(__FILE__, __LINE__, "time");
    return n;
}

void Gettimeofday(struct timeval *tv, struct timezone *tz)
{
    if (gettimeofday(tv, tz) < 0)
        lsd_fatal_error(__FILE__, __LINE__, "gettimeofday");
}

#define POLLFD_ALLOC_CHUNK  16
#define POLLFD_MAGIC    0x56452334
struct Pollfd {
    int magic;
#if HAVE_POLL
    unsigned int nfds;
    unsigned int ufds_size;
    struct pollfd *ufds;
#else /* select */
    int maxfd;
    fd_set rset;
    fd_set wset;
#endif
};

/* a null tv means no timeout (could block forever) */
int
Poll(Pollfd_t pfd, struct timeval *tv)
{
    struct timeval tv_cpy, start, end, delta;
    int n;

    if (tv) {
        tv_cpy = *tv;
        Gettimeofday(&start, NULL);
    }

    /* repeat poll if interrupted */
    do {
#if HAVE_POLL
        int tv_msec = tv ? tv_cpy.tv_sec * 1000 + tv_cpy.tv_usec / 1000 : -1;

        n = poll(pfd->ufds, pfd->nfds, tv_msec);
#else
        n = select(pfd->maxfd + 1, &pfd->rset, &pfd->wset, NULL, 
                tv ? &tv_cpy : NULL);
#endif
        if (n < 0 && errno != EINTR)
            lsd_fatal_error(__FILE__, __LINE__, "select");
        if (n < 0 && tv != NULL) {
            Gettimeofday(&end, NULL);
            timersub(&end, &start, &delta);     /* delta = end-start */
            timersub(tv, &delta, &tv_cpy);      /* tv_cpy = tv-delta */
        }
    } while (n < 0);
    return n;
}

#if HAVE_POLL
static void
_grow_pollfd(Pollfd_t pfd, int n)
{
    assert(pfd->magic == POLLFD_MAGIC);
    while (pfd->ufds_size < n) {
        pfd->ufds_size += POLLFD_ALLOC_CHUNK;
        pfd->ufds = (struct pollfd *)Realloc((char *)pfd->ufds, sizeof(struct pollfd) * pfd->ufds_size);
    }
}
#endif

Pollfd_t
PollfdCreate(void)
{
    Pollfd_t pfd = (Pollfd_t)Malloc(sizeof(struct Pollfd));
  
    pfd->magic = POLLFD_MAGIC;
#if HAVE_POLL
    pfd->ufds_size += POLLFD_ALLOC_CHUNK;
    pfd->ufds = (struct pollfd *)Malloc(sizeof(struct pollfd) * pfd->ufds_size);
    pfd->nfds = 0;
#else
    pfd->maxfd = 0;
    FD_ZERO(&pfd->rset);
    FD_ZERO(&pfd->wset);
#endif

    return pfd;
}

void
PollfdDestroy(Pollfd_t pfd)
{
    assert(pfd->magic == POLLFD_MAGIC);
    pfd->magic = 0;
#if HAVE_POLL
    if (pfd->ufds != NULL)
        Free(pfd->ufds);
#endif
    Free(pfd);
}

void
PollfdZero(Pollfd_t pfd)
{
    assert(pfd->magic == POLLFD_MAGIC);
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
PollfdSet(Pollfd_t pfd, int fd, short events)
{
#if HAVE_POLL
    int i;

    assert(pfd->magic == POLLFD_MAGIC);
    for (i = 0; i < pfd->nfds; i++) {
        if (pfd->ufds[i].fd == fd) {
            pfd->ufds[i].events |= events;
            break;
        }
    }
    if (i == pfd->nfds) { /* not found */
        _grow_pollfd(pfd, ++pfd->nfds);
        pfd->ufds[i].fd = fd;
        pfd->ufds[i].events = events;
    }
#else
    assert(pfd->magic == POLLFD_MAGIC);
    assert(fd < FD_SETSIZE);
    if (events & POLLIN)
        FD_SET(fd, &pfd->rset);
    if (events & POLLOUT)
        FD_SET(fd, &pfd->wset);
    pfd->maxfd = MAX(pfd->maxfd, fd);
#endif
}

char *
PollfdStr(Pollfd_t pfd, char *str, int len)
{
    int i;
    int maxfd = -1;

    assert(pfd->magic == POLLFD_MAGIC);
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
PollfdRevents(Pollfd_t pfd, int fd)
{
    short flags = 0;
#if HAVE_POLL
    int i;

    assert(pfd->magic == POLLFD_MAGIC);
    for (i = 0; i < pfd->nfds; i++) {
        if (pfd->ufds[i].fd == fd) {
            flags = pfd->ufds[i].revents;
            break;
        }
    }
#else
    assert(pfd->magic == POLLFD_MAGIC);
    if (FD_ISSET(fd, &pfd->rset))
        flags |= POLLIN;
    if (FD_ISSET(fd, &pfd->wset))
        flags |= POLLOUT;
#endif
    return flags;
} 

/* Review: look into dmalloc */
#define MALLOC_MAGIC 0xf00fbaab
#define MALLOC_PAD_SIZE   16
#define MALLOC_PAD_FILL 0x55

#ifndef NDEBUG
static int _checkfill(unsigned char *buf, unsigned char fill, int size)
{
    while (size-- > 0)
        if (buf[size] != fill)
            return 0;
    return 1;
}
#endif

char *wrap_malloc(char *file, int line, int size)
{
    char *new;
    int *p;


    assert(size > 0 && size <= INT_MAX);
    p = (int *) malloc(2*sizeof(int) + size + MALLOC_PAD_SIZE);
    if (p == NULL)
        return lsd_nomem_error(file, line, "malloc");
    p[0] = MALLOC_MAGIC;                           /* magic cookie */
    p[1] = size;                                   /* store size in buffer */
#ifndef NDEBUG
    memory_alloc += size;
#endif
    new = (char *) &p[2];
    memset(new, 0, size);
    memset(new + size, MALLOC_PAD_FILL, MALLOC_PAD_SIZE);
    return new;
}

char *wrap_realloc(char *file, int line, char *item , int newsize)
{
    char *new;
    int *p = (int *)item - 2;
    int oldsize;


    assert(item != NULL);
    assert(newsize > 0 && newsize <= INT_MAX);
    assert(p[0] == MALLOC_MAGIC);
    oldsize = p[1];
    assert(_checkfill(item + oldsize, MALLOC_PAD_FILL, MALLOC_PAD_SIZE));
    p = (int *)realloc(p, 2*sizeof(int) + newsize + MALLOC_PAD_SIZE);
    if (p == NULL)
        return lsd_nomem_error(file, line, "realloc");
    assert(p[0] == MALLOC_MAGIC);
    p[1] = newsize;
#ifndef NDEBUG
    memory_alloc += (newsize - oldsize);
#endif
    new = (char *) &p[2];
    if (newsize > oldsize)
        memset(new + oldsize, 0, newsize - oldsize);
    memset(new + newsize, MALLOC_PAD_FILL, MALLOC_PAD_SIZE);
    return new;
}

void Free(void *ptr)
{
    if (ptr != NULL) {
        int *p = (int *) ptr - 2;
        int size;

        assert(p[0] == MALLOC_MAGIC);   /* magic cookie still there? */
        size = p[1];
        assert(_checkfill(ptr + size, MALLOC_PAD_FILL, MALLOC_PAD_SIZE));
        memset(p, 0, 2*sizeof(int) + size + MALLOC_PAD_SIZE);
#ifndef NDEBUG
        memory_alloc -= size;
#endif
        free(p);
    }
}

char *Strdup(const char *str)
{
    char *cpy;

    cpy = Malloc(strlen(str) + 1);

    strcpy(cpy, str);
    return cpy;
}


int Accept(int fd, struct sockaddr_in *addr, socklen_t * addrlen)
{
    int new;

    new = accept(fd, (struct sockaddr *) addr, addrlen);
    if (new < 0) {
        /* 
         *   A client could abort before a ready connection is accepted.  
         *  "The fix for this problem is to:  ...  2.  "Ignore the following 
         *  errors ..."  - Stevens, UNP p424
         */
        if (!((errno == EWOULDBLOCK) ||
              (errno == ECONNABORTED) ||
              (errno == EPROTO) || (errno == EINTR)))
            lsd_fatal_error(__FILE__, __LINE__, "accept");
    }
    return new;
}

int Connect(int fd, struct sockaddr *addr, socklen_t addrlen)
{
    int n;

    n = connect(fd, addr, addrlen);
    if (n < 0) {
        switch (errno)  {
            case EINPROGRESS:
            case ECONNREFUSED:
                break;
            default:
                lsd_fatal_error(__FILE__, __LINE__, "connect");
                /*NOTREACHED*/
        }
    }
    return n;
}

int Read(int fd, unsigned char *p, int max)
{
    int n;

    do {
        n = read(fd, p, max);
    } while (n < 0 && errno == EINTR);
    if (n < 0 && errno != EWOULDBLOCK && errno != ECONNRESET)
        lsd_fatal_error(__FILE__, __LINE__, "read");
    return n;
}

int Write(int fd, unsigned char *p, int max)
{
    int n;

    do {
        n = write(fd, p, max);
    } while (n < 0 && errno == EINTR);
    if (n < 0 && errno != EAGAIN && errno != ECONNRESET && errno != EPIPE)
        lsd_fatal_error(__FILE__, __LINE__, "write");
    return n;
}


int Open(char *str, int flags, int mode)
{
    int fd;

    assert(str != NULL);
    fd = open(str, flags, mode);
    if (fd < 0)
        lsd_fatal_error(__FILE__, __LINE__, str);
    return fd;
}

int Close(int fd)
{
    int n;

    n = close(fd);
    if (n < 0)
        lsd_fatal_error(__FILE__, __LINE__, "close");
    return n;
}

/* Work around a problem in glibc-2.3.2-27.9 (redhat 9) */
static void _clean_stack(void)
{
    char _dummy[330]; /* 320 nope, 330 yes */
    int i;

    for (i = 0; i < (sizeof(_dummy)/sizeof(_dummy[0])); i++)
        _dummy[i] = 0;
}

#define MAX_GETADDRINFO_ERR_STR 1024
static void _gai_fatal_error(char *host, char *service, int err)
{
    char buf[MAX_GETADDRINFO_ERR_STR];

    snprintf(buf, sizeof(buf),
            "getaddrinfo: %s:%s: %s", host, service, gai_strerror(err));
    lsd_fatal_error(__FILE__, __LINE__, buf);
}

int
Getaddrinfo(char *host, char *service, struct addrinfo *hints,
            struct addrinfo **addrinfo)
{
    int n;

    _clean_stack();

    n = getaddrinfo(host, service, hints, addrinfo);
    if (n != 0)
        _gai_fatal_error(host, service, n);
    return n;
}

/* 
 * Substitute all occurrences of s2 with s3 in s1, 
 * e.g. _str_subst(str, "\\r", "\r") 
 */
static void _str_subst(char *s1, int len, const char *s2, const char *s3)
{
    int s2len = strlen(s2);
    int s3len = strlen(s3);
    char *p;

    while ((p = strstr(s1, s2)) != NULL) {
        assert(strlen(s1) + (s3len - s2len) + 1 <= len);
        memmove(p + s3len, p + s2len, strlen(p + s2len) + 1);
        memcpy(p, s3, s3len);
    }
}

void Regcomp(regex_t * preg, const char *regex, int cflags)
{
    char buf[MAX_REG_BUF];
    int n;

    assert(regex != NULL);
    assert(strlen(regex) < sizeof(buf));

    Strncpy(buf, regex, MAX_REG_BUF);

    /* convert backslash-prefixed special characters in regex to value */
    _str_subst(buf, MAX_REG_BUF, "\\r", "\r");
    _str_subst(buf, MAX_REG_BUF, "\\n", "\n");

    /*
     * N.B.
     * The buffer space available in a compiled RegEx expression is only 
     * 256 bytes.  A long or complicated RegEx will exceed this space and 
     * cause the library call to silently fail.
     */
    n = regcomp(preg, buf, cflags);
    if (n != REG_NOERROR)
        lsd_fatal_error(__FILE__, __LINE__, "regcomp failed");
}

int
Regexec(const regex_t * preg, const char *string,
        size_t nmatch, regmatch_t pmatch[], int eflags)
{
    int n;
    char buf[MAX_REG_BUF];

    /* Review: undocumented, is it needed? */
    re_syntax_options = RE_SYNTAX_POSIX_EXTENDED;
    Strncpy(buf, string, MAX_REG_BUF);
    n = regexec(preg, buf, nmatch, pmatch, eflags);
    return n;
}

pid_t Fork(void)
{
    pid_t pid;

    if ((pid = fork()) == -1)
        lsd_fatal_error(__FILE__, __LINE__, "fork");
    return (pid);
}

Sigfunc *Signal(int signo, Sigfunc * func)
{
    struct sigaction act, oact;
    int n;

    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (signo == SIGALRM) {
#ifdef  SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;   /* SunOS 4.x */
#endif
    } else {
#ifdef  SA_RESTART
        act.sa_flags |= SA_RESTART;     /* SVR4, 44BSD */
#endif
    }
    n = sigaction(signo, &act, &oact);
    if (n < 0)
        lsd_fatal_error(__FILE__, __LINE__, "sigaction");

    return (oact.sa_handler);
}

void Pipe(int filedes[2])
{
    if (pipe(filedes) < 0)
        lsd_fatal_error(__FILE__, __LINE__, "pipe");
}

void Dup2(int oldfd, int newfd)
{
    if (dup2(oldfd, newfd) < 0)
        lsd_fatal_error(__FILE__, __LINE__, "dup2");
}

void Execv(const char *path, char *const argv[])
{
    if (execv(path, argv) < 0)
        lsd_fatal_error(__FILE__, __LINE__, "execv");
}

pid_t Waitpid(pid_t pid, int *status, int options)
{
    pid_t n;

    while ((n = waitpid(pid, status, options)) < 0) {
        if (errno != EINTR)
            lsd_fatal_error(__FILE__, __LINE__, "waitpid");
    }

    return n;
}


#ifndef NDEBUG
int Memory(void)
{
    return memory_alloc;
}
#endif

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
