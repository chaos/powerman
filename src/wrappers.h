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


#ifndef WRAPPERS_H
#define WRAPPERS_H

extern int allocated_memory;

/* Wrapper functions (in wrappers.c) */
extern int Socket(int family, int type, int protocol);
extern int Setsockopt( int fd, int level, int optname, const void *opt_val, 
		socklen_t optlen );
extern int Bind( int fd, struct sockaddr_in *addr, socklen_t len );
extern int Getsockopt( int fd, int level, int optname, void *opt_val, 
		socklen_t *optlen ); 
extern int Listen(int fd, int backlog);
extern int Fcntl( int fd, int cmd, int arg);
extern int Select(int maxfd, fd_set *rset, fd_set *wset, fd_set *eset, struct timeval *tv);
extern void Delay(struct timeval *tv);
extern char * Malloc(int size);
extern void Free(void *ptr, int size);
extern void Report_Memory();
extern int Accept(int fd, struct sockaddr_in *addr, socklen_t *addrlen);
extern int Connect(int fd, struct sockaddr *addr, socklen_t addrlen);
extern int Read(int fd, char *p, int max);
extern int Write(int fd, char *p, int max);
extern int Open(char *str, int flags, int mode);
extern void Getaddrinfo(char *host, char *service, struct addrinfo *hints, 
			struct addrinfo **addrinfo);
extern void Regcomp(regex_t *preg, const char *regex, int cflags);
extern int Regexec(const regex_t *preg, const char *string, 
		   size_t nmatch, regmatch_t pmatch[], int eflags);
extern pid_t Fork(void);
extern Sigfunc *Signal(int signo, Sigfunc *func);


#endif
