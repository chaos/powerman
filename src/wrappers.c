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

#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>

#include "powerman.h"
#include "error.h"
#include "wrappers.h"

#define MAX_REG_BUF 64000

#ifndef NDEBUG
static int memory_alloc = 0;
#endif

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
	err_exit(TRUE, "socket");
    return fd;
}

int
Setsockopt(int fd, int level, int optname, const void *opt_val,
	   socklen_t optlen)
{
    int ret_code;

    ret_code = setsockopt(fd, level, optname, opt_val, optlen);
    if (ret_code < 0)
	err_exit(TRUE, "setsockopt");
    return ret_code;
}

/* Review: socklen_t may not be defined - autoconf this later */
int Bind(int fd, struct sockaddr_in *saddr, socklen_t len)
{
    int ret_code;

    ret_code = bind(fd, (struct sockaddr *) saddr, len);
    if (ret_code < 0)
	err_exit(TRUE, "bind");
    return ret_code;
}

int
Getsockopt(int fd, int level, int optname, void *opt_val,
	   socklen_t * optlen)
{
    int ret_code;

    ret_code = getsockopt(fd, level, optname, opt_val, optlen);
    if (ret_code < 0)
	err_exit(TRUE, "getsockopt");
    return ret_code;
}

int Listen(int fd, int backlog)
{
    int ret_code;

    ret_code = listen(fd, backlog);
    if (ret_code < 0)
	err_exit(TRUE, "listen");
    return ret_code;
}

int Fcntl(int fd, int cmd, int arg)
{
    int ret_code;

    ret_code = fcntl(fd, cmd, arg);
    if (ret_code < 0)
	err_exit(TRUE, "fcntl");
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
	err_exit(TRUE, "time");
    return n;
}

void Gettimeofday(struct timeval *tv, struct timezone *tz)
{
    if (gettimeofday(tv, tz) < 0)
	err_exit(TRUE, "gettimeofday");
}

static void _clear_sets(fd_set * rset, fd_set * wset, fd_set * eset)
{
    if (rset != NULL)
	FD_ZERO(rset);
    if (wset != NULL)
	FD_ZERO(wset);
    if (eset != NULL)
	FD_ZERO(eset);
}

/*
 * Select wrapper that retries select on EINTR with appropriate timeout
 * adjustments, and err_exit on any other failures.
 * Can return 0 indicating timeout or a value > 0.
 * NOTE: fd_sets are cleared on timeout.
 */
int
Select(int maxfd, fd_set * rset, fd_set * wset, fd_set * eset,
       struct timeval *tv)
{
    int n;
    struct timeval tv_orig;
    struct timeval start, end, delta;

    /* prep for EINTR handling */
    if (tv) {
	tv_orig = *tv;
	Gettimeofday(&start, NULL);
    }
    /* repeat select if interrupted */
    do {
	n = select(maxfd, rset, wset, eset, tv);
	if (n < 0 && errno != EINTR)	/* unrecov error */
	    err_exit(TRUE, "select");
	if (n < 0 && tv != NULL) {	/* EINTR - adjust tv */
	    Gettimeofday(&end, NULL);
	    timersub(&end, &start, &delta);	/* delta = end-start */
	    timersub(&tv_orig, &delta, tv);	/* tv = tvsave-delta */
	}
	if (n < 0)
	    fprintf(stderr, "retrying interrupted select\n");
    } while (n < 0);
    /* XXX main select loop needs this fd_sets cleared on timeout */
    if (n == 0)
	_clear_sets(rset, wset, eset);
    return n;
}


void Delay(struct timeval *tv)
{
    int res;
    res = Select(0, NULL, NULL, NULL, tv);
    assert(res == 0);
}

/* Review: look into dmalloc */
#define MALLOC_MAGIC 0xf00fbaab
char *Malloc(int size)
{
    char *new;
    int *p;

    assert(size > 0 && size <= INT_MAX);
    p = (int *) malloc(size + 2 * sizeof(int));
    if (p == NULL)
	err_exit(FALSE, "out of memory");
    p[0] = MALLOC_MAGIC;	/* add "secret" magic cookie */
    p[1] = size;		/* store size in buffer */
#ifndef NDEBUG
    memory_alloc += size;
#endif
    new = (char *) &p[2];
    memset(new, 0, size);
    return new;
}

void Free(void *ptr)
{
    if (ptr != NULL) {
	int *p = (int *) ptr - 2;
	int size;

	assert(p[0] == MALLOC_MAGIC);	/* magic cookie still there? */
	size = p[1];
	memset(p, 0, size + 2 * sizeof(int));
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
	    err_exit(TRUE, "accept");
    }
    return new;
}

int Connect(int fd, struct sockaddr *addr, socklen_t addrlen)
{
    int n;

    n = connect(fd, addr, addrlen);
    if (n < 0) {
	if (errno != EINPROGRESS)
	    err_exit(TRUE, "connect");
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
	err_exit(TRUE, "read");
    return n;
}

int Write(int fd, unsigned char *p, int max)
{
    int n;

    do {
	n = write(fd, p, max);
    } while (n < 0 && errno == EINTR);
    if (n < 0 && errno != EAGAIN && errno != ECONNRESET && errno != EPIPE)
	err_exit(TRUE, "write");
    return n;
}

int Open(char *str, int flags, int mode)
{
    int fd;

    assert(str != NULL);
    fd = open(str, flags, mode);
    if (fd < 0)
	err_exit(TRUE, "open %s", str);
    return fd;
}

int Close(int fd)
{
    int n;

    n = close(fd);
    if (n < 0)
	err_exit(TRUE, "close");
    return n;
}

int
Getaddrinfo(char *host, char *service, struct addrinfo *hints,
	    struct addrinfo **addrinfo)
{
    int n;

    n = getaddrinfo(host, service, hints, addrinfo);
    if (n != 0)
	err_exit(FALSE, "getaddrinfo host %s service %s: %s",
		 host, service, gai_strerror(n));
    return n;
}

/* 
 * Substitute all occurences of s2 with s3 in s1, 
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
	err_exit(FALSE, "regcomp failed");
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
	err_exit(TRUE, "fork");
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
#ifdef	SA_INTERRUPT
	act.sa_flags |= SA_INTERRUPT;	/* SunOS 4.x */
#endif
    } else {
#ifdef	SA_RESTART
	act.sa_flags |= SA_RESTART;	/* SVR4, 44BSD */
#endif
    }
    n = sigaction(signo, &act, &oact);
    if (n < 0)
	err_exit(TRUE, "sigaction");

    return (oact.sa_handler);
}

#ifndef NDEBUG
int Memory(void) {
    return memory_alloc;
}
#endif

/*
 * vi:softtabstop=4
 */
