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

#include "powerman.h"

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

	errno = 0;
	fd = socket(family, type, protocol);
	if(fd < 0) 
		exit_error("Could not open socket");
	return fd;
}

int
Setsockopt( int fd, int level, int optname, const void *opt_val, 
	    socklen_t optlen ) 
{
	int ret_code;

	errno = 0;
	ret_code = setsockopt(fd, level, optname, 
			      opt_val, optlen);
	if(ret_code < 0) 
		exit_error("Could not set socket option");
	return ret_code;
}

int
Bind( int fd, struct sockaddr_in *saddr, socklen_t len )
{
	int ret_code;

	errno = 0;
	ret_code = bind(fd, (struct sockaddr *)saddr, len);
	if(ret_code < 0) 
		exit_error("bind problem");
	return ret_code;
}

int
Getsockopt( int fd, int level, int optname, void *opt_val, 
	    socklen_t *optlen ) 
{
	int ret_code;

	errno = 0;
	ret_code = getsockopt(fd, level, optname, 
			      opt_val, optlen);
	if(ret_code < 0) 
		exit_error("Could not get socket info");
	return ret_code;
}

int
Listen(int fd, int backlog)
{
	int ret_code;

	errno = 0;
	ret_code = listen( fd, backlog );
	if(ret_code < 0) 
		exit_error("listen problem");
	return ret_code;
}

int
Fcntl( int fd, int cmd, int arg)
{
	int ret_code;

	errno = 0;
	ret_code = fcntl(fd, cmd, arg);
	if(ret_code < 0) 
		exit_error("fcntl problem");
	return ret_code;
}

int
Select(int maxfd, fd_set *rset, fd_set *wset, fd_set *eset, struct timeval *tv)
{
	int n;

	errno = 0;
	n = select(maxfd, rset, wset, eset, tv);
	if (n < 0)
/* Some sort of error occured.  Can we ignore it? */
		if (errno != EINTR) 
			exit_error("select error");
	if (n <= 0)
/* If it timed out then don't do anything else on this round of the loop */
		clear_sets(rset, wset, eset);
	return n;
}

void
Delay(struct timeval *tv)
{
	int n;
	struct timeval t;

	errno = 0;
	t = *tv;
	n = select(0, NULL, NULL, NULL, &t);
	if (n < 0)
/* Some sort of error occured.  Can we ignore it? */
		if (errno != EINTR) 
			exit_error("select error in Delay");
	return;
}

void
clear_sets(fd_set *rset, fd_set *wset, fd_set *eset)
{
	if(rset != NULL) FD_ZERO(rset);
	if(wset != NULL) FD_ZERO(wset);
	if(eset != NULL) FD_ZERO(eset);
}

int allocated_memory = 0;

char *
Malloc(int size)
{
	char *new;

	errno = 0;
	new = malloc(size);
	if (new == NULL)
		exit_error("Out of memory");
	allocated_memory += size;
	return new;
}

void Free(void *ptr, int size)
{
	allocated_memory -= size;
	free(ptr);
}

int
Accept(int fd, struct sockaddr_in *addr, socklen_t *addrlen)
{
	int new;

	errno = 0;
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
			exit_error("accept problem");
	}
	return new;
}

int
Connect(int fd, struct sockaddr *addr, socklen_t addrlen)
{
	int n;

	errno = 0;
	n = connect(fd, addr, addrlen);
	if(n < 0) 
	{
		if (errno != EINPROGRESS)
			exit_error("connect problem");
	}
	return n;
}

int
Read(int fd, char *p, int max)
{
	int n;

	errno = 0;
	n = read(fd, p, max);
	if (( n < 0 ) && ( errno != EWOULDBLOCK) && ( errno != ECONNRESET) )
		exit_error("Read error");
	return n;
}

int
Write(int fd, char *p, int max)
{
	int n;

	errno = 0;
	n = write(fd, p, max);
	if (( n < 0 ) && ( errno != EWOULDBLOCK ))
		exit_error("Write error");
	return n;
}

int
Open(char *str, int flags, int mode)
{
	int fd;

	errno = 0;
	fd = open(str, flags, mode);
	if (fd < 0)
		exit_error("Problem opening file %s", str);
	return fd;
}

void
Getaddrinfo(char *host, char *service, struct addrinfo *hints, 
	    struct addrinfo **addrinfo)
{
	int n;

	errno = 0;
	n = getaddrinfo(host, service, hints, addrinfo);
	if (n != 0)
		exit_error("getaddrinfo failed for host %s and service %s",
			   host, service);
}

void
Regcomp(regex_t *preg, const char *regex, int cflags)
{
	char buf[MAX_BUF];
	int n;
	int i = 0;
	int j = 0;

	errno = 0;
/* My lame hack because regcomp won't interpret '\r' and '\n' */
	while ( (i < MAX_BUF) && (regex[j] != '\0') )
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
	n = regcomp(preg, buf, cflags);
	if (n != REG_NOERROR) exit_error("regex compile failed");
}

int
Regexec(const regex_t *preg, const char *string, 
	size_t nmatch, regmatch_t pmatch[], int eflags)
{
	int n;
	char buf[MAX_BUF];
	
	re_syntax_options = RE_SYNTAX_POSIX_EXTENDED;
	strncpy(buf, string, MAX_BUF);
	n = regexec(preg, buf, nmatch, pmatch, eflags);
	return n;
}

pid_t
Fork(void)
{
        pid_t   pid;

	errno = 0;
        if ( (pid = fork()) == -1)
                exit_error("fork error");
        return(pid);
}

Sigfunc *
Signal(int signo, Sigfunc *func)
{
	struct sigaction act, oact;
	int n;

	errno = 0;
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
	if ( n < 0) exit_error("signal error");

	return(oact.sa_handler);
}
