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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/time.h>
#include <time.h>
#include <regex.h>
#include <netdb.h>
#if HAVE_POLL
#include <sys/poll.h>
#endif

/*
 * If WITH_LSD_FATAL_ERROR_FUNC is defined, the linker will expect to
 * find an external lsd_fatal_error(file,line,mesg) function.  By default,
 * lsd_fatal_error(file,line,mesg) is a macro definition that outputs an
 * error message to stderr.  This macro may be redefined to invoke another
 * routine instead.
 * 
 * If WITH_LSD_NOMEM_ERROR_FUNC is defined, the linker will expect to
 * find an external lsd_nomem_error(file,line,mesg) function.  By default,
 * lsd_nomem_error(file,line,mesg) is a macro definition that returns NULL.
 * This macro may be redefined to invoke another routine instead.
 *
 * Credit: borrowed from cbuf.c (Chris Dunlap)
 */

typedef struct Pollfd *Pollfd_t;

/* Wrapper functions (in wrappers.c) */
int Socket(int family, int type, int protocol);
int Setsockopt(int fd, int level, int optname, const void *opt_val,
               socklen_t optlen);
int Bind(int fd, struct sockaddr_in *addr, socklen_t len);
int Getsockopt(int fd, int level, int optname, void *opt_val,
               socklen_t * optlen);
int Listen(int fd, int backlog);
int Fcntl(int fd, int cmd, int arg);

int Poll(Pollfd_t pfd, struct timeval *timeout);
Pollfd_t PollfdCreate(void);
void PollfdDestroy(Pollfd_t pfd);
void PollfdZero(Pollfd_t pfd);
void PollfdSet(Pollfd_t pfd, int fd, short events);
short PollfdRevents(Pollfd_t pfd, int fd);
char *PollfdStr(Pollfd_t pfd, char *str, int len);


#if !HAVE_POLL /* need these for poll emulation with select */
#ifndef POLLIN
#define POLLIN      1
#define POLLOUT     2
#define POLLHUP     4
#define POLLERR     8
#define POLLNVAL    16
#endif
#endif

#define Malloc(size)          wrap_malloc(__FILE__, __LINE__, size)
#define Realloc(item,newsize) wrap_realloc(__FILE__, __LINE__, item, newsize)
char *wrap_malloc(char *file, int line, int size);
char *wrap_realloc(char *file, int line, char *item, int newsize);

void Free(void *ptr);
char *Strdup(const char *str);
int Accept(int fd, struct sockaddr_in *addr, socklen_t * addrlen);
int Connect(int fd, struct sockaddr *addr, socklen_t addrlen);
int Read(int fd, unsigned char *p, int max);
int Write(int fd, unsigned char *p, int max);
int Open(char *str, int flags, int mode);
int Close(int fd);
int Getaddrinfo(char *host, char *service, struct addrinfo *hints,
                struct addrinfo **addrinfo);
void Regcomp(regex_t * preg, const char *regex, int cflags);
int Regexec(const regex_t * preg, const char *string,
            size_t nmatch, regmatch_t pmatch[], int eflags);
pid_t Fork(void);
typedef void Sigfunc(int);
Sigfunc *Signal(int signo, Sigfunc * func);
int Memory(void);
void Gettimeofday(struct timeval *tv, struct timezone *tz);
time_t Time(time_t * t);
char *Strncpy(char *s1, const char *s2, int len);
void Pipe(int filedes[2]);
void Dup2(int oldfd, int newfd);
void Execv(const char *path, char *const argv[]);
pid_t Waitpid(pid_t pid, int *status, int options);

#endif                          /* WRAPPERS_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
