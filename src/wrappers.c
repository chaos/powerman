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
#include "exit_error.h"
#include "wrappers.h"

#define MAX_REG_BUF 64000

static void clear_sets(fd_set *rset, fd_set *wset, fd_set *eset);


/*
 *   Taken nearly verbatim from Stevens, "UNIX Network Programming".
 * Some are my own extrapolations of his ideas, but should be
 * immediately obvious.  
 */

int
Socket(int family, int type, int protocol)
{
	int fd;

	fd = socket(family, type, protocol);
	if(fd < 0) 
		exit_error("socket");
	return fd;
}

int
Setsockopt( int fd, int level, int optname, const void *opt_val, 
	    socklen_t optlen ) 
{
	int ret_code;

	ret_code = setsockopt(fd, level, optname, 
			      opt_val, optlen);
	if(ret_code < 0) 
		exit_error("setsockopt");
	return ret_code;
}

/* XXX socklen_t may not be defined - autoconf this later */
int
Bind( int fd, struct sockaddr_in *saddr, socklen_t len )
{
	int ret_code;

	ret_code = bind(fd, (struct sockaddr *)saddr, len);
	if(ret_code < 0) 
		exit_error("bind");
	return ret_code;
}

int
Getsockopt( int fd, int level, int optname, void *opt_val, 
	    socklen_t *optlen ) 
{
	int ret_code;

	ret_code = getsockopt(fd, level, optname, 
			      opt_val, optlen);
	if(ret_code < 0) 
		exit_error("getsockopt");
	return ret_code;
}

int
Listen(int fd, int backlog)
{
	int ret_code;

	ret_code = listen( fd, backlog );
	if(ret_code < 0) 
		exit_error("listen");
	return ret_code;
}

int
Fcntl( int fd, int cmd, int arg)
{
	int ret_code;

	ret_code = fcntl(fd, cmd, arg);
	if(ret_code < 0) 
		exit_error("fcntl");
	return ret_code;
}

int
Select(int maxfd, fd_set *rset, fd_set *wset, fd_set *eset, struct timeval *tv)
{
	int n;

	n = select(maxfd, rset, wset, eset, tv);
	if (n < 0)
/* Some sort of error occured.  Can we ignore it? */
		if (errno != EINTR) 
			exit_error("select");
	if (n <= 0)
/* If it timed out then don't do anything else on this round of the loop */
		clear_sets(rset, wset, eset);
	return n;
}

time_t
Time(time_t *t)
{
	time_t n;

	n = time(t);
	if (n < 0) 
		exit_error("time");
	return n;
}

void
Gettimeofday(struct timeval *tv, struct timezone *tz)
{
	if (gettimeofday(tv, tz) < 0)
		exit_error("gettimeofday");
}

void
Delay(struct timeval *tv)
{
	int n;
	struct timeval t, start, end, delta;

	if (tv == NULL)
		exit_msg("Delay called with NULL timeval");
	t = *tv;
	Gettimeofday(&start, NULL);
	for (;;) {
		n = select(0, NULL, NULL, NULL, &t);
		if (n == 0 || errno != EINTR)
			break;
		/* interrupted - adjust t to account for time elapsed so far */
		Gettimeofday(&end, NULL);
		timersub(&end, &start, &delta);
		timersub(tv, &delta, &t);
	} 
	if (n < 0)
		exit_error("select (in Delay)");
	return;
}

void
clear_sets(fd_set *rset, fd_set *wset, fd_set *eset)
{
	if(rset != NULL) FD_ZERO(rset);
	if(wset != NULL) FD_ZERO(wset);
	if(eset != NULL) FD_ZERO(eset);
}

static int allocated_memory = 0;

/* XXX look into dmalloc */
char *
Malloc(int size)
{
	char *new;

	new = malloc(size);
	if (new == NULL)
		exit_msg("Out of memory");
	allocated_memory += size;
	return new;
}

char *
Strdup(char *str)
{
	char *cpy;
       
	cpy = Malloc(strlen(str) + 1);

	strcpy(cpy, str);
	return cpy;
}

void 
Free(void *ptr, int size)
{
	assert(size > 0);
	assert(ptr != NULL);
	memset(ptr, 0, size);
	allocated_memory -= size;
	free(ptr);
}

int
Accept(int fd, struct sockaddr_in *addr, socklen_t *addrlen)
{
	int new;

	new = accept(fd, (struct sockaddr *)addr, addrlen);
	if(new < 0) 
	{
/* 
 *   A client could abort before a ready connection is accepted.  "The fix
 * for this problem is to:  ...  2.  "Ignore the following errors ..."
 *                                                     - Stevens, UNP p424
 */
		if ( !( (errno == EWOULDBLOCK)  ||
			(errno == ECONNABORTED) ||
			(errno == EPROTO)       ||
			(errno == EINTR) ) )
			exit_error("accept");
	}
	return new;
}

int
Connect(int fd, struct sockaddr *addr, socklen_t addrlen)
{
	int n;

	n = connect(fd, addr, addrlen);
	if(n < 0) 
	{
		if (errno != EINPROGRESS)
			exit_error("connect");
	}
	return n;
}

int
Read(int fd, char *p, int max)
{
	int n;

	do {
		n = read(fd, p, max);
	} while (n < 0 && errno == EINTR);
	if (n < 0 && errno != EWOULDBLOCK && errno != ECONNRESET) 
		exit_error("read");
	return n;
}

int
Write(int fd, char *p, int max)
{
	int n;

	do {
		n = write(fd, p, max);
	} while (n < 0 && errno == EINTR);
	if (n < 0 && errno != EAGAIN && errno != ECONNRESET && errno != EPIPE)
		exit_error("write");
	return n;
}

int
Open(char *str, int flags, int mode)
{
	int fd;

	assert(str != NULL);
	fd = open(str, flags, mode);
	if (fd < 0)
		exit_error("open %s", str);
	return fd;
}

int
Close(int fd)
{
	int n;

	n = close(fd);
	if (n < 0)
		exit_error("close");
	return n;
}

int
Getaddrinfo(char *host, char *service, struct addrinfo *hints, 
	    struct addrinfo **addrinfo)
{
	int n;

	n = getaddrinfo(host, service, hints, addrinfo);
	if (n != 0)
		exit_msg("getaddrinfo host %s service %s: %s", 
				host, service, gai_strerror(n));
	return n;
}

/* XXX overflow of buf is possible */
void
Regcomp(regex_t *preg, const char *regex, int cflags)
{
	char buf[MAX_REG_BUF];
	int n;
	int i = 0;
	int j = 0;

	assert(regex != NULL);
	assert(strlen(regex) < sizeof(buf));

/* My lame hack because regcomp won't interpret '\r' and '\n' */
	while ( (i < MAX_REG_BUF) && (regex[j] != '\0') )
	{
		if ( (regex[j] == '\\') && (regex[j+1] == 'r') )
		{
			buf[i++] = '\r';
			j += 2;
		}
		else if ( (regex[j] == '\\') && (regex[j+1] == 'n') )
		{
			buf[i++] = '\n';
			j += 2;
		}
		else
		{
			buf[i++] = regex[j];
			j += 1;
		}
	}
	buf[i] = '\0';
	/* XXX document limitation of library regcomp */
	n = regcomp(preg, buf, cflags);
	if (n != REG_NOERROR) exit_msg("regcomp failed");
}

int
Regexec(const regex_t *preg, const char *string, 
	size_t nmatch, regmatch_t pmatch[], int eflags)
{
	int n;
	char buf[MAX_REG_BUF];

	/* XXX is this needed? */	
	re_syntax_options = RE_SYNTAX_POSIX_EXTENDED;
	strncpy(buf, string, MAX_REG_BUF);
	buf[MAX_REG_BUF - 1] = '\0'; /* be safe */
	n = regexec(preg, buf, nmatch, pmatch, eflags);
	return n;
}

pid_t
Fork(void)
{
        pid_t   pid;

        if ( (pid = fork()) == -1)
                exit_error("fork");
        return(pid);
}

Sigfunc *
Signal(int signo, Sigfunc *func)
{
	struct sigaction act, oact;
	int n;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (signo == SIGALRM) 
	{
#ifdef	SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;	/* SunOS 4.x */
#endif
	} 
	else 
	{
#ifdef	SA_RESTART
		act.sa_flags |= SA_RESTART;		/* SVR4, 44BSD */
#endif
	}
	n = sigaction(signo, &act, &oact);
	if ( n < 0) exit_error("sigaction");

	return(oact.sa_handler);
}

#ifndef NDUMP
void
Report_Memory()
{
	fprintf(stderr, "Remaining allocated memory is: %d\n", 
		allocated_memory);
}
#endif

